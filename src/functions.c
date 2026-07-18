/*
 * All of the auxiliary functions used by the shell, the public interface is defined in functions.h
 * Copyright (C) 2026 BLT Sandwich
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

// NOTE: AI was used in part of the writing of this program, however it was only used as a guideline, and most of the code was hand-written.
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include "functions.h"

static void arrRem(char *arr[], int index) {
	int size = 0;
	while (arr[size] != NULL) size++;
	for (int nextIn = index; nextIn < size; nextIn++) {
		arr[nextIn] = arr[nextIn + 1];
	}
}

void setSignals(void (*sig)(int)) {
	struct sigaction sa;
	sa.sa_handler = sig;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGTSTP, &sa, NULL);
	sigaction(SIGTTIN, &sa, NULL);
	sigaction(SIGTTOU, &sa, NULL);
}

void doWait(pid_t child, pid_t parent, struct termios *modes, pid_t *stoppedPID, unsigned char *exitStat) {
	int status;
	waitpid(child, &status, WUNTRACED);
	tcsetpgrp(STDIN_FILENO, parent);
	*exitStat = WEXITSTATUS(status);
	if (modes != NULL)
		tcsetattr(STDIN_FILENO, TCSADRAIN, modes);
	if (WIFSTOPPED(status)) {
		write(STDERR_FILENO, "Child suspended\n", 16);
		*stoppedPID = child;
	}
}

char **parseArgs(char *input) {
	char **args = malloc(sizeof(char *));
	if (args == NULL)
		return NULL;
	int count = 0;
	char *tokPtr;
	for (char *token = strtok_r(input, " ", &tokPtr); token != NULL; token = strtok_r(NULL, " ", &tokPtr)) {
		args[count] = token;
		count++;
		char **allocCheck = realloc(args, (count + 1) * sizeof(char *));
		if (allocCheck == NULL) {
			free(args);
			return NULL;
		}
		args = allocCheck;
	}
	args[count] = NULL;
	return args;
}

int parseBuiltins(char *input[], unsigned char *status, pid_list *list) {
	if (strcmp(input[0], "exit") == 0) {
		if (input[1] != NULL) {
			char *checkLen;
			const long newStatus = strtol(input[1], &checkLen, 10);
			if (checkLen == input[1]) {
				write(STDERR_FILENO, "invalid argument\n", 17);
				return 2;
			}
			*status = newStatus % 256;
		}
		return 1;
	}
	if (strcmp(input[0], "fg") == 0) {
		*status = 0;
		if (*(list->stoppedPID) == 0) {
			write(STDERR_FILENO, "fg: no stopped job\n", 19);
			*status = 1;
			return 2;
		}
		pid_t job = *(list->stoppedPID);
		*(list->stoppedPID) = 0;
		tcsetpgrp(STDIN_FILENO, job);
		kill(-job, SIGCONT);
		doWait(job, *(list->parent), list->modes, list->stoppedPID, status);
		return 2;
	}
	if (strcmp(input[0], "cd") == 0) {
		*status = 0;
		if (input[1] == NULL) {
			char *home = getenv("HOME");
			if (home == NULL) {
				write(STDERR_FILENO, "how are you on linux with no home set?\n", 39);
				*status = 1;
				return 2;
			}
			int check = chdir(home);
			if (check == -1) {
				write(STDERR_FILENO, "that didn't work\n", 17);
				*status = 1;
			}
			return 2;
		}
		int check = chdir(input[1]);
		if (check == -1) {
			write(STDERR_FILENO, "that didn't work\n", 17);
			*status = 1;
		}
		return 2;
	}
	if (strcmp(input[0], "pwd") == 0) {
		*status = 0;
		char *cwd = getcwd(NULL, 0);
		if (cwd == NULL) {
			write(STDERR_FILENO, "getcwd malloc failed\n", 21);
			*status = 1;
			return 2;
		}
		write(STDOUT_FILENO, cwd, strlen(cwd));
		write(STDOUT_FILENO, "\n", 1);
		free(cwd);
		return 2;
	}
	if (strcmp(input[0], "pstat") == 0) {
		unsigned char prevStat = *status;
		*status = 0;
		char printStat[4];
		printStat[3] = '\0';
		int digit = 2;
		if (prevStat == 0) {
			printStat[digit] = '0';
			goto skip;
		}
		for (; prevStat != 0; digit--) {
			printStat[digit] = (prevStat % 10) + '0';
			prevStat /= 10;
		}
		digit++;
skip:;		write(STDOUT_FILENO, printStat + digit, 3 - digit);
		write(STDOUT_FILENO, "\n", 1);
		return 2;
	}
	if (strcmp(input[0], "which") == 0) {
		*status = 0;
		char *builtins[] = {
			(char *)"exit",
			(char *)"fg",
			(char *)"cd",
			(char *)"pwd",
			(char *)"which",
			(char *)"pstat",
			NULL
		};
		for (int argI = 1; input[argI] != NULL; argI++) {
			for (int i = 0; builtins[i] != NULL; i++) {
				if (strcmp(input[argI], builtins[i]) == 0) {
					write(STDOUT_FILENO, builtins[i], strlen(builtins[i]));
					write(STDOUT_FILENO, " is a shell built-in\n", 21);
					arrRem(input, argI);
					argI--;
					break;
				}
			}
		}
		if (input[1] == NULL) {
			*status = 1;
			return 2;
		}
	}
	return 0;
}

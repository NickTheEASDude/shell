/*
 * The main logic of the shell (it's not that impressive lol)
 * Copyright (C) 2026 BLT Sandwich
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

// NOTE: AI was used in part of the writing of this program, however it was only used as a guideline, and most of the code was hand-written.

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>
#include <termios.h>
#include "functions.h"
static struct termios parentTmodes;
static pid_t childStoppedPID = 0;

static void freeArgs(char **args) {
	for (int i = 0; args[i] != NULL; i++) free(args[i]);
	free(args);
}

int main() {
	pid_t parentPID = getpid();
	setpgid(parentPID, parentPID);
	tcsetpgrp(STDIN_FILENO, parentPID);
	tcgetattr(STDIN_FILENO, &parentTmodes);
	unsigned char currentStatus = 0;
	setSignals(SIG_IGN);
	for (;;) {
		if (childStoppedPID != 0) {
			pid_t checkZombie = waitpid(childStoppedPID, 0, WNOHANG);
			if (checkZombie == childStoppedPID)
				childStoppedPID = 0;
		}
		setSignals(SIG_IGN);
		tcsetattr(STDIN_FILENO, TCSADRAIN, &parentTmodes);
		write(STDOUT_FILENO, "# ", 2);
		char *command = NULL;
		size_t lineSize = 0;
		ssize_t bytesRead = getline(&command, &lineSize, stdin);
		if (bytesRead < 0) {
			if (feof(stdin)) {
				free(command);
				write(STDOUT_FILENO, "\n", 1);
				return currentStatus;
			}
			write(STDERR_FILENO, "getline somehow failed, trying again\n", 34);
			currentStatus = 1;
			continue;
		}
		if (command[bytesRead - 1] == '\n')
			command[bytesRead - 1] = '\0';
		else
			command[bytesRead] = '\0';
		if (command[0] == '\0')
			continue;
		char **parsedCmd = parseArgs(command);
		free(command);
		if (parsedCmd == NULL) {
			write(STDERR_FILENO, "one of the allocs failed\n", 25);
			return 3;
		}
		pid_list list = {
			.parent = &parentPID,
			.child = NULL,
			.modes = &parentTmodes,
			.stoppedPID = &childStoppedPID
		};
		int checkCont = parseBuiltins(parsedCmd, &currentStatus, &list);
		if (checkCont != 0) {
			freeArgs(parsedCmd);
			if (checkCont == 1)
				return currentStatus;
			continue;
		}
		pid_t childPID = fork();
		if (childPID < 0) {
			write(STDERR_FILENO, "the fork failed lol\n", 20);
			freeArgs(parsedCmd);
			return 1;
		} else if (childPID > 0) {
			setpgid(childPID, childPID);
			tcsetpgrp(STDIN_FILENO, childPID);
			doWait(childPID, parentPID, NULL, &childStoppedPID, &currentStatus);
			freeArgs(parsedCmd);
		} else {
			setSignals(SIG_DFL);
			setpgid(0, 0);
			execvp(parsedCmd[0], parsedCmd);
			write(STDERR_FILENO, "it no exist\n", 12);
			freeArgs(parsedCmd);
			_exit(1);
		}
	}
	return currentStatus;
}

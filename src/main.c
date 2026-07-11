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
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>
#include <termios.h>
#include "functions.h"
static struct termios parentTmodes;
static pid_t childStoppedPID = 0;

int main(int argc, char *argv[]) {
	pid_t parentPID = getpid();
	setpgid(parentPID, parentPID);
	tcsetpgrp(parentPID, parentPID);
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
		char command[4097];
		ssize_t bytesRead = read(STDIN_FILENO, command, 4096);
		if (bytesRead == 0)
			return currentStatus;
		if (command[bytesRead - 1] == '\n')
			command[bytesRead - 1] = '\0';
		else
			command[bytesRead] = '\0';
		if (command[0] == '\0')
			continue;
		char **parsedCmd = parseArgs(command);
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
			free(parsedCmd);
			if (checkCont == 1)
				return currentStatus;
			continue;
		}
		pid_t childPID = fork();
		if (childPID < 0) {
			write(STDERR_FILENO, "the fork failed lol\n", 20);
			free(parsedCmd);
			return 1;
		} else if (childPID > 0) {
			setpgid(childPID, childPID);
			tcsetpgrp(STDIN_FILENO, childPID);
			doWait(childPID, parentPID, NULL, &childStoppedPID, &currentStatus);
			free(parsedCmd);
		} else {
			setSignals(SIG_DFL);
			setpgid(0, 0);
			execvp(parsedCmd[0], parsedCmd);
			write(STDERR_FILENO, "it no exist\n", 12);
			free(parsedCmd);
			_exit(1);
		}
	}
	return currentStatus;
}

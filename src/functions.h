/*
 * The public interface for the functions defined in functions.c
 * Copyright (C) 2026 BLT Sandwich
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

// NOTE: AI was used in part of the writing of this program, however it was only used as a guideline, and most of the code was hand-written.
#ifndef	FUNCTIONS_H
#define FUNCTIONS_H
#include <sys/types.h>

void setSignals(void (*sig)(int));
void doWait(pid_t child, pid_t parent, struct termios *modes, pid_t *stoppedPID, char *exitStat);
char **parseArgs(char *input);
typedef struct {
	pid_t *parent;
	pid_t *child;
	struct termios *modes;
	pid_t *stoppedPID;
} pid_list;
int parseBuiltins(char *input[], char *status, pid_list *list);
#endif

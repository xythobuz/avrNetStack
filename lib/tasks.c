/*
 * tasks.c
 *
 * Copyright 2012 Thomas Buck <xythobuz@me.com>
 *
 * This file is part of avrNetStack.
 *
 * avrNetStack is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * avrNetStack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with avrNetStack.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <stdint.h>

#define DEBUG 0

#include <std.h>
#include <scheduler.h> // TimedTask typedef
#include <tasks.h>
#include <net/controller.h>

TimedTask *tasksWithCheck = NULL;
TestFunc *taskChecks = NULL;
uint8_t tasksWithCheckRegistered = 0;
uint8_t nextCheckTask = 0;

// ----------------------
// |    Internal API    |
// ----------------------

uint8_t extendTaskCheckList(void) {
	TestFunc *p;
	TimedTask *q;

	// Extend Check List
	p = (TestFunc *)mrealloc(taskChecks, (tasksWithCheckRegistered + 1) * sizeof(TestFunc), tasksWithCheckRegistered * sizeof(TestFunc));
	if (p == NULL) {
		return 1;
	}
	taskChecks = p;

	// Extend Task List
	q = (TimedTask *)mrealloc(tasksWithCheck, (tasksWithCheckRegistered + 1) * sizeof(TimedTask), tasksWithCheckRegistered * sizeof(TimedTask));
	if (q == NULL) {
		// Try to revert size of intervall list
		p = (TestFunc *)mrealloc(taskChecks, tasksWithCheckRegistered * sizeof(TestFunc), (tasksWithCheckRegistered + 1) * sizeof(TestFunc));
		if (p != NULL) {
			taskChecks = p;
		}
		return 1;
	}
	tasksWithCheck = q;

	tasksWithCheckRegistered++; // Success!
	return 0;
}

// ----------------------
// |    External API    |
// ----------------------

uint8_t addConditionalTask(TimedTask func, TestFunc testFunc) {
	// func will be executed if testFunc returns a value other than zero!
	if (extendTaskCheckList()) {
		return 1;
	}
	tasksWithCheck[tasksWithCheckRegistered - 1] = func;
	taskChecks[tasksWithCheckRegistered - 1] = testFunc;
	return 0;
}

void tasks(void) {
	// Check for Tasks that have a check function
	if (tasksWithCheckRegistered > 0) {
		if ((*taskChecks[nextCheckTask])() != 0) {
			(*tasksWithCheck[nextCheckTask])();
		}

		debugPrint("Checked for Task ");
		debugPrint(timeToString(nextCheckTask));
		debugPrint("\n");

		if (nextCheckTask < (tasksWithCheckRegistered - 1)) {
			nextCheckTask++;
		} else {
			nextCheckTask = 0;
		}
	}
}

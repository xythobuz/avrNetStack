/*
 * tasks.c
 *
 * Copyright (c) 2012, Thomas Buck <xythobuz@me.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdlib.h>
#include <stdint.h>

#define DEBUG 0

#include <std.h>
#include <tasks.h>
#include <net/controller.h>

Task *tasksWithCheck = NULL;
TestFunc *taskChecks = NULL;
uint8_t tasksWithCheckRegistered = 0;
uint8_t nextCheckTask = 0;

// ----------------------
// |    Internal API    |
// ----------------------

uint8_t extendTaskCheckList(void) {
    TestFunc *p;
    Task *q;

    // Extend Check List
    p = (TestFunc *)mrealloc(taskChecks, (tasksWithCheckRegistered + 1) * sizeof(TestFunc), tasksWithCheckRegistered * sizeof(TestFunc));
    if (p == NULL) {
        return 1;
    }
    taskChecks = p;

    // Extend Task List
    q = (Task *)mrealloc(tasksWithCheck, (tasksWithCheckRegistered + 1) * sizeof(Task), tasksWithCheckRegistered * sizeof(Task));
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

uint8_t taskTestAlways(void) {
    return 1;
}

uint8_t tasksRegistered(void) {
    return tasksWithCheckRegistered;
}

uint8_t addConditionalTask(Task func, TestFunc testFunc) {
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

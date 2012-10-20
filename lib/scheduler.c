/*
 * scheduler.c
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
/*
 * A very simple task scheduler. You can register up to 254 tasks.
 * Each task can be executed in different frequencies,
 * from 1ms between each execution up to (2^64 - 1)ms
 * Add tasks with addTimedTasks(foo, 1000);
 * Then Execute them in the main loop with while(1) { scheduler(); }
 */
#include <stdlib.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include <std.h>
#include <time.h>
#include <scheduler.h>

Task *timedTasks = NULL;
time_t *timedTaskIntervall = NULL;
time_t *timedTaskCounter = NULL;
uint8_t timedTasksRegistered = 0;

// ----------------------
// |    Internal API    |
// ----------------------

uint8_t extendTimedTaskList(void) {
    time_t *p;
    Task *q;

    // Extend Intervall List
    p = (time_t *)mrealloc(timedTaskIntervall, (timedTasksRegistered + 1) * sizeof(time_t), timedTasksRegistered * sizeof(time_t));
    if (p == NULL) {
        return 1;
    }
    timedTaskIntervall = p;

    // Extend Counter List
    p = (time_t *)mrealloc(timedTaskCounter, (timedTasksRegistered + 1) * sizeof(time_t), timedTasksRegistered * sizeof(time_t));
    if (p == NULL) {
        // Try to revert size of intervall list
        p = (time_t *)mrealloc(timedTaskIntervall, timedTasksRegistered * sizeof(time_t), (timedTasksRegistered + 1) * sizeof(time_t));
        if (p != NULL) {
            timedTaskIntervall = p;
        }
        return 1;
    }
    timedTaskCounter = p;

    // Extend Timed Task List
    q = (Task *)mrealloc(timedTasks, (timedTasksRegistered + 1) * sizeof(Task), timedTasksRegistered * sizeof(Task));
    if (q == NULL) {
        // Try to revert size of intervall list
        p = (time_t *)mrealloc(timedTaskIntervall, timedTasksRegistered * sizeof(time_t), (timedTasksRegistered + 1) * sizeof(time_t));
        if (p != NULL) {
            timedTaskIntervall = p;
        }
        // Try to revert size of counter list
        p = (time_t *)mrealloc(timedTaskCounter, timedTasksRegistered * sizeof(time_t), (timedTasksRegistered + 1) * sizeof(time_t));
        if (p != NULL) {
            timedTaskCounter = p;
        }
        return 1;
    }
    timedTasks = q;

    timedTasksRegistered++; // Success!
    return 0;
}

// ----------------------
// |    External API    |
// ----------------------

uint8_t schedulerRegistered(void) {
    return timedTasksRegistered;
}

// 0 on success
uint8_t addTimedTask(Task func, time_t intervall) {
    // func will be called every time_t milliseconds
    if (extendTimedTaskList()) {
        return 1;
    }
    timedTasks[timedTasksRegistered - 1] = func;
    timedTaskIntervall[timedTasksRegistered - 1] = intervall;
    timedTaskCounter[timedTasksRegistered - 1] = 0;
    return 0;
}

void scheduler(void) {
    time_t t = schedulerTimeFunc(), d;
    static time_t lastTimeSchedulerWasCalled = 0;
    uint8_t i;

    // Execute Timed Tasks
    d = diffTime(lastTimeSchedulerWasCalled, t);
    if (d > 0) {
        for (i = 0; i < timedTasksRegistered; i++) {
            timedTaskCounter[i] += d;
            if (timedTaskCounter[i] >= timedTaskIntervall[i]) {
                (*timedTasks[i])(); // Execute timed task
                timedTaskCounter[i] = 0;
            }
        }
        lastTimeSchedulerWasCalled = t;
    }
}

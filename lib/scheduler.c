/*
 * scheduler.c
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

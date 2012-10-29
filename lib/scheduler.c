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
 * A very simple task scheduler. Each task can be executed once
 * or in different frequencies, from 1ms between each execution
 * up to (2^64 - 1)ms
 * Add tasks with addTimedTasks(foo, 1000, repeat);
 * Then Execute them in the main loop with while(1) { scheduler(); }
 */
#include <stdlib.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include <std.h>
#include <time.h>
#include <scheduler.h>

typedef struct SchedElement SchedElement;
struct SchedElement {
    Task task;
    time_t intervall;
    time_t counter;
    uint8_t repeat;
    SchedElement* next;
};

SchedElement *schedulerList = NULL; // Single-linked-list

// ----------------------
// |    External API    |
// ----------------------

// 0 on success
uint8_t addTimedTask(Task func, time_t intervall, uint8_t repeat) {
    // Allocate new list element
    SchedElement *t = (SchedElement *)mmalloc(sizeof(SchedElement));
    if (t == NULL) {
        return 1;
    }

    // Fill with data
    t->task = func;
    t->intervall = intervall;
    t->repeat = repeat;

    // Put in front of list
    t->next = schedulerList;
    schedulerList = t;
    return 0;
}

uint8_t schedulerRegistered(void) {
    SchedElement *t = schedulerList;
    uint8_t count = 0;
    while (t != NULL) {
        count++;
        t = t->next;
    }
    return count;
}

void scheduler(void) {
    time_t t = schedulerTimeFunc(), d;
    static time_t lastTimeSchedulerWasCalled = 0;
    SchedElement *p = schedulerList, *previous = NULL;

    // Execute Timed Tasks
    d = diffTime(lastTimeSchedulerWasCalled, t);
    if (d > 0) {
        while (p != NULL) {
            p->counter += d;
            if (p->counter >= p->intervall) {
                p->counter = 0;
                p->task();
                if (p->repeat == 0) {
                    if (p == schedulerList) {
                        schedulerList = p->next;
                        mfree(p, sizeof(SchedElement));
                        p = schedulerList;
                    } else {
                        previous->next = p->next;
                        mfree(p, sizeof(SchedElement));
                        p = previous->next;
                    }
                }
            }
            previous = p;
            p = p->next;
        }
        lastTimeSchedulerWasCalled = t;
    }
}

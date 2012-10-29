/*
 * scheduler.h
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
#ifndef _scheduler_h
#define _scheduler_h

#include <time.h> // time_t definition
#include <tasks.h> // Task definition

#define schedulerTimeFunc(x) getSystemTime(x) // has to return system time in milliseconds

// Add a new timed task. Calling scheduler() in your main-loop
// will cause a call to func() every intervall milliseconds, if repeat != 0.
// If repeat == 0, func will be called once after intervall milliseconds.
uint8_t addTimedTask(Task func, time_t intervall, uint8_t repeat); // 0 on success
void scheduler(void); // Call this in your main loop!

uint8_t schedulerRegistered(void);

#endif

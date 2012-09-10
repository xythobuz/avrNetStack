/*
 * scheduler.h
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
#ifndef _scheduler_h
#define _scheduler_h

#include <time.h> // time_t definition
#include <tasks.h> // Task definition

#define schedulerTimeFunc(x) getSystemTime(x) // has to return system time in milliseconds

// Add a new timed task. Calling scheduler() in your main-loop
// will cause a call to func() every intervall milliseconds.
uint8_t addTimedTask(Task func, time_t intervall); // 0 on success
void scheduler(void); // Call this in your main loop!

uint8_t schedulerRegistered(void);

#endif

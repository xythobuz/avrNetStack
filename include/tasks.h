/*
 * tasks.h
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
#ifndef _tasks_h
#define _tasks_h

#include <scheduler.h> // TimedTask typedef

typedef uint8_t (*TestFunc)(void);

// Adds another conditional task that will cause func() to be called
// when testFunc() returns a value other than zero.
uint8_t addConditionalTask(TimedTask func, TestFunc testFunc); // 0 on success
void tasks(void); // Call in your main loop!

#endif

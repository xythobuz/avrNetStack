/*
 * time.h
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
#ifndef _time_h
#define _time_h

typedef uint64_t time_t;

typedef struct {
	uint16_t milliseconds;
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	uint8_t day;
	uint8_t month;
	uint16_t year;
} Time;

volatile extern Time currentTime;

void initSystemTimer(void);
time_t getSystemTime(void); // System uptime in ms
time_t getSystemTimeSeconds(void); // System uptime in seconds
time_t diffTime(time_t a, time_t b);
uint8_t daysInMonth(uint8_t month, uint16_t year);
void setTime(Time t);

#endif

/*
 * time.h
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
#ifndef _time_h
#define _time_h

#define TIMEZONE 2 // If you're eg. GMT-5, enter -5 or -4 on DST

typedef uint64_t time_t; // For milliseconds since system start or UNIX timestamp

void initSystemTimer(void);

time_t getSystemTime(void); // System uptime in ms
time_t getSystemTimeSeconds(void); // System uptime in seconds

time_t diffTime(time_t a, time_t b);

uint8_t daysInMonth(uint8_t month, uint16_t year);
uint8_t isLeapYear(uint16_t year);

void setTimestamp(time_t unix);
void setNtpTimestamp(time_t ntp);

// Fills y, m, d, h, min & sec with stamps values
void convertTimestamp(time_t stamp, uint16_t *y, uint8_t *m, uint8_t *d,
						uint8_t *h, uint8_t *min, uint8_t *sec);

#endif

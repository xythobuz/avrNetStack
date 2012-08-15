/*
 * time.c
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
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include <time.h>

// Uses Timer 2!
// Interrupt:
// Prescaler 64
// Count to 250
// => 1 Interrupt per millisecond

volatile time_t systemTime = 0; // Overflows in 500 million years... :)
volatile Time currentTime;

#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega2560__)
void initSystemTimer() {
	TCCR2A |= (1 << WGM21); // CTC Mode
#if F_CPU == 16000000
		TCCR2B |= (1 << CS22); // Prescaler: 64
		OCR2A = 250;
#elif F_CPU == 20000000
		TCCR2B |= (1 << CS22) | (1 << CS21); // Prescaler 256
		OCR2A = 78;
#else
#error F_CPU not compatible with timer module. DIY!
#endif
	TIMSK2 |= (1 << OCIE2A); // Enable compare match interrupt
#elif defined(__AVR_ATmega32__)
void initSystemTimer() {
	TCCR2 |= (1 << WGM21); // CTC Mode
#if F_CPU == 16000000
		TCCR2 |= (1 << CS22); // Prescaler: 64
		OCR2 = 250;
#elif F_CPU == 20000000
		TCCR2 |= (1 << CS22) | (1 << CS21); // Prescaler 256
		OCR2 = 78;
#else
#error F_CPU not compatible with timer module. DIY!
#endif
	TIMSK |= (1 << OCIE2); // Enable compare match interrupt
#else
#error MCU not compatible with timer module. DIY!
#endif
	currentTime.milliseconds = 0;
	currentTime.seconds = 0;
	currentTime.minutes = 0;
	currentTime.hours = 0;
	currentTime.day = 1;
	currentTime.month = 1;
	currentTime.year = 1970;
}

void setTime(Time *t) {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		currentTime.milliseconds = t->milliseconds;
		currentTime.seconds = t->seconds;
		currentTime.minutes = t->minutes;
		currentTime.hours = t->hours;
		currentTime.day = t->day;
		currentTime.month = t->month;
		currentTime.year = t->year;
	}
}

uint8_t daysInMonth(uint8_t month, uint16_t year) {
	switch (month) {
	default: case 4: case 6: case 9: case 11:
		return 30;
	case 1: case 3: case 5: case 7: case 8: case 10: case 12:
		return 31;
	case 2:
		if (((year % 400) == 0) || ((year % 4) == 0)) {
			return 29;
		} else {
			return 28;
		}
	}
}

#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega2560__)
ISR(TIMER2_COMPA_vect) {
#elif defined(__AVR_ATmega32__)
ISR(TIMER2_COMP_vect) {
#else
#error MCU not compatible with timer module. DIY!
#endif
	systemTime++;
	currentTime.milliseconds++;
	if (currentTime.milliseconds >= 1000) {
		currentTime.milliseconds = 0;
		currentTime.seconds++;
		if (currentTime.seconds >= 60) {
			currentTime.seconds = 0;
			currentTime.minutes++;
			if (currentTime.minutes >= 60) {
				currentTime.minutes = 0;
				currentTime.hours++;
				if (currentTime.hours >= 24) {
					currentTime.hours = 0;
					currentTime.day++;
					if (currentTime.day >= daysInMonth(currentTime.month, currentTime.year)) {
						currentTime.day = 1;
						currentTime.month++;
						if (currentTime.month >= 13) {
							currentTime.year++;
						}
					}
				}
			}
		}
	}
}

time_t getSystemTime(void) {
	return systemTime;
}

time_t getSystemTimeSeconds(void) {
	return systemTime / 1000;
}

time_t diffTime(time_t a, time_t b) {
	if (a > b) {
		return a - b;
	} else {
		return b - a;
	}
}

void incrementSeconds(time_t sec) {
	time_t i;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		for (i = 0; i < sec; i++) {
			currentTime.seconds++;
			if (currentTime.seconds >= 60) {
				currentTime.seconds = 0;
				currentTime.minutes++;
				if (currentTime.minutes >= 60) {
					currentTime.minutes = 0;
					currentTime.hours++;
					if (currentTime.hours >= 24) {
						currentTime.hours = 0;
						currentTime.day++;
						if (currentTime.day >= daysInMonth(currentTime.month, currentTime.year)) {
							currentTime.day = 1;
							currentTime.month++;
							if (currentTime.month >= 13) {
								currentTime.year++;
							}
						}
					}
				}
			}
		}
	}
}

void setNtpTimestamp(time_t ntp) {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		currentTime.year = 1900;
		currentTime.month = 1;
		currentTime.day = 1;
		currentTime.hours = 0;
		currentTime.minutes = 0;
		currentTime.seconds = 0;
		incrementSeconds(ntp);
	}
}

void setTimestamp(time_t unix) {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		currentTime.year = 1970;
		currentTime.month = 1;
		currentTime.day = 1;
		currentTime.hours = 0;
		currentTime.minutes = 0;
		currentTime.seconds = 0;
		incrementSeconds(unix);
	}
}

// From: http://de.wikipedia.org/wiki/Unixzeit#Beispiel-Implementierung :)
time_t getUnixTimestamp(void) {
	const uint16_t tage_bis_monatsanfang[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

	const uint16_t jahr = currentTime.year;
	const uint8_t monat = currentTime.month;
	const uint8_t tag = currentTime.day;
	const uint8_t stunde = currentTime.hours;
	const uint8_t minute = currentTime.minutes;
	const uint8_t sekunde = currentTime.seconds;

	time_t unix_zeit;
	uint16_t jahre = jahr - 1970;
	uint16_t schaltjahre = ((jahr - 1) - 1968) / 4 - ((jahr - 1) - 1900) / 100 + ((jahr - 1) - 1600) / 400;

	unix_zeit = sekunde + (time_t)60 * minute + 60 * 60 * stunde + (tage_bis_monatsanfang[monat - 1] + tag - 1) * 60 * 60 * 24 + (jahre * 365 + schaltjahre) * 60 * 60 * 24;

	if ((monat > 2) && (jahr % 4 == 0 && (jahr % 100 != 0 || jahr % 400 == 0)))
		unix_zeit += (time_t)60 * 60 * 24;
 
	return unix_zeit;
}

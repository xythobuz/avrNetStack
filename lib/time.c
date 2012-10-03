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

#include <std.h>
#include <time.h>

#define DEBUG 0

#include <net/controller.h>

// Uses Timer 2!
// Interrupt:
// Prescaler 64
// Count to 250
// => 1 Interrupt per millisecond

volatile time_t systemTime = 0; // Unix Timestamp, Overflows in 500 million years... :)

// Currently works with ATmega168, 32, 2560
#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega2560__)
#define TCRA TCCR2A
#define TCRB TCCR2B
#define OCR OCR2A
#define TIMS TIMSK2
#define OCIE OCIE2A
#elif defined(__AVR_ATmega32__)
#define TCRA TCCR2
#define TCRB TCCR2
#define OCR OCR2
#define TIMS TIMSK
#define OCIE OCIE2
#else
#error MCU not compatible with timer module. DIY!
#endif

void initSystemTimer() {
	// Timer initialization
	TCRA |= (1 << WGM21); // CTC Mode

#if F_CPU == 16000000
	TCRB |= (1 << CS22); // Prescaler: 64
	OCR = 250;
#elif F_CPU == 20000000
	TCRB |= (1 << CS22) | (1 << CS21); // Prescaler 256
	OCR = 78;
#else
#error F_CPU not compatible with timer module. DIY!
#endif

	TIMS |= (1 << OCIE); // Enable compare match interrupt
}

// ISR Name is MCU dependent
#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega2560__)
ISR(TIMER2_COMPA_vect) {
#elif defined(__AVR_ATmega32__)
ISR(TIMER2_COMP_vect) {
#else
#error MCU not compatible with timer module. DIY!
#endif
	systemTime++;
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

uint8_t daysInMonth(uint8_t month, uint16_t year) {
	switch (month) {
	default: case 4: case 6: case 9: case 11:
		return 30;
	case 1: case 3: case 5: case 7: case 8: case 10: case 12:
		return 31;
	case 2:
		if (isLeapYear(year)) {
			return 29;
		} else {
			return 28;
		}
	}
}

uint8_t isLeapYear(uint16_t year) {
	if ((year % 400) == 0) {
		return 1;
	} else if ((year % 100) == 0) {
		return 0;
	} else if ((year % 4) == 0) {
		return 1;
	}
	return 0;
}

void setTimestamp(time_t unix) {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		systemTime = unix;
		systemTime *= 1000;
	}
}

void setNtpTimestamp(time_t ntp) {
	// Convert NTP Timestamp to Unix Timestamp
	setTimestamp(ntp - 2208988800);
}

void incMonth(uint8_t *m, uint16_t *y) {
	(*m)++;
	if (*m >= 13) {
		*m = 1;
		(*y)++;
	}
}

void incDay(uint8_t *d, uint8_t *m, uint16_t *y) {
	(*d)++;
	if (*d > daysInMonth(*m, *y)) {
		*d = 1;
		incMonth(m, y);
	}
}

void incHour(uint8_t *h, uint8_t *d, uint8_t *m, uint16_t *y) {
	(*h)++;
	if (*h >= 24) {
		*h = 0;
		incDay(d, m, y);
	}
}

void incMinute(uint8_t *min, uint8_t *h, uint8_t *d, uint8_t *m, uint16_t *y) {
	(*min)++;
	if (*min >= 60) {
		*min = 0;
		incHour(h, d, m, y);
	}
}

void convertTimestamp(time_t stamp, uint16_t *y, uint8_t *m, uint8_t *d,
						uint8_t *h, uint8_t *min, uint8_t *sec) {
	uint8_t c;
	uint32_t max;
	*y = 1970;
	*m = 1;
	*d = 1;
	*h = 0;
	*min = 0;
	*sec = 0;

	stamp += (3600 * TIMEZONE);

	c = 2;
	while(c) { // years
		if (isLeapYear(*y)) {
			max = 31622400; // 366 days
		} else {
			max = 31536000; // 365 days
		}
		if (stamp >= max) {
			stamp -= max;
			(*y)++;
		} else {
			c--;
		}
	}

	c = 2;
	while(c) { // months
		max = daysInMonth(*m, *y) * 86400;
		if (stamp >= max) {
			stamp -= max;
			incMonth(m, y);
		} else {
			c--;
		}
	}

	while(stamp >= 86400) { // days
		stamp -= 86400;
		incDay(d, m, y);
	}

	while (stamp >= 3600) { // hours
		stamp -= 3600;
		incHour(h, d, m, y);
	}

	while (stamp >= 60) { // minutes
		stamp -= 60;
		incMinute(min, h, d, m, y);
	}

	while (stamp > 0) {
		stamp--;
		(*sec)++;
		if (*sec >= 60) {
			*sec = 0;
			incMinute(min, h, d, m, y);
		}
	}
}

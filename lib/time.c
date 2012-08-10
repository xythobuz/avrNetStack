/*
 * time.c
 *
 * Copyright 2011 Thomas Buck <xythobuz@me.com>
 *
 * This file is part of xyRobot.
 *
 * xyRobot is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * xyRobot is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with xyRobot.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include <time.h>

// Uses Timer 2!
// Interrupt:
// Prescaler 64
// Count to 250
// => 1 Interrupt per millisecond

volatile time_t systemTime = 0; // Overflows in 500 million years... :)

#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega2560)
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
}
#else
#error MCU not compatible with timer module. DIY!
#endif

ISR(TIMER2_COMPA_vect) {
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

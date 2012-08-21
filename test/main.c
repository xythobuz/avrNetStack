/*
 * main.c
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
#include <avr/io.h>
#include <stdint.h>
#include <stdlib.h>
#include <avr/interrupt.h>

#include <net/controller.h>
#include <net/icmp.h>
#include <time.h>
#include <serial.h>

// Thats, the MAC of my WLAN Module, with some bytes swapped...
MacAddress mac = {0x00, 0x1E, 0x99, 0x02, 0xC0, 0x42};

char buff[80];

char *timeToString(time_t s) {
	return ultoa(s, buff, 10);
}

void printStats(time_t sum, time_t max, time_t min, time_t count) {
	time_t avg = sum / count;
	serialWriteString("NetworkHandler Statistics:\nMax: ");
	serialWriteString(timeToString(max));
	serialWriteString("\nMin: ");
	serialWriteString(timeToString(min));
	serialWriteString("\nAverage: ");
	serialWriteString(timeToString(avg));
	serialWriteString("\nTimes called: ");
	serialWriteString(timeToString(count));
	serialWrite('\n');
}

void icmpCallBack(char *s) {
	serialWriteString("ICMP: ");
	serialWriteString(s);
	serialWrite('\n');
}

int main(void) {
	char c;
	time_t start, end, average = 0, max = 0, min = UINT64_MAX, count = 0;

	serialInit(BAUD(39400, F_CPU), 8, NONE, 1);

	DDRA = 0xC0;
	PORTA |= 0xC0; // LEDs on

	sei(); // Enable Interrupts so we get UART data before entering networkInit

	networkInit(mac);
	icmpRegisterMessageCallback(icmpCallBack);

	PORTA &= ~(0xC0); // LEDs off

	while(1) {
		// Network Handler Stats
		start = getSystemTime();
		networkHandler();
		count++;
		end = getSystemTime();
		average += diffTime(start, end);
		if (diffTime(start, end) > max) {
			max = diffTime(start, end);
		}
		if (diffTime(start, end) < min) {
			min = diffTime(start, end);
		}

		if (serialHasChar()) {
			c = serialGet();
			switch(c) {
				case 's':
					printStats(average, max, min, count);
					break;
				case '?': case 'v':
					serialWriteString("avrNetStack Debug\n");
					break;
				default:
					serialWrite(c);
					break;
			}
		}

		PORTA ^= (1 << PA6);
	}

	return 0;
}

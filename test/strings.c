/*
 * strings.c
 *
 * Copyright 2012 Thomas Buck <xythobuz@me.com>
 *
 * This file is part of avrNetStack.
 *
 * avrNetStack is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * avrNetStack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with avrNetStack.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <avr/pgmspace.h>
#include <serial.h>
#include <net/controller.h>

const char string0[] PROGMEM = "avrNetStack-Debug";
const char string1[] PROGMEM = " initialized!\n";
const char string2[] PROGMEM = "Link is down...\n";
const char string3[] PROGMEM = "Link is up!\n";
const char string4[] PROGMEM = "Handler returned: ";
const char string5[] PROGMEM = "ICMP Packet: ";
const char string6[] PROGMEM = "Last Protocol: ";
const char string7[] PROGMEM = "IPv4";
const char string8[] PROGMEM = "NTP Request: ";
const char string9[] PROGMEM = "DHCP Request: ";
const char string10[] PROGMEM = "Commands: qvdna\n";
const char string11[] PROGMEM = "Good Bye...\n\n";
const char string12[] PROGMEM = "ARP Table:\n";
const char string13[] PROGMEM = " --> ";

// Last index + 1
#define STRINGNUM 14

PGM_P stringTable[STRINGNUM] PROGMEM = { string0, string1, string2, string3, string4,
									string5, string6, string7, string8, string9,
									string10, string11, string12, string13 };

char stringNotFoundError[] PROGMEM = "String not found!\n";

char *getString(uint8_t id) {
	if (id < STRINGNUM) {
		strcpy_P(buff, (PGM_P)pgm_read_word(&(stringTable[id])));
	} else {
		strcpy_P(buff, (PGM_P)pgm_read_word(&stringNotFoundError));
	}
	return buff;
}

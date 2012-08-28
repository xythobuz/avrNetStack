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
#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/pgmspace.h>

#include <serial.h>
#include <net/controller.h>

const char string0[] PROGMEM = "avrNetStack-Debug";
const char string1[] PROGMEM = " initialized!\n";
const char string2[] PROGMEM = "Link is down!\n";
const char string3[] PROGMEM = "Link is up!\n";
const char string4[] PROGMEM = "Handler returned: ";
const char string5[] PROGMEM = "ICMP Packet: ";
const char string6[] PROGMEM = "No Handler";
const char string7[] PROGMEM = " for Protocol: ";
const char string8[] PROGMEM = "NTP Request: ";
const char string9[] PROGMEM = "DHCP Request: ";
const char string10[] PROGMEM = "Commands: qvdnalt\n";
const char string11[] PROGMEM = "Good Bye...\n\n";
const char string12[] PROGMEM = "\nARP Table:\n";
const char string13[] PROGMEM = " --> ";
const char string14[] PROGMEM = "Invalid";
const char string15[] PROGMEM = "Not enough Memory";
const char string16[] PROGMEM = "Success";
const char string17[] PROGMEM = "Waiting...\n";
const char string18[] PROGMEM = "Not yet available...\n";
const char string19[] PROGMEM = "Target MAC: ";

// Last index + 1
#define STRINGNUM 20

PGM_P stringTable[STRINGNUM] PROGMEM = { string0, string1, string2, string3, string4,
									string5, string6, string7, string8, string9,
									string10, string11, string12, string13, string14,
									string15, string16, string17, string18, string19 };

char stringNotFoundError[] PROGMEM = "String not found!\n";

char *getString(uint8_t id) {
	if (id < STRINGNUM) {
		strcpy_P(buff, (PGM_P)pgm_read_word(&(stringTable[id])));
	} else {
		strcpy_P(buff, (PGM_P)pgm_read_word(&stringNotFoundError));
	}
	return buff;
}

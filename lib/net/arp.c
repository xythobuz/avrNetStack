/*
 * arp.c
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
#include <stdint.h>
#include <avr/io.h>

#include <net/mac.h>
#include <net/ipv4.h>
#include <net/arp.h>

#define ARPTableSize 6

ARPTableEntry arpTable[ARPTableSize];

void arpInit(void) {
	uint8_t i, j;

	// Reset ARP Table
	for (i = 0; i < ARPTableSize; i++) {
		for (j = 0; j < 4; j++) {
			arpTable[i].ip[j] = 0;
		}
		for (j = 0; j < 6; j++) {
			arpTable[i].mac[j] = 0;
		}
		arpTable[i].time = 0;
	}
}

void arpProcessPacket(MacPacket *p) {

}

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
#include <avr/io.h>
#include <stdint.h>
#include <stdlib.h>

#include <net/mac.h>
#include <net/ipv4.h>
#include <net/arp.h>

ARPTableEntry arpTable[ARPTableSize];

// ------------------------
// |     Internal API     |
// ------------------------

uint8_t isZero(uint8_t *d, uint8_t l) {
	uint8_t i;
	for (i = 0; i < l; i++) {
		if (d[i] != 0) {
			return 0;
		}
	}
	return 1;
}

uint8_t isTableEntryFree(uint8_t i) {
	if (i < ARPTableSize) {
		if (isZero(arpTable[i].ip, 4) && isZero(arpTable[i].mac, 6)
			&& (arpTable[i].time == 0)) {
			return 1;
		}
	}
	return 0;
}

uint8_t oldestEntry(void) {
	uint8_t i;
	time_t min = UINT64_MAX;
	uint8_t pos = 0;
	for (i = 0; i < ARPTableSize; i++) {
		// It is not free, or else we would not need to delete it.
		if (arpTable[i].time < min) {
			min = arpTable[i].time;
			pos = i;
		}
	}
	return pos;
}

uint8_t getFirstFreeEntry(void) {
	uint8_t i;
	for (i = 0; i < ARPTableSize; i++) {
		if (isTableEntryFree(i)) {
			return i;
		}
	}
	
	// No free space, so we throw out the oldest
	return oldestEntry();
}

// ------------------------
// |     External API     |
// ------------------------

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

// Searches in ARP Table. If entry is found, return non-alloced buffer
// with mac address and update the time of the entry.
// If there is no entry, issue arp packet and return NULL. Try again later.
uint8_t *arpGetMacFromIp(IPv4Address ip) {
	return NULL;
}

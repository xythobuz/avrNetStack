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
#include <avr/pgmspace.h>

#include <net/mac.h>
#include <net/ipv4.h>
#include <net/arp.h>

ARPTableEntry arpTable[ARPTableSize];
/* const uint16_t hardwareType = 1; // Ethernet
 * const uint16_t protocolType = 0x0800; // IPv4
 * const uint8_t  hardwareAddressLength = 6; // MAC Length
 * const uint8_t  protocolAddressLength = 4; // IP Length */
#define HEADERLENGTH 6
uint8_t ArpPacketHeader[HEADERLENGTH] PROGMEM = {0x00, 0x01, 0x08, 0x00, 0x06, 0x04};
uint8_t zero[6] PROGMEM = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// ------------------------
// |     Internal API     |
// ------------------------

uint8_t isEqual(uint8_t *d1, uint8_t *d2, uint8_t l) {
	uint8_t i;
	for (i = 0; i < l; i++) {
		if (d1[i] != pgm_read_byte(&(d2[i]))) {
			return 0;
		}
	}
	return 1;
}

#define isZero(x, y) isEqual(x, zero, y)

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

// return 0 on success, 1 on error
uint8_t macPacketToArpPacket(MacPacket *mp, ArpPacket *ap) {
	uint8_t i;
	if ((mp != NULL) && (ap != NULL)) {
		// Check validity
		if (isEqual(mp->data, ArpPacketHeader, HEADERLENGTH)
			&& (mp->dLength >= (HEADERLENGTH + 22))) {
			// IPv4 Ethernet ARP Packet wich is big enough!
			ap->operation = (mp->data[HEADERLENGTH]) << 8;
			ap->operation |= (mp->data[HEADERLENGTH + 1]);
			for (i = 0; i < 6; i++) {
				ap->senderMac[i] = mp->data[HEADERLENGTH + 2 + i];
				ap->targetMac[i] = mp->data[HEADERLENGTH + 12 + i];
				if (i < 4) {
					ap->senderIp[i] = mp->data[HEADERLENGTH + 8 + i];
					ap->targetIp[i] = mp->data[HEADERLENGTH + 18 + i];
				}
			}
			return 0;
		} else {
			return 1;
		}
	} else {
		return 1;
	}
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
	ArpPacket *ap = (ArpPacket *)malloc(sizeof(ArpPacket));
	if (ap == NULL) {
		// Discard packet, return
		free(p->data);
		free(p);
		return;
	}
	if (macPacketToArpPacket(p, ap) != 0) {
		// Packet not valid!
		free(p->data);
		free(p);
		free(ap);
		return;
	}
	free(p->data);
	free(p); // We don't need the MacPacket anymore

	if (ap->operation == 1) {
		// ARP Request
	} else if (ap->operation == 2) {
		// ARP Reply. Store the information, if not already present
	} else {
		// Unknown ARP Operation
		free(ap);
		return;
	}
}

// Searches in ARP Table. If entry is found, return non-alloced buffer
// with mac address and update the time of the entry.
// If there is no entry, issue arp packet and return NULL. Try again later.
uint8_t *arpGetMacFromIp(IPv4Address ip) {
	return NULL;
}

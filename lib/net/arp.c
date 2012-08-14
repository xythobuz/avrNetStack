/*
 * arp.c
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
#include <avr/pgmspace.h>

#include <net/mac.h>
#include <net/ipv4.h>
#include <net/arp.h>
#include <net/utils.h>
#include <net/controller.h>

ARPTableEntry *arpTable = NULL;
uint8_t arpTableSize = 0;
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

#define isZero(x, y) isEqual(x, zero, y)

uint8_t isTableEntryFree(uint8_t i) {
	if (i < arpTableSize) {
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
	for (i = 0; i < arpTableSize; i++) {
		// It is not free, or else we would not need to delete it.
		if ((arpTable[i].time < min) && (arpTable[i].time != 0)) {
			min = arpTable[i].time;
			pos = i;
		}
	}
	return pos;
}

int8_t tooOldEntry(void) {
	uint8_t i;
	time_t time = getSystemTime();
	for (i = 0; i < arpTableSize; i++) {
		if (((arpTable[i].time + ARPTableTimeToLive) <= time) && (arpTable[i].time != 0)) {
			return i;
		}
	}
	return -1;
}

int8_t getFirstFreeEntry(void) {
	int8_t i;
	ARPTableEntry *tp;
	for (i = 0; i < arpTableSize; i++) {
		if (isTableEntryFree(i)) {
			return i;
		}
	}
	
	i = tooOldEntry();
	if (i != -1) {
		return i;
	}

	if (arpTableSize < ARPMaxTableSize) {
		// Allocate more space
		arpTableSize++;
		tp = (ARPTableEntry *)realloc(arpTable, (arpTableSize * sizeof(ARPTableEntry)));
		if (tp != NULL) {
			arpTable = tp;
			return (arpTableSize - 1);
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

int8_t findIpFromMac(MacAddress mac) {
	uint8_t i;
	for (i = 0; i < arpTableSize; i++) {
		if (!isTableEntryFree(i)) {
			if (isEqualMem(mac, arpTable[i].mac, 6)) {
				return (int8_t)i;
			}
		}
	}
	return -1;
}

int8_t findMacFromIp(IPv4Address ip) {
	uint8_t i;
	for (i = 0; i < arpTableSize; i++) {
		if (!isTableEntryFree(i)) {
			if (isEqualMem(ip, arpTable[i].ip, 4)) {
				return (int8_t)i;
			}
		}
	}
	return -1;
}

void copyEntry(MacAddress mac, IPv4Address ip, time_t time, uint8_t index) {
	uint8_t i;
	if ((arpTable != NULL) && (arpTableSize > index)) {
		for (i = 0; i < 6; i++) {
			if (i < 4) {
				arpTable[index].ip[i] = ip[i];
			}
			arpTable[index].mac[i] = mac[i];
		}
		arpTable[index].time = time;
	}
}

MacPacket *arpPacketToMacPacket(ArpPacket *ap) {
	uint8_t i;
	MacPacket *mp = (MacPacket *)malloc(sizeof(MacPacket));
	if (mp == NULL) {
		return NULL;
	}
	for (i = 0; i < 6; i++) {
		mp->destination[i] = ap->targetMac[i];
		mp->source[i] = ap->senderMac[i];
	}
	mp->typeLength = ARP;
	mp->dLength = ARPPacketSize + HEADERLENGTH;
	mp->data = (uint8_t *)malloc(mp->dLength * sizeof(uint8_t));
	if (mp->data == NULL) {
		free(mp);
		return NULL;
	}
	for (i = 0; i < HEADERLENGTH; i++) {
		mp->data[i] = pgm_read_byte(&(ArpPacketHeader[i]));
	}
	mp->data[HEADERLENGTH] = (uint8_t)(ap->operation & 0xFF00) >> 8;
	mp->data[HEADERLENGTH + 1] = (uint8_t)(ap->operation & 0x00FF);
	for (i = 0; i < 6; i++) {
		mp->data[HEADERLENGTH + 2 + i] = ap->senderMac[i];
		mp->data[HEADERLENGTH + 12 + i] = ap->targetMac[i];
		if (i < 4) {
			mp->data[HEADERLENGTH + 8 + i] = ap->senderIp[i];
			mp->data[HEADERLENGTH + 18 + i] = ap->targetIp[i];
		}
	}
	return mp;
}

// ------------------------
// |     External API     |
// ------------------------

void arpInit(void) {
	uint8_t i;
	arpTable = (ARPTableEntry *)malloc(sizeof(ARPTableEntry));
	if (arpTable != NULL) {
		for (i = 0; i < 6; i++) {
			arpTable[0].mac[i] = 0xFF;
			if (i < 4) {
				arpTable[0].ip[i] = 0xFF;
			}
		}
		arpTable[0].time = 0;
		arpTableSize = 1;
	}
}

uint8_t arpProcessPacket(MacPacket *p) {
	uint8_t i;
	ArpPacket *ap = (ArpPacket *)malloc(sizeof(ArpPacket));
	if (ap == NULL) {
		// Discard packet, return
		free(p->data);
		free(p);
		return 1;
	}
	if (macPacketToArpPacket(p, ap) != 0) {
		// Packet not valid!
		free(p->data);
		free(p);
		free(ap);
		return 2;
	}
	free(p->data);
	free(p); // We don't need the MacPacket anymore

	if (ap->operation == 1) {
		// ARP Request. Check if we have stored the sender MAC.
		if (findIpFromMac(ap->senderMac) == -1) {
			// Sender MAC is not stored. Store combination!
			copyEntry(ap->senderMac, ap->senderIp, getSystemTime(), getFirstFreeEntry());
		}
		// Check if the request is for us. If so, issue an answer!
		if (isEqualMem(ownIpAddress, ap->targetIp, 4)) {
			ap->operation = 2; // Reply
			for (i = 0; i < 6; i++) {
				ap->targetMac[i] = ap->senderMac[i]; // Goes back to sender
				ap->targetIp[i] = ap->senderIp[i];
				ap->senderMac[i] = ownMacAddress[i]; // Comes from us
				ap->senderIp[i] = ownIpAddress[i];
			}
			p = arpPacketToMacPacket(ap);
			if (p != NULL) {
				if (macSendPacket(p)) { // If it doesn't work, we can't do anything...
					// ...except trying again.
					macSendPacket(p);
				}
				free(p->data);
				free(p);
			}
		}
		// Request is not for us. Ignore!
		free(ap);
	} else if (ap->operation == 2) {
		// ARP Reply. Store the information, if not already present
		// Each packet contains two MAC-IP Combinations. Sender & Target
		if (findIpFromMac(ap->senderMac) == -1) {
			// Sender MAC is not stored. Store combination!
			copyEntry(ap->senderMac, ap->senderIp, getSystemTime(), getFirstFreeEntry());
		}
		if (findIpFromMac(ap->targetMac) == -1) {
			// Target MAC is not stored. Store combination!
			copyEntry(ap->targetMac, ap->targetIp, getSystemTime(), getFirstFreeEntry());
		}
		free(ap);
	}
	return 0;
}

uint8_t macReturnBuffer[6];

// Searches in ARP Table. If entry is found, return non-alloced buffer
// with mac address and update the time of the entry.
// If there is no entry, issue arp packet and return NULL. Try again later.
uint8_t *arpGetMacFromIp(IPv4Address ip) {
	uint8_t i;
	int8_t index = findMacFromIp(ip);
	ArpPacket *ap;
	MacPacket *p;

	if (index != -1) {
		for (i = 0; i < 6; i++) {
			macReturnBuffer[i] = arpTable[index].mac[i];
		}
		arpTable[index].time = getSystemTime();
		return macReturnBuffer;
	} else {
		// No entry found. Issue ARP Request
		ap = (ArpPacket *)malloc(sizeof(ArpPacket));
		if (ap == NULL) {
			return NULL;
		}
		ap->operation = 1; // Request
		for (i = 0; i < 6; i++) {
			ap->senderMac[i] = ownMacAddress[i];
			ap->targetMac[i] = 0xFF; // Broadcast
			if (i < 4) {
				ap->senderIp[i] = ownIpAddress[i];
				ap->targetIp[i] = ip[i];
			}
		}
		p = arpPacketToMacPacket(ap);
		free(ap);
		if (p == NULL) {
			return NULL;
		}
		if (macSendPacket(p)) { // Can't do anything if error...
			// ...except try again
			macSendPacket(p);
		}
		return NULL;
	}
}

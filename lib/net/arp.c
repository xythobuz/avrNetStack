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

#define DEBUG 2 // 0 to receive no debug serial output
// 2 to also get a message for every received ARP Request.

#include <std.h>
#include <net/mac.h>
#include <net/ipv4.h>
#include <net/arp.h>
#include <net/utils.h>
#include <serial.h> // debug output
#include <net/controller.h>

ARPTableEntry *arpTable = NULL;
uint8_t arpTableSize = 0;
/* const uint16_t hardwareType = 1; // Ethernet
 * const uint16_t protocolType = 0x0800; // IPv4
 * const uint8_t  hardwareAddressLength = 6; // MAC Length
 * const uint8_t  protocolAddressLength = 4; // IP Length */
#define HEADERLENGTH 6
uint8_t ArpPacketHeader[HEADERLENGTH] PROGMEM = {0x00, 0x01, 0x08, 0x00, 0x06, 0x04};
uint8_t zero[6] PROGMEM = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // 12 bytes in flash, who cares?

// ------------------------
// |     Internal API     |
// ------------------------

#define isZero(x, y) isEqualFlash(x, zero, y)

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
	time_t t, time = getSystemTime();
	for (i = 0; i < arpTableSize; i++) {
		if (!isZero(arpTable[i].mac, 6)) {
			// Normal full entry
			t = arpTable[i].time + ARPTableTimeToLive;
		} else {
			// Waiting for Reply
			t = arpTable[i].time + ARPTableTimeout;
		}
		if ((t <= time) && (arpTable[i].time != 0)) {
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
		tp = (ARPTableEntry *)mrealloc(arpTable, arpTableSize * sizeof(ARPTableEntry), (arpTableSize - 1) * sizeof(ARPTableEntry));
		if (tp != NULL) {
			arpTable = tp;
			return (arpTableSize - 1);
		}
	}

	// No free space, so we throw out the oldest
	return oldestEntry();
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

void copyEntry(uint8_t *mac, IPv4Address ip, time_t time, uint8_t index) {
	uint8_t i;
	if ((arpTable != NULL) && (arpTableSize > index)) {
		for (i = 0; i < 6; i++) {
			if (i < 4) {
				arpTable[index].ip[i] = ip[i];
			}
			if (mac != NULL) {
				arpTable[index].mac[i] = mac[i];
			} else {
				arpTable[index].mac[i] = 0x00;
			}
		}
		arpTable[index].time = time;
	}
}

uint8_t isIpInThisNetwork(uint8_t *d) {
	uint8_t i;
	for (i = 0; i < 4; i++) {
		if (subnetmask[i] == 255) {
			if (d[i] != defaultGateway[i]) {
				return 0;
			}
		}
	}
	return 1;
}

// ------------------------
// |     External API     |
// ------------------------

void arpInit(void) {
	uint8_t i;
	arpTable = (ARPTableEntry *)mmalloc(sizeof(ARPTableEntry));
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

uint8_t arpProcessPacket(Packet *p) {
	uint8_t i;

	assert(arpTableSize > 0); // At least the broadcast MAC should be there

	if (!(isEqualFlash(p->d + MACPreambleSize, ArpPacketHeader, HEADERLENGTH) && (p->dLength >= (HEADERLENGTH + 22 + MACPreambleSize)))) {
		// Packet invalid
		debugPrint("ARP Packet not valid!\n");
		mfree(p->d, p->dLength);
		mfree(p, sizeof(Packet));
		return 2;
	}

	if (p->d[MACPreambleSize + HEADERLENGTH + 1] == 1) {
		// ARP Request. Check if we have stored the sender MAC.
		if (findIpFromMac(p->d + MACPreambleSize + HEADERLENGTH + 2) == -1) {
			// Sender MAC is not stored. Store combination!
			copyEntry(p->d + MACPreambleSize + HEADERLENGTH + 2, p->d + MACPreambleSize + HEADERLENGTH + 8, getSystemTime(), getFirstFreeEntry());
		}
		// Check if the request is for us. If so, issue an answer!
		if (isEqualMem(ownIpAddress, p->d + MACPreambleSize + HEADERLENGTH + 18, 4)) {
			debugPrint("ARP Request for us!");
			p->d[MACPreambleSize + HEADERLENGTH + 1] = 2; // Reply
			for (i = 0; i < 6; i++) {
				p->d[MACPreambleSize + HEADERLENGTH + 12 + i] = p->d[MACPreambleSize + HEADERLENGTH + 2 + i]; // Back to sender
				p->d[MACPreambleSize + HEADERLENGTH + 2 + i] = ownMacAddress[i]; // Comes from us
				p->d[i] = 255;
				p->d[i + 6] = ownMacAddress[i];
				p->d[12] = (ARP & 0xFF00) >> 8;
				p->d[13] = (ARP & 0x00FF);
				if (i < 4) {
					p->d[MACPreambleSize + HEADERLENGTH + 18 + i] = p->d[MACPreambleSize + HEADERLENGTH + 8 + i];

					p->d[MACPreambleSize + HEADERLENGTH + 8 + i] = ownIpAddress[i];
				}
			}
			debugPrint(" Sending Response...");
			if (macSendPacket(p)) { // If it doesn't work, we can't do anything...
				// ...except trying again.
				debugPrint(" Again...");
				if (macSendPacket(p)) {
					mfree(p->d, p->dLength);
					mfree(p, sizeof(Packet));
					debugPrint(" Error!\n");
					return 1;
				}
			}
			mfree(p->d, p->dLength);
			mfree(p, sizeof(Packet));
			debugPrint(" Done!\n");
			return 0;
		} else {
#if DEBUG >= 2
			debugPrint("ARP Request for ");
			for (i = 0; i < 4; i++) {
				debugPrint(timeToString(p->d[MACPreambleSize + HEADERLENGTH + 18 + i]));
				if (i < 3) {
					debugPrint(".");
				}
			}
			debugPrint("\n");
#endif
		}
		// Request is not for us. Ignore!
		mfree(p->d, p->dLength);
		mfree(p, sizeof(Packet));
		return 0;
	} else if (p->d[MACPreambleSize + HEADERLENGTH + 1] == 2) {
		debugPrint("Got ARP Reply\n");
		// ARP Reply. Store the information, if not already present
		// Each packet contains two MAC-IP Combinations. Sender & Target
		if (findIpFromMac(p->d + MACPreambleSize + HEADERLENGTH + 2) == -1) {
			if (!isEqualMem(ownIpAddress, p->d + MACPreambleSize + HEADERLENGTH + 8, 4)) {
				// Sender MAC is not stored. Store combination!
				i = findMacFromIp(p->d + MACPreambleSize + HEADERLENGTH + 8);
				if (i == -1) {
					i = getFirstFreeEntry();
				}
				debugPrint("Sender unknown!\n");
				copyEntry(p->d + MACPreambleSize + HEADERLENGTH + 2, p->d + MACPreambleSize + HEADERLENGTH + 8, getSystemTime(), i);
			}
		}
		if (findIpFromMac(p->d + MACPreambleSize + HEADERLENGTH + 12) == -1) {
			if (!isEqualMem(ownIpAddress, p->d + MACPreambleSize + HEADERLENGTH + 18, 4)) {
				// Target MAC is not stored. Store combination!
				i = findMacFromIp(p->d + MACPreambleSize + HEADERLENGTH + 18);
				if (i == -1) {
					i = getFirstFreeEntry();
				}
				debugPrint("Target unknown!\n");
				copyEntry(p->d + MACPreambleSize + HEADERLENGTH + 12, p->d + MACPreambleSize + HEADERLENGTH + 18, getSystemTime(), i);
			}
		}
		mfree(p->d, p->dLength);
		mfree(p, sizeof(Packet));
		return 0;
	} else {
		// Neither request nor reply...
		debugPrint("Invalid ARP Packet Type!\n");
		mfree(p->d, p->dLength);
		mfree(p, sizeof(Packet));
		return 2;
	}
	return 0;
}

uint8_t macReturnBuffer[6];

// Searches in ARP Table. If entry is found, return non-alloced buffer
// with mac address and update the time of the entry.
// If there is no entry, issue arp packet and return NULL. Try again later.
uint8_t *arpGetMacFromIp(IPv4Address ip) {
	uint8_t i, a;
	int8_t index = findMacFromIp(ip);
	Packet *p;

	if (!isIpInThisNetwork(ip)) {
#if DEBUG >= 1
		debugPrint("ARP Cache Request for IP: ");
		for (i = 0; i < 4; i++) {
			debugPrint(timeToString(ip[i]));
			if (i < 3) {
				debugPrint(".");
			}
		}
		debugPrint("\nRedirecting to default Gateway...\n");
#endif
		return arpGetMacFromIp(defaultGateway);
	}

	if (index != -1) {
		a = 0;
		for (i = 0; i < 6; i++) {
			macReturnBuffer[i] = arpTable[index].mac[i];
			if (macReturnBuffer[i] != 0x00) {
				a++;
			}
		}
		if (a == 0) {
			// Not yet found but already requested. Return NULL!
			debugPrint("MAC already requested, waiting for response...\n");
			return NULL;
		}
		arpTable[index].time = getSystemTime();
		return macReturnBuffer;
	} else {
		// No entry found. Issue ARP Request
#if DEBUG >= 1
		debugPrint("Sending ARP Request for ");
		for (i = 0; i < 4; i++) {
			debugPrint(timeToString(ip[i]));
			if (i < 3) {
				debugPrint(".");
			}
		}
		debugPrint("...");
#endif
		p = (Packet *)mmalloc(sizeof(Packet));
		if (p == NULL) {
			debugPrint("Not enough memory for Packet struct!\n");
			return NULL;
		}
		p->d = (uint8_t *)mmalloc(MACPreambleSize + HEADERLENGTH + ARPPacketSize);
		if (p->d == NULL) {
			mfree(p, sizeof(Packet));
			return NULL;
		}
		for (i = 0; i < 6; i++) {
			p->d[i] = 0xFF; // Target MAC
			p->d[6 + i] = ownMacAddress[i];
			p->d[MACPreambleSize + i] = pgm_read_byte(&(ArpPacketHeader[i])); // ARP Header
			p->d[MACPreambleSize + HEADERLENGTH + 2 + i] = ownMacAddress[i];
			p->d[MACPreambleSize + HEADERLENGTH + 12 + i] = 0xFF;
			if (i < 4) {
				p->d[MACPreambleSize + HEADERLENGTH + 8 + i] = ownIpAddress[i];
				p->d[MACPreambleSize + HEADERLENGTH + 18 + i] = ip[i]; // Target IP
			}
		}
		p->d[12] = (ARP & 0xFF00) >> 8;
		p->d[13] = (ARP & 0x00FF); // ARP Packet
		p->d[MACPreambleSize + HEADERLENGTH] = 0;
		p->d[MACPreambleSize + HEADERLENGTH + 1] = 1; // Request
		p->dLength = MACPreambleSize + HEADERLENGTH + ARPPacketSize;
		i = macSendPacket(p);
		mfree(p->d, p->dLength);
		mfree(p, sizeof(Packet));
		if (i) {
			debugPrint(" Couldn't send!\n");
			return NULL;
		} else {
			debugPrint(" Done!\n");
		}
		copyEntry(NULL, ip, getSystemTime(), getFirstFreeEntry());
		return NULL;
	}
}

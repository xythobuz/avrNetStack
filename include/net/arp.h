/*
 * arp.h
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
#ifndef _arp_h
#define _arp_h

#include <net/mac.h>
#include <net/ipv4.h>
#include <time.h>

#define ARPTableSize 6

typedef struct {
	IPv4Address ip;
	MacAddress  mac;
	time_t      time;
} ARPTableEntry;

extern ARPTableEntry arpTable[ARPTableSize];

#define ARPPacketSize 22 // Without header

typedef struct {
	uint16_t       operation; // 1 for request, 2 for reply
	MacAddress     senderMac;
	IPv4Address    senderIp;
	MacAddress     targetMac;
	IPv4Address    targetIp;
} ArpPacket;

void arpInit(void);
uint8_t arpProcessPacket(MacPacket *p); // Processes all received ARP Packets
// Returns 0 on success, 1 if there was not enough memory,
// 2 if the packet was no valid ipv4 ethernet arp packet..
// p is freed afterwards!

// Searches in ARP Table. If entry is found, return non-alloced buffer with mac address.
// If there is no entry, issue arp packet and return NULL. Try again later.
uint8_t *arpGetMacFromIp(IPv4Address ip);

#endif
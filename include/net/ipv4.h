/*
 * ipv4.h
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
#ifndef _ipv4_h
#define _ipv4_h

#include <net/mac.h>
#include <net/controller.h>

typedef uint8_t IPv4Address[4];

typedef struct {
	uint8_t version;
	uint8_t internetHeaderLength; // This times 4 is the complete header length
	uint8_t typeOfService;
	// First 16bit

	uint16_t totalLength; // 16bit
	uint16_t identification; // 16bit
	uint8_t flags; // 3bit
	uint16_t fragmentOffset; // 13bit
	// Next 48bit

	uint8_t timeToLive;
	uint8_t protocol;
	uint16_t headerChecksum;
	// Next 32bit

	IPv4Address sourceIp;
	IPv4Address destinationIp;
	uint8_t *options; // Not NULL if internetHeaderLength > 5
	uint8_t *data;
	uint16_t dLength; // Real length of data buffer
} IPv4Packet;

#define ICMP 0x01
#define IGMP 0x02
#define TCP 0x06
#define UDP 0x11

extern IPv4Address ownIpAddress;
extern IPv4Address subnetmask;
extern IPv4Address defaultGateway;

void ipv4Init(IPv4Address ip, IPv4Address subnet, IPv4Address gateway);

uint8_t ipv4ProcessPacket(MacPacket *p);
// Returns 0 on success, 1 if not enough mem, 2 if packet invalid.
// p is freed afterwards!

// Returns 0 if packet was sent. 1 if destination was unknown.
// Try again later, after ARP response could have arrived...
// Returns 2 if there was not enough memory.
// Checksum is calculated for you. Leave checksum field 0x00, as well as identification
// If data is too large, packet is fragmented automatically
// Returns 3 on PHY Error. If Return is 0 or 3, the IPv4Packet is freed!
uint8_t ipv4SendPacket(IPv4Packet *p);

#endif

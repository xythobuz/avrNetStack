/*
 * ipv4.c
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

#define DEBUG 0 // 0 to receive no debug serial output

#include <time.h>
#include <net/mac.h>
#include <net/arp.h>
#include <net/controller.h>
#include <net/ipv4.h>
#include <net/icmp.h>
#include <net/udp.h>
#include <net/utils.h>

IPv4Address ownIpAddress;
IPv4Address subnetmask;
IPv4Address defaultGateway;

// ----------------------
// |    Internal API    |
// ----------------------

uint16_t risingIdentification = 1;

uint8_t isBroadcastIp(uint8_t *d) {
	uint8_t i;
	for (i = 0; i < 4; i++) {
		if (subnetmask[i] == 255) {
			if (!((d[i] == 255) || (d[i] == defaultGateway[i]))) {
				// ip does not match subnet or broadcast
				return 0;
			}
		} else {
			if (d[i] != 255) {
				return 0;
			}
		}
	}
	return 1;
}

#if (!defined(DISABLE_IPV4_CHECKSUM)) || (!defined(DISABLE_UDP_CHECKSUM))
uint16_t checksum(uint8_t *rawData, uint16_t l) {
	uint32_t a = 0;
	uint16_t i;
	uint16_t w;

	for (i = 0; i < l; i += 2) {
		if (i == (l - 1)) {
			w = ((uint16_t)rawData[i] << 8);
		} else {
			w = ((uint16_t)rawData[i] << 8);
			w |= rawData[i + 1];
		}
		a += w;
		
	}
	a = (a & 0xFFFF) + ((a >> 16) & 0xF); // 1's complement 16bit sum
	a = ~a; // 1's complement of 1's complement 16bit sum
	return a;
}
#endif

// ----------------------
// |    External API    |
// ----------------------

void ipv4Init(IPv4Address ip, IPv4Address subnet, IPv4Address gateway) {
	uint8_t i;
	for (i = 0; i < 4; i++) {
		ownIpAddress[i] = ip[i];
		subnetmask[i] = subnet[i];
		defaultGateway[i] = gateway[i];
	}
#if DEBUG >= 1
	debugPrint("IP: ");
	for (i = 0; i < 4; i++) {
		debugPrint(timeToString(ip[i]));
		if (i < 3) {
			debugPrint(".");
		}
	}
	debugPrint("\nSubnet: ");
	for (i = 0; i < 4; i++) {
		debugPrint(timeToString(subnet[i]));
		if (i < 3) {
			debugPrint(".");
		}
	}
	debugPrint("\nGateway: ");
	for (i = 0; i < 4; i++) {
		debugPrint(timeToString(gateway[i]));
		if (i < 3) {
			debugPrint(".");
		}
	}
	debugPrint("\n");
#endif
}

uint8_t ipLastProtocol = 0x00;

uint8_t ipv4LastProtocol(void) {
	return ipLastProtocol;
}

// Returns 0 on success, 1 if not enough mem, 2 if packet invalid.
uint8_t ipv4ProcessPacket(Packet *p) {
	uint16_t cs = 0x0000, w;
	uint8_t pr;
#if DEBUG >= 2
	uint8_t i;
#endif
#ifndef DISABLE_IPV4_CHECKSUM
	cs = checksum(p->d + MACPreambleSize, IPv4PacketHeaderLength);
#endif
	if ((cs != 0x0000) || ((p->d[MACPreambleSize] & 0xF0) != 0x40)) {
		// Checksum or version invalid
#if DEBUG >= 1
		debugPrint("Checksum: ");
		debugPrint(hexToString(cs));
		debugPrint("  First byte: "),
		debugPrint(hexToString(p->d[MACPreambleSize]));
		debugPrint("!\n");
#endif
		free(p->d);
		free(p);
		return 2;
	} else {
		debugPrint("Valid IPv4 Packet!\n");
	}

	w = get16Bit(p->d, MACPreambleSize + IPv4PacketFlagsOffset);
	if (w & 0x1FFF) {
		debugPrint("Fragment Offset is ");
		debugPrint(hexToString(w & 0x1FFF));
		debugPrint("!\n");
		free(p->d);
		free(p);
		return 2;
	}
	if (w & 0x2000) {
		// Part of a fragmented IPv4 Packet... No support for that
		debugPrint("More Fragments follow!\n");
		free(p->d);
		free(p);
		return 2;
	}

	if (isBroadcastIp(p->d + MACPreambleSize + IPv4PacketDestinationOffset)) {
		debugPrint("IPv4 Broadcast Packet!\n");
	} else if (isEqualMem(ownIpAddress, p->d + MACPreambleSize + IPv4PacketDestinationOffset, 4)) {
		debugPrint("IPv4 Packet for us!\n");
	} else {
		debugPrint("IPv4 Packet not for us!\n");
		free(p->d);
		free(p);
		return 0;
	}

	// Packet to act on...
#if DEBUG >= 2
	debugPrint("From: ");
	for (i = 0; i < 4; i++) {
		debugPrint(timeToString(p->d[MACPreambleSize + IPv4PacketSourceOffset + i]));
		if (i < 3) {
			debugPrint(".");
		}
	}
	debugPrint("\nTo: ");
	for (i = 0; i < 4; i++) {
		debugPrint(timeToString(p->d[MACPreambleSize + IPv4PacketDestinationOffset + i]));
		if (i < 3) {
			debugPrint(".");
		}
	}
	debugPrint("\n");
#endif

	pr = p->d[MACPreambleSize + IPv4PacketProtocolOffset];
	ipLastProtocol = pr;
	if (pr == ICMP) {
		debugPrint("Is ICMP Packet!\n");
		return icmpProcessPacket(p);
	} else if (pr == IGMP) {
		debugPrint("Is IGMP Packet!\n");
	} else if (pr == TCP) {
		debugPrint("Is TCP Packet!\n");
	} else if (pr == UDP) {
		debugPrint("Is UDP Packet!\n");
		return udpHandlePacket(p);
	} else {
#if DEBUG >= 1
		debugPrint("No handler for: ");
		debugPrint(hexToString(pr));
		debugPrint("!\n");
		free(p->d);
		free(p);
		return 0;
#endif
	}

	free(p->d);
	free(p);
	return 0;
}

uint8_t ipv4SendPacket(Packet *p, uint8_t *target, uint8_t protocol) {
	uint16_t tLength = p->dLength - MACPreambleSize;
	uint8_t *mac = NULL;
	p->d[MACPreambleSize + 0] = (4) << 4; // Version
	p->d[MACPreambleSize + 0] |= 5; // InternetHeaderLength
	p->d[MACPreambleSize + 1] = 0; // Type Of Service
	p->d[MACPreambleSize + 2] = (tLength & 0xFF00) >> 8;
	p->d[MACPreambleSize + 3] = (tLength & 0x00FF);
	p->d[MACPreambleSize + 4] = (risingIdentification & 0xFF00) >> 8;
	p->d[MACPreambleSize + 5] = (risingIdentification++ & 0x00FF);
	p->d[MACPreambleSize + 6] = 0;
	p->d[MACPreambleSize + 7] = 0; // Flags and Fragment Offset
	p->d[MACPreambleSize + 8] = 0xFF; // Time To Live
	p->d[MACPreambleSize + 10] = 0;
	p->d[MACPreambleSize + 11] = 0; // Checksum field
#ifndef DISABLE_IPV4_CHECKSUM
	tLength = checksum(p->d + MACPreambleSize, IPv4PacketHeaderLength);
	p->d[MACPreambleSize + 10] = (tLength & 0xFF00) >> 8;
	p->d[MACPreambleSize + 11] = (tLength & 0x00FF);
#endif
	p->d[MACPreambleSize + IPv4PacketProtocolOffset] = protocol;
	for (tLength = 0; tLength < 4; tLength++) {
		p->d[MACPreambleSize + IPv4PacketSourceOffset + tLength] = ownIpAddress[tLength];
		p->d[MACPreambleSize + IPv4PacketDestinationOffset + tLength] = target[tLength];
	}
	if ((mac = arpGetMacFromIp(target)) == NULL) { // Target MAC Unknown
		free(p->d);
		free(p);
		return 3; // Ensure the target Mac is known!
	}
	for (tLength = 0; tLength < 6; tLength++) {
		p->d[tLength] = mac[tLength]; // Destination
		p->d[6 + tLength] = ownMacAddress[tLength]; // Source
	}
	p->d[12] = (IPV4 & 0xFF00) >> 8;
	p->d[13] = (IPV4 & 0x00FF);
	if (macSendPacket(p)) {
		if (macSendPacket(p)) {
			return 2;
		}
	}
	return 0;
}

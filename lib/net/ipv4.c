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
// 1 for init, error and status messages
// 2 for ips of each packet.

#include <std.h>
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

uint16_t risingIdentification = 1;

// Transmission Buffer
Packet **ipv4Queue = NULL;
uint8_t ipv4PacketsInQueue = 0;

// ----------------------
// |    Internal API    |
// ----------------------

uint8_t extendTransmissionBuffer(void) {
	Packet **p;
	p = (Packet **)mrealloc(ipv4Queue, (ipv4PacketsInQueue + 1) * sizeof(Packet), ipv4PacketsInQueue * sizeof(Packet));
	if (p == NULL) {
		return MEM;
	}
	ipv4Queue = p;
	ipv4PacketsInQueue++;
	return OK;
}

void removeFromTransmissionBuffer(uint8_t pos) {
	// move data after pos onto pos,
	// reduce size of queue
	uint8_t i;
	Packet **p;

	// Free entry
	mfree(ipv4Queue[pos]->d, ipv4Queue[pos]->dLength);
	mfree(ipv4Queue[pos], sizeof(Packet));

	// Move following entries
	for (i = pos; i < ipv4PacketsInQueue - 1; i++) {
		ipv4Queue[i] = ipv4Queue[i + 1];
	}

	// Reduce array size
	p = (Packet **)mrealloc(ipv4Queue, (ipv4PacketsInQueue - 1) * sizeof(Packet), ipv4PacketsInQueue * sizeof(Packet));
	if (p != NULL) {
		ipv4Queue = p; // Should always happen...
	}

	ipv4PacketsInQueue--;
}

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
uint16_t checksum(uint8_t *addr, uint16_t count) {
	// C Implementation Example from RFC 1071, p. 7, slightly adapted
	// Compute Internet Checksum for count bytes beginning at addr
	uint32_t sum = 0;

	while (count > 1) {
		sum += (((uint16_t)(*addr++)) << 8);
		sum += (*addr++); // Reference Implementation assumes wrong endianness...
		count -= 2;
	}

	// Add left-over byte, if any
	if (count > 0)
		sum += (((uint16_t)(*addr++)) << 8);

	// Fold 32-bit sum to 16 bits
	while (sum >> 16) {
		sum = (sum & 0xFFFF) + (sum >> 16);
	}

	return ~sum;
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

	assert(p->dLength > (MACPreambleSize + IPv4PacketHeaderLength)); // Big enough
	assert(p->dLength < MaxPacketSize); // Not too big

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
		mfree(p->d, p->dLength);
		mfree(p, sizeof(Packet));
		return 2;
	} else {
		debugPrint("Valid IPv4 Packet!\n");
	}

	w = get16Bit(p->d, MACPreambleSize + IPv4PacketFlagsOffset);
	if (w & 0x1FFF) {
#if DEBUG >= 1
		debugPrint("Fragment Offset is ");
		debugPrint(hexToString(w & 0x1FFF));
		debugPrint("!\n");
#endif
		mfree(p->d, p->dLength);
		mfree(p, sizeof(Packet));
		return 2;
	}
	if (w & 0x2000) {
		// Part of a fragmented IPv4 Packet... No support for that
		debugPrint("More Fragments follow!\n");
		mfree(p->d, p->dLength);
		mfree(p, sizeof(Packet));
		return 2;
	}

	if (isBroadcastIp(p->d + MACPreambleSize + IPv4PacketDestinationOffset)) {
		debugPrint("IPv4 Broadcast Packet!\n");
	} else if (isEqualMem(ownIpAddress, p->d + MACPreambleSize + IPv4PacketDestinationOffset, 4)) {
		debugPrint("IPv4 Packet for us!\n");
	} else {
		debugPrint("IPv4 Packet not for us!\n");
		mfree(p->d, p->dLength);
		mfree(p, sizeof(Packet));
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
#endif
		mfree(p->d, p->dLength);
		mfree(p, sizeof(Packet));
		return 0;
	}

	mfree(p->d, p->dLength);
	mfree(p, sizeof(Packet));
	return 0;
}

uint8_t ipv4SendPacket(Packet *p, uint8_t *target, uint8_t protocol) {
	uint16_t tLength = p->dLength - MACPreambleSize;
	uint8_t *mac = NULL;

	// Prepare Header Data
	p->d[12] = (IPV4 & 0xFF00) >> 8;
	p->d[13] = (IPV4 & 0x00FF); // IPv4 Protocol
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
	p->d[MACPreambleSize + IPv4PacketProtocolOffset] = protocol;
	for (tLength = 0; tLength < 4; tLength++) {
		p->d[MACPreambleSize + IPv4PacketSourceOffset + tLength] = ownIpAddress[tLength];
		p->d[MACPreambleSize + IPv4PacketDestinationOffset + tLength] = target[tLength];
	}

#ifndef DISABLE_IPV4_CHECKSUM
	tLength = checksum(p->d + MACPreambleSize, IPv4PacketHeaderLength);
	p->d[MACPreambleSize + 10] = (tLength & 0xFF00) >> 8;
	p->d[MACPreambleSize + 11] = (tLength & 0x00FF);
#endif

	// Aquire MAC
	mac = arpGetMacFromIp(target);
	if (mac != NULL) { // Target MAC known
		// Insert MACs
		for (tLength = 0; tLength < 6; tLength++) {
			p->d[tLength] = mac[tLength]; // Destination
			p->d[6 + tLength] = ownMacAddress[tLength]; // Source
		}

		// Try to send packet...
		tLength = macSendPacket(p);
		if (tLength) {
			// Could not send, so put into buffer to try again later...
			debugPrint("Moved Packet into IPv4 Transmit Buffer (");
			debugPrint(timeToString(tLength));
			debugPrint(")\n");

			if (extendTransmissionBuffer() != OK) {
				mfree(p->d, p->dLength);
				mfree(p, sizeof(Packet));
				return 1;
			}
			ipv4Queue[ipv4PacketsInQueue - 1] = p;
			return 0;
		}
		mfree(p->d, p->dLength);
		mfree(p, sizeof(Packet));
		return 0;
	} else {
		// MAC Unknown, insert packet into queue
		debugPrint("MAC Unknown. Moved Packet into IPv4 Transmit Buffer.\n");

		if (extendTransmissionBuffer() != OK) {
			debugPrint("Could not extend Transmit Buffer!\n");
			mfree(p->d, p->dLength);
			mfree(p, sizeof(Packet));
			return 1;
		}
		ipv4Queue[ipv4PacketsInQueue - 1] = p;
		return 0;
	}
}

void ipv4SendQueue(void) {
	uint8_t i, pos;
	uint8_t *mac;

	pos = ipv4PacketsToSend();
	if (pos == 0) {
		return;
	}
	pos -= 1; // Now pos has position of next ready packet in queue

	debugPrint("Working on IPv4 Send Queue...\n");

	mac = arpGetMacFromIp(ipv4Queue[pos]->d + MACPreambleSize + IPv4PacketDestinationOffset);

	// Send Packet, Insert MACs
	for (i = 0; i < 6; i++) {
		ipv4Queue[pos]->d[i] = mac[i]; // Destination
		ipv4Queue[pos]->d[6 + i] = ownMacAddress[i]; // Source
	}
	ipv4Queue[pos]->d[12] = (IPV4 & 0xFF00) >> 8;
	ipv4Queue[pos]->d[13] = (IPV4 & 0x00FF); // IPv4 Protocol

	// Try to send packet...
	if (macSendPacket(ipv4Queue[pos]) == 0) {
		removeFromTransmissionBuffer(pos);
	}
}

uint8_t ipv4PacketsToSend(void) {
	// Check if we got the MAC Address in the ARP Cache first...
	// Returns != 0 if we found a packet
	// (return - 1) is position in queue of packet to send next
	uint8_t i;
	uint8_t *mac;

	if (ipv4Queue == NULL) {
		return 0;
	}

	for (i = 0; i < ipv4PacketsInQueue; i++) {
		mac = arpGetMacFromIp(ipv4Queue[i]->d + MACPreambleSize + IPv4PacketDestinationOffset);
		if (mac != NULL) {
			// MAC is now available
			return i + 1;
		}
	}

	return 0;
}

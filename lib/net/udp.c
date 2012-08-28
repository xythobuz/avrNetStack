/*
 * udp.c
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

#define DEBUG 1

#include <net/icmp.h>
#include <net/utils.h>
#include <net/udp.h>
#include <net/controller.h>

#ifndef DISABLE_UDP

uint8_t isBroadcastIp(uint8_t *d);

typedef struct {
	uint16_t port;
	uint8_t (*func)(Packet *);
} UdpHandler;

UdpHandler *handlers = NULL;
uint16_t registeredHandlers = 0;
IPv4Address target;

// --------------------------
// |      Internal API      |
// --------------------------

uint16_t checksum(uint8_t *rawData, uint16_t l); // From ipv4.c

int16_t findHandler(uint16_t port) {
	uint16_t i;
	if (handlers != NULL) {
		for (i = 0; i < registeredHandlers; i++) {
			if (handlers[i].port == port) {
				return i;
			}
		}
	}
	return -1;
}

#ifndef DISABLE_UDP_CHECKSUM
uint16_t udpChecksum(Packet *p) {
	uint8_t i;
	uint16_t cs;
	// We create the pseudo header in our already present buffer,
	// calculate the checksum,
	// then move the ip addresses back to their old places, so it can be used again...
	
	// Move IPs (8 bytes) from IPv4PacketSourceOffset (12) to 8
	for (i = 0; i < 8; i++) {
		p->d[MACPreambleSize + 8 + i] = p->d[MACPreambleSize + 12 + i];
	}

	// Insert data for Pseudo Header
	p->d[MACPreambleSize + 16] = 0x00; // Zeros
	p->d[MACPreambleSize + 17] = 0x11; // UDP Protocol
	p->d[MACPreambleSize + 18] = p->d[UDPOffset + UDPLengthOffset];
	p->d[MACPreambleSize + 19] = p->d[UDPOffset + UDPLengthOffset + 1]; // UDP Length

	// Clear UDP Checksum field
	p->d[UDPOffset + UDPChecksumOffset] = 0;
	p->d[UDPOffset + UDPChecksumOffset + 1] = 0;

#if DEBUG >= 2
	debugPrint("\nChecksum data size: ");
	debugPrint(timeToString(12 + get16Bit(p->d, UDPOffset + UDPLengthOffset)));
	debugPrint(" bytes.\nHeader:\n");
	for (i = 0; i < 12; i++) {
		if (i < 8) {
			debugPrint(timeToString((p->d + MACPreambleSize + 8)[i]));
		} else {
			debugPrint(hex2ToString((p->d + MACPreambleSize + 8)[i]));
		}
		if (i < 11) {
			debugPrint(" ");
		}
	}
	debugPrint("\n");
	for (i = 0; i < 8; i++) {
		debugPrint(hex2ToString((p->d + MACPreambleSize + 20)[i]));
		if (i < 7) {
			debugPrint(" ");
		}
	}
	debugPrint("\n\n");
#endif

	// Calculate Checksum
	cs = checksum(p->d + MACPreambleSize + 8, 12 + get16Bit(p->d, UDPOffset + UDPLengthOffset));

	// Move IPs back
	for (i = 0; i < 8; i++) {
		p->d[MACPreambleSize + 12 + i] = p->d[MACPreambleSize + 8 + i];
	}
	// Restore IPv4 Protocol and Checksum
	p->d[MACPreambleSize + IPv4PacketProtocolOffset] = 0x11; // UDP
	p->d[MACPreambleSize + IPv4PacketProtocolOffset + 1] = 0;
	p->d[MACPreambleSize + IPv4PacketProtocolOffset + 2] = 0; // Checksum field

	// Haha. I tested this with Dropbox' LAN-Sync UDP Broadcasts.
	// Interestingly, this seems to be a Dropbox Bug...
	// return cs - 0x10; // Please, don't ask me why this works. I have no idea...
	return cs;
}
#endif

// --------------------------
// |      External API      |
// --------------------------

void udpInit(void) {}

// 0 on success, 1 not enough mem, 2 invalid
uint8_t udpHandlePacket(Packet *p) {
	uint8_t i;
	uint16_t ocs = 0x0000, cs = 0x0000;
#ifndef DISABLE_UDP_CHECKSUM
	ocs = get16Bit(p->d, UDPOffset + UDPChecksumOffset);
	cs = udpChecksum(p);
#endif
	if (cs != ocs) {
#if DEBUG >= 1
		debugPrint("UDP Checksum invalid: ");
		debugPrint(hexToString(cs));
		debugPrint(" != ");
		debugPrint(hexToString(ocs));
		debugPrint("\n");
#endif
		free(p->d);
		free(p);
		return 2;
	}

	// Look for a handler
	for (i = 0; i < registeredHandlers; i++) {
		if (handlers[i].port == get16Bit(p->d, UDPOffset + UDPDestinationOffset)) {
			// found handler
			return handlers[i].func(p);
		}
	}

	debugPrint("UDP: No handler for ");
	debugPrint(timeToString(get16Bit(p->d, UDPOffset + UDPDestinationOffset)));
	debugPrint("\n");
	free(p->d);
	free(p);
	return 0;
}


// Overwrites existing handler for this port
// 0 on succes, 1 on not enough RAM
uint8_t udpRegisterHandler(uint8_t (*handler)(Packet *), uint16_t port) {
	uint16_t i;

	// Check if port is already in list
	for (i = 0; i < registeredHandlers; i++) {
		if (handlers[i].port == port) {
			handlers[i].func = handler;
			return 0;
		}
	}

	// Extend list, add new handler.
	UdpHandler *tmp = (UdpHandler *)realloc(handlers, (registeredHandlers + 1) * sizeof(UdpHandler));
	if (tmp == NULL) {
		return 1;
	}
	handlers = tmp;
	handlers[registeredHandlers].port = port;
	handlers[registeredHandlers].func = handler;
	registeredHandlers++;
	return 0;
}

#endif // DISABLE_UDP

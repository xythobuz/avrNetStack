/*
 * udp.h
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
#ifndef _udp_h
#define _udp_h

#include <net/mac.h>
#include <net/ipv4.h>
#include <net/controller.h>

/* typedef struct {
	uint16_t source;
	uint16_t destination;
	uint16_t length; // Minimum Headerlength (8)
	uint16_t checksum;
	uint8_t *data;
	uint16_t dLength;
} UdpPacket; */

#define UDPOffset (MACPreambleSize + IPv4PacketHeaderLength)
#define UDPSourceOffset 0
#define UDPDestinationOffset 2
#define UDPLengthOffset 4
#define UDPChecksumOffset 6
#define UDPDataOffset 8

void udpInit(void);

// 0 on success, 1 if not enough mem, 2 invalid
uint8_t udpHandlePacket(Packet *p);

// 0 on success, 1 if destination unknown, try again later.
// 2 or 4 if there was not enough RAM. 3 on PHY Error
// On Return 0, 1, 2 and 3, up was already freed
// uint8_t udpSendPacket(UdpPacket *up, IPv4Address target); // Leave checksum 0x0000

// Overwrites existing handler for this port
// 0 on succes, 1 on not enough RAM
// Handler has to free the UdpPacket!
uint8_t udpRegisterHandler(uint8_t (*handler)(Packet *), uint16_t port);

uint16_t udpLastPort(void);

#endif

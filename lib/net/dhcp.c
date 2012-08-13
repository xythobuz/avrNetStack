/*
 * dhcp.c
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

#include <net/udp.h>
#include <time.h>
#include <net/dhcp.h>

// 0 on success, 1 no mem, 2 invalid
uint8_t dhcpHandler(UdpPacket *up) {
	return 0;
}

#define DHCPHeaderLength 0xF0

uint8_t dhcpIssueRequest(void) {
	IPv4Address dest = {255, 255, 255, 255};
	UdpPacket *up = (UdpPacket *)malloc(sizeof(UdpPacket));
	time_t time = getSystemTime();
	uint8_t i;
	if (up == NULL) {
		return 1;
	}
	/*
	 * Option 53: DHCP Message Type, 1byte, DHCP Discover, 0x01
	 */
	up->dLength = DHCPHeaderLength + 2;
	up->source = 68;
	up->destination = 67;
	up->data = (uint8_t *)malloc(up->dLength * sizeof(uint8_t));
	if (up->data == NULL) {
		free(up);
		return 1;
	}
	// Build DHCPDISCOVER Message
	up->data[0] = 0x01; // OP
	up->data[1] = 0x01; // HTYPE
	up->data[2] = 0x06; // HLEN
	up->data[3] = 0x00; // HOPS
	up->data[8] = (time & 0xFF00) >> 8;
	up->data[9] = (time & 0x00FF); // SECS
	up->data[10] = 0x00;
	up->data[11] = 0x00; // FLAGS
	for (i = 0; i < 4; i++) {
		up->data[4 + i] = 0x00; // XID
		up->data[12 + i] = 0x00; // CIADDR
		up->data[16 + i] = 0x00; // YIADDR
		up->data[20 + i] = 0x00; // SIADDR
		up->data[24 + i] = 0x00; // GIADDR
		up->data[36 + i] = 0x00; // CHADDR?
		up->data[40 + i] = 0x00; // CHADDR?
	}
	for (i = 0; i < 6; i++) {
		up->data[28 + i] = ownMacAddress[i];
	}
	for (i = 0; i < 192; i++) {
		// BOOTP legacy
		up->data[44 + i] = 0x00;
	}
	up->data[236] = 0x63;
	up->data[237] = 0x82;
	up->data[238] = 0x53;
	up->data[239] = 0x63; // Magic Cookie
	up->data[240] = 53; // DHCP Message Type
	up->data[241] = 0x01; // "DHCP Discover"

	i = udpSendPacket(up, dest);
	if ((i != 0) && (i != 1) && (i != 2) && (i != 3)) {
		i = udpSendPacket(up, dest);
		if ((i != 0) && (i != 1) && (i != 2) && (i != 3)) {
			free(up->data);
			free(up);
		}
		return i;
	}
	return i;
}

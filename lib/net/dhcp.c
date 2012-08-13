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

#define DHCPHeader1Length 0xF0
#define DHCPHeader2Length 0xE8

uint8_t isEqualMem(uint8_t *d1, uint8_t *d2, uint8_t l);
void freeUdpPacket(UdpPacket *up);

IPv4Address dest = {255, 255, 255, 255};
IPv4Address server = {0, 0, 0, 0};
IPv4Address self = {0, 0, 0, 0};
uint8_t acked = 0;

// 0 on success, 1 no mem, 2 invalid
uint8_t dhcpHandler(UdpPacket *up) {
	uint8_t i;
	UdpPacket *p;
	time_t time = getSystemTime() / 1000;
	if ((up->data[0] == 0x02) && (up->data[1] == 0x01) && (up->data[2] == 0x06) && (up->data[3] ==0x00)) {
		if (isEqualMem((up->data + 20), ownMacAddress, 6)) {
			// Offer for us
			if (acked == 1) {
				 // Thats the DHCPACK, we can use the lease
				if (up->dLength > 240) {
					// We got out IP in self, the Server IP in server
					// Looking for subnet mask and gateway
					if (up->data[242] == 0x01) {
						for (i = 0; i < 4; i++) {
							subnetmask[i] = up->data[243 + i];
						}
					} else if (up->data[247] == 0x01) {
						for (i = 0; i < 4; i++) {
							subnetmask[i] = up->data[248 + i];
						}
					}
					if (up->data[242] == 0x03) {
						for (i = 0; i < 4; i++) {
							defaultGateway[i] = up->data[243 + i];
						}
					} else if (up->data[247] == 0x03) {
						for (i = 0; i < 4; i++) {
							defaultGateway[i] = up->data[248 + i];
						}
					}
					for (i = 0; i < 4; i++) {
						ownIpAddress[i] = self[i];
					}
				}
				freeUdpPacket(up);
				return 0;
			}
			for (i = 0; i < 4; i++) {
				server[i] = up->data[12 + i];
				self[i] = up->data[8 + i];
			}
			p = (UdpPacket *)malloc(sizeof(UdpPacket));
			if (p == NULL) {
				freeUdpPacket(up);
				return 1;
			}
			p->source = 68;
			p->destination = 67;
			/*
			 * Option 53: DHCP Message Type, 1byte, DHCP Request, 0x03
			 * Option 50: Requested IP, 4byte
			 * Option 54: Server IP, 4byte
			 */
			p->dLength = DHCPHeader1Length + 12;
			p->data = (uint8_t *)malloc(p->dLength * sizeof(uint8_t));
			if (p->data == NULL) {
				freeUdpPacket(up);
				free(p);
				return 1;
			}
			p->data[0] = 0x01; // OP
			p->data[1] = 0x01; // HTYPE
			p->data[2] = 0x06; // HLEN
			p->data[3] = 0x00; // HOPS
			p->data[8] = (time & 0xFF00) >> 8;
			p->data[9] = (time & 0x00FF);
			p->data[10] = 0x00;
			p->data[11] = 0x00; // FLAGS
			for (i = 0; i < 4; i++) {
				p->data[4 + i] = 0x42; // XID
				p->data[12 + i] = 0x00; //CIADDR
				p->data[16 + i] = 0x00; // YIADDR
				p->data[20 + i] = server[i]; // SIADDR
				p->data[24 + i] = 0x00; // GIADDR
				p->data[40 + i] = 0x00; // CHADDR...
				p->data[243 + i] = self[i];
				p->data[248 + i] = server[i];
			}
			for (i = 0; i < 6; i++) {
				p->data[28 + i] = ownMacAddress[i];
				p->data[36 + i] = 0x00; // CHADDR...
			}
			for (i = 0; i < 192; i++) {
				p->data[44 + i] = 0x00; // BOOTP legacy
			}
			p->data[236] = 0x63;
			p->data[237] = 0x82;
			p->data[238] = 0x53;
			p->data[239] = 0x63; // Magic Cookie
			p->data[240] = 53; // DHCP Option 53
			p->data[241] = 0x03; // DHCP Request
			p->data[242] = 50; // DHCP Requested IP
			p->data[247] = 54; // DCHP Server IP
			acked++;
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
	}
	freeUdpPacket(up);
	return 0;
}

uint8_t dhcpIssueRequest(void) {
	UdpPacket *up = (UdpPacket *)malloc(sizeof(UdpPacket));
	time_t time = getSystemTime() / 1000;
	uint8_t i;
	if (up == NULL) {
		return 1;
	}
	/*
	 * Option 53: DHCP Message Type, 1byte, DHCP Discover, 0x01
	 * Option 55: Parameter Request List, 1+byte, 0x01, 0x03 (Subnet, Router)
	 */
	up->dLength = DHCPHeader1Length + 2;
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
		up->data[4 + i] = 0x42; // XID
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

/*
 * ipv4.c
 *
 * Copyright 2012 Thomas Buck <xythobuz@me.com>
 *
 * This file is part of avrNetStack.
 *
 * avrNetStack is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * avrNetStack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with avrNetStack.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <avr/io.h>
#include <stdint.h>
#include <stdlib.h>

#include <time.h>
#include <net/mac.h>
#include <net/arp.h>
#include <net/controller.h>
#include <net/ipv4.h>

IPv4Address ownIpAddress;
IPv4Address subnetmask;
IPv4Address defaultGateway;

// ----------------------
// |    Internal API    |
// ----------------------

IPv4Packet *macPacketToIpPacket(MacPacket *p) {
	return NULL;
}

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
}

void ipv4ProcessPacket(MacPacket *p) {

}

// Returns 0 if packet was sent. 1 if destination was unknown.
// Try again later, after ARP response could have arrived...
uint8_t ipv4SendPacket(IPv4Packet *p) {
	return 1;
}

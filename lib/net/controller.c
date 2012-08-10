/*
 * controller.c
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
#include <stdint.h>
#include <avr/io.h>
#include <stdlib.h>

#include <net/mac.h>
#include <net/arp.h>
#include <net/controller.h>



void networkInit(MacAddress a) {
	macInitialize(a);
	arpInit();
}

void networkHandler(void) {
	MacPacket *p;
	
	if (macLinkIsUp() && (macPacketsReceived() > 0)) {
		p = macGetPacket();
		if (p != NULL) {
			if (p->typeLength == IPV4) {
				// IPv4 Packet

			} else if (p->typeLength == ARP) {
				// Address Resolution Protocol Packet
				arpProcessPacket(p);
			} else if (p->typeLength == WOL) {
				// Wake on Lan Packet

			} else if (p->typeLength == RARP) {
				// Reverse Address Resolution Protocol Packet

			} else if (p->typeLength <= 0x0600) {
				// Ethernet type I packet. typeLength = Real data length

			} else if (p->typeLength == IPV6) {
				// IPv6 Packet

			} else {
				// Unknown Protocol

			}

			// Packet was handled, free it
			free(p->data);
			free(p);
		}
	}
}

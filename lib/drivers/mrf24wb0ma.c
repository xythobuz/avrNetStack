/*
 * mrf24wb0ma.c
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

#include <net/mac.h>

MacAddress ownMacAddress;

uint8_t macInitialize(MacAddress address) { // 0 if success, 1 on error
	uint8_t i;
	for (i = 0; i < 6; i++) {
		ownMacAddress[i] = address[i];
	}
	return 1;
}

void macReset(void) {

}

uint8_t macLinkIsUp(void) { // 0 if down, 1 if up
	return 0;
}

uint8_t macSendPacket(Packet *p) { // 0 on success, 1 on error
	return 1;
}

uint8_t macPacketsReceived(void) { // 0 if no packet, 1 if packet ready
	return 0;
}

Packet *macGetPacket(void) { // Returns NULL on error
	return NULL;
}

/*
 * utils.c
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
#include <avr/pgmspace.h>

#include <net/icmp.h>
#include <net/udp.h>

void freeUdpPacket(UdpPacket *up) {
	if (up != NULL) {
		if (up->data != NULL) {
			free(up->data);
		}
		free(up);
	}
}

void freeIPv4Packet(IPv4Packet *ip) {
	if (ip != NULL) {
		if (ip->options != NULL) {
			free(ip->options);
		}
		if (ip->data != NULL) {
			free(ip->data);
		}
		free(ip);
	}
}

void freeIcmpPacket(IcmpPacket *ic) {
	if (ic != NULL) {
		if (ic->data != NULL) {
			free(ic->data);
		}
		free(ic);
	}
}

uint8_t isEqual(uint8_t *d1, uint8_t *d2, uint8_t l) {
	uint8_t i;
	for (i = 0; i < l; i++) {
		if (d1[i] != pgm_read_byte(&(d2[i]))) {
			return 0;
		}
	}
	return 1;
}

uint8_t isEqualMem(uint8_t *d1, uint8_t *d2, uint8_t l) {
	uint8_t i;
	for (i = 0; i < l; i++) {
		if (d1[i] != d2[i]) {
			return 0;
		}
	}
	return 1;
}

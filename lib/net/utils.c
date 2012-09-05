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

#include <net/controller.h>
#include <net/mac.h>
#include <net/ipv4.h>

#define print(x) DEBUGOUT(x)

uint8_t isEqualFlash(uint8_t *d1, uint8_t *d2, uint8_t l) {
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

void dumpPacket(Packet *p) {
	uint8_t i;
	uint16_t tl;
	if (p == NULL) {
		print("Packet Struct is NULL!\n");
		return;
	}
	print("Packet Length: ");
	print(timeToString(p->dLength));
	print(" bytes.\n");
	if (p->dLength == 0) {
		return;
	}

	print("Source: ");
	for (i = 0; i < 6; i++) {
		print(hex2ToString(p->d[i + 6]));
		if (i < 5) {
			print(":");
		}
	}
	print("\nDestination: ");
	for (i = 0; i < 6; i++) {
		print(hex2ToString(p->d[i]));
		if (i < 5) {
			print(":");
		}
	}
	print("\nType/Length: ");
	tl = get16Bit(p->d, 12);
	print(hexToString(tl));
	print("\n");
	if (tl == IPV4) {
		print("IPv");
		print(timeToString((p->d[MACPreambleSize] & 0xF0) >> 4));
		if (((p->d[MACPreambleSize] & 0xF0) >> 4) != 4) {
			print(" not supported!\n");
			return;
		}
		print(", ");
		print(timeToString(4 * (p->d[MACPreambleSize] & 0x0F)));
		print(" bytes Header.\nTotal Length: ");
		print(timeToString(get16Bit(p->d, MACPreambleSize + 2)));
		print("\nSource: ");
		for (i = 0; i < 4; i++) {
			print(timeToString(p->d[MACPreambleSize + IPv4PacketSourceOffset]));
			if (i < 3) {
				print(".");
			}
		}
		print("\nDestination: ");
		for (i = 0; i < 4; i++) {
			print(timeToString(p->d[MACPreambleSize + IPv4PacketDestinationOffset]));
			if (i < 3) {
				print(".");
			}
		}
		print("\nProtocol: ");
		print(hexToString(p->d[MACPreambleSize + IPv4PacketProtocolOffset]));
		if (p->d[MACPreambleSize + IPv4PacketProtocolOffset] == ICMP) {
			print("ICMP\n");
		} else if (p->d[MACPreambleSize + IPv4PacketProtocolOffset] == TCP) {
			print("TCP\n");
		} else if (p->d[MACPreambleSize + IPv4PacketProtocolOffset] == UDP) {
			print("UDP\n");
		} else {
			print("Unknown!\n");
		}
	} else if (tl == ARP) {
		print("ARP ");
		if (get16Bit(p->d, MACPreambleSize) == 1) {
			print("Request");
		} else {
			print("Reply");
		}
		print("\nSource:");
		for (i = 0; i < 4; i++) {
			print(timeToString(p->d[MACPreambleSize + 8]));
			if (i < 3) {
				print(".");
			}
		}
		print("\nTarget:");
		for (i = 0; i < 4; i++) {
			print(timeToString(p->d[MACPreambleSize + 18]));
			if (i < 3) {
				print(".");
			}
		}
	} else {
		print("Unknown Type!\n");
	}
}

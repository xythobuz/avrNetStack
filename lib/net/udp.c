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

#include <net/udp.h>

typedef struct {
	uint16_t port;
	uint8_t (*func)(UdpPacket *);
} UdpHandler;

UdpHandler *handlers = NULL;
uint16_t registeredHandlers = 0;

// --------------------------
// |      Internal API      |
// --------------------------

void freeIPv4Packet(IPv4Packet *ip);

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

UdpPacket *ipv4PacketToUdpPacket(IPv4Packet *ip) {
	uint16_t i;
	UdpPacket *up = (UdpPacket *)malloc(sizeof(UdpPacket));
	if (up == NULL) {
		return NULL;
	}
	up->source = ((uint16_t)ip->data[0] << 8);
	up->source |= ip->data[1];
	up->destination = ((uint16_t)ip->data[2] << 8);
	up->destination |= ip->data[3];
	up->length = ((uint16_t)ip->data[4] << 8);
	up->length |= ip->data[5];
	up->checksum = ((uint16_t)ip->data[6] << 8);
	up->checksum |= ip->data[7];
	up->dLength = ip->dLength - 8;
	up->data = malloc(up->dLength * sizeof(uint8_t));
	if (up->data == NULL) {
		free(up);
		return NULL;
	}
	for (i = 0; i < up->dLength; i++) {
		up->data[i] = ip->data[i + 8];
	}
	return up;
}

#ifndef DISABLE_UDP_CHECKSUM
uint16_t checksum(uint8_t *rawData, uint16_t l);
uint16_t udpChecksum(UdpPacket *up, IPv4Packet *ip) {
	uint16_t i;
	uint8_t *pseudo = (uint8_t *)malloc((20 + up->dLength) * sizeof(uint8_t));
	if (pseudo == NULL) {
		return 0;
	}
	for (i = 0; i < 4; i++) {
		// Copy IPs
		pseudo[i] = ip->sourceIp[i];
		pseudo[4 + i] = ip->destinationIp[i];
	}
	pseudo[8] = 0;
	pseudo[9] = UDP;
	pseudo[10] = ((8 + up->dLength) & 0xFF00) >> 8;
	pseudo[11] = ((8 + up->dLength) & 0x00FF);
	pseudo[12] = (up->source & 0xFF00) >> 8;
	pseudo[13] = (up->source & 0x00FF);
	pseudo[14] = (up->destination & 0xFF00) >> 8;
	pseudo[15] = (up->destination & 0x00FF);
	pseudo[16] = (up->length & 0xFF00) >> 8;
	pseudo[17] = (up->length & 0x00FF);
	pseudo[18] = (up->checksum & 0xFF00) >> 8;
	pseudo[19] = (up->checksum & 0x00FF);
	for (i = 0; i < up->dLength; i++) {
		pseudo[20 + i] = up->data[i];
	}
	i = checksum(pseudo, 20 + up->dLength);
	if (i == 0x0000) {
		i = 0xFFFF;
	}
	free(pseudo);
	return i;
}
#endif

void freeUdpPacket(UdpPacket *up) {
	if (up != NULL) {
		if (up->data != NULL) {
			free(up->data);
		}
		free(up);
	}
}

// --------------------------
// |      External API      |
// --------------------------

void udpInit(void) {}

// 0 on success, 1 not enough mem, 2 invalid
uint8_t udpHandlePacket(IPv4Packet *ip) {
	UdpPacket *up = ipv4PacketToUdpPacket(ip);
	uint16_t cs;
#ifndef DISABLE_UDP_CHECKSUM
	cs = udpChecksum(up, ip);
#endif
	freeIPv4Packet(ip);
	if (up == NULL) {
		return 1;
	}
#ifndef DISABLE_UDP_CHECKSUM
	if (!((up->checksum == 0x00) || (cs == 0xFFFF) || (cs == up->checksum))) {
		// Checksum calculated from sender and found to be incorrect
		freeUdpPacket(up);
		return 2;
	}
#endif
	cs = findHandler(up->destination);
	if (cs != -1) {
		cs = handlers[cs].func(up);
		return cs;
	}
	// No handler registered
	freeUdpPacket(up);
	return 0;
}

uint8_t udpSendPacket(UdpPacket *up, IPv4Address target) {
	IPv4Packet *ip = (IPv4Packet *)malloc(sizeof(IPv4Packet));
	if (ip == NULL) {
		return 4;
	}

	return 0;
}

// Overwrites existing handler for this port
// 0 on succes, 1 on not enough RAM
uint8_t udpRegisterHandler(uint8_t (*handler)(UdpPacket *), uint16_t port) {
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

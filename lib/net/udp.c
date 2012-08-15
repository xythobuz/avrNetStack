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

#include <net/icmp.h>
#include <net/utils.h>
#include <net/udp.h>

#ifndef DISABLE_UDP

typedef struct {
	uint16_t port;
	uint8_t (*func)(UdpPacket *);
} UdpHandler;

UdpHandler *handlers = NULL;
uint16_t registeredHandlers = 0;
IPv4Address target;
extern IPv4Address broadcastIp;
// --------------------------
// |      Internal API      |
// --------------------------

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
	for (i = 0; i < 4; i++) {
		up->sourceIp[i] = ip->sourceIp[i];
		target[i] = ip->destinationIp[i];
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

// --------------------------
// |      External API      |
// --------------------------

void udpInit(void) {}

// 0 on success, 1 not enough mem, 2 invalid
// 3 on success, 5 or 7 on not enough mem, 6 on PHY error
// 4 on unknown destination, try again later
// ip is always freed!
uint8_t udpHandlePacket(IPv4Packet *ip) {
	UdpPacket *up = ipv4PacketToUdpPacket(ip);
	IcmpPacket *ic;
	uint16_t cs;
#ifndef DISABLE_UDP_CHECKSUM
	cs = udpChecksum(up, ip);
#endif
	free(ip->data);
	ip->data = NULL,
	ip->dLength = 0;
	if (up == NULL) {
		freeIPv4Packet(ip);
		return 1;
	}
#ifndef DISABLE_UDP_CHECKSUM
	if (!((up->checksum == 0x00) || (cs == 0xFFFF) || (cs == up->checksum))) {
		// Checksum calculated from sender and found to be incorrect
		freeUdpPacket(up);
		freeIPv4Packet(ip);
		return 2;
	}
#endif
	cs = findHandler(up->destination);
	if (cs != -1) {
		freeIPv4Packet(ip);
		cs = handlers[cs].func(up); // Frees up...
		return cs;
	}
	// No handler registered
#ifndef DISABLE_ICMP_UDP_MSG
	if (!isEqualMem(target, broadcastIp, 4)) {
		// Issue ICMP Port not reachable Message
		ic = (IcmpPacket *)malloc(sizeof(IcmpPacket));
		if (ic == NULL) {
			freeUdpPacket(up);
			freeIPv4Packet(ip);
			return 1;
		}
		ic->type = 3; // Destination not reachable
		ic->code = 3; // Port not reachable
		ic->checksum = 0;
		ic->restOfHeader = 0; // No Message Code 4...
		ic->data = (uint8_t *)malloc((ip->internetHeaderLength * 4) + 8);
		ic->data[0] |= (ip->internetHeaderLength = 0x0F);
		ic->data[1] = ip->typeOfService;
		ic->data[2] = (ip->totalLength & 0xFF00) >> 8;
		ic->data[3] = (ip->totalLength & 0x00FF);
		ic->data[4] = (ip->identification & 0xFF00) >> 8;
		ic->data[5] = (ip->identification & 0x00FF);
		ic->data[6] = (ip->flags & 0x07) | ((ip->fragmentOffset & 0x1F00) >> 5);
		ic->data[7] = (ip->fragmentOffset & 0x00FF);
		ic->data[8] = ip->timeToLive;
		ic->data[9] = ip->protocol;
		ic->data[10] = 0x00;
		ic->data[11] = 0x00; // Checksum is calculated later
		for (cs = 0; cs < 4; cs++) {
			ic->data[12 + cs] = ip->sourceIp[cs];
			ic->data[16 + cs] = ip->destinationIp[cs];
		}
		if (ip->options != NULL) {
			for (cs = 20; cs < (ip->internetHeaderLength * 4); cs++) {
				ic->data[cs] = ip->options[cs - 20];
			}
		}
		cs = (ip->internetHeaderLength * 4);
		ic->data[cs + 0] = (up->source & 0xFF00) >> 8;
		ic->data[cs + 1] = (up->source & 0x00FF);
		ic->data[cs + 2] = (up->destination & 0xFF00) >> 8;
		ic->data[cs + 3] = (up->destination & 0x00FF);
		ic->data[cs + 4] = (up->length & 0xFF00) >> 8;
		ic->data[cs + 5] = (up->length & 0x00FF);
		ic->data[cs + 6] = (up->checksum & 0xFF00) >> 8;
		ic->data[cs + 7] = (up->checksum & 0x00FF);
		freeIPv4Packet(ip);
		cs = icmpSendPacket(ic, up->sourceIp);
		if (!((cs == 0) || (cs == 1) || (cs == 2) || (cs == 3))) {
			// ic is still there
			cs = icmpSendPacket(ic, up->sourceIp);
			if (!((cs == 0) || (cs == 1) || (cs == 2) || (cs == 3))) {
				freeIcmpPacket(ic);
			}
			return 3 + cs;
		}
		return 3 + cs;
	}
#endif // DISABLE_ICMP_UDP_MSG
	freeUdpPacket(up);
	freeIPv4Packet(ip);
	return 0;
}

uint8_t udpSendPacket(UdpPacket *up, IPv4Address target) {
	uint16_t i;
	IPv4Packet *ip = (IPv4Packet *)malloc(sizeof(IPv4Packet));
	if (ip == NULL) {
		return 4;
	}
	ip->data = (uint8_t *)malloc((up->dLength + 8) * sizeof(uint8_t));
	if (ip->data == NULL) {
		free(ip);
		return 4;
	}
	// Fill out IP Header
	ip->version = 4;
	ip->internetHeaderLength = 5; // No options
	ip->typeOfService = 0x00; // Nothing fancy...
	ip->totalLength = 20 + ip->dLength;
	ip->flags = 0;
	ip->fragmentOffset = 0;
	ip->timeToLive = 0x80;
	ip->protocol = UDP;
	for (i = 0; i < 4; i++) {
		ip->sourceIp[i] = ownIpAddress[i];
		ip->destinationIp[i] = target[i];
	}
	ip->options = NULL;
	// Insert UDP Packet into IP Payload
	up->checksum = 0x00;
	up->checksum = udpChecksum(up, ip);
	ip->data[0] = (up->source & 0xFF00) >> 8;
	ip->data[1] = (up->source & 0x00FF);
	ip->data[2] = (up->destination & 0xFF00) >> 8;
	ip->data[3] = (up->destination & 0x00FF);
	ip->data[4] = (up->length & 0xFF00) >> 8;
	ip->data[5] = (up->length & 0x00FF);
	ip->data[6] = (up->checksum & 0xFF00) >> 8;
	ip->data[7] = (up->checksum & 0x00FF);
	for (i = 0; i < up->dLength; i++) {
		ip->data[8 + i] = up->data[i];
	}
	ip->dLength = up->dLength + 8;
	free(up->data);
	free(up);
	i = ipv4SendPacket(ip);
	if (!((i == 0) || (i == 3))) {
		i = ipv4SendPacket(ip);
		if (!((i == 0) || (i == 3))) {
			freeIPv4Packet(ip);
			return i;
		}
	}
	return i;
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

#endif // DISABLE_UDP

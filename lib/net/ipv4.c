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
	uint8_t i, l;
	uint16_t j, realLength;
	IPv4Packet *ip = (IPv4Packet *)malloc(sizeof(IPv4Packet));
	if (ip == NULL) {
		return NULL;
	}
	ip->version = (p->data[0] & 0xF0) >> 4;
	ip->internetHeaderLength = p->data[0] & 0x0F;
	ip->typeOfService = p->data[1];
	ip->totalLength = p->data[3];
	ip->totalLength |= (p->data[2] << 8);
	ip->identification = p->data[5];
	ip->identification |= (p->data[4] << 8);
	ip->flags = (p->data[6] & 0xE0) >> 5;
	ip->fragmentOffset = p->data[7];
	ip->fragmentOffset |= (p->data[6] & 0x1F) << 8;
	ip->timeToLive = p->data[8];
	ip->protocol = p->data[9];
	ip->headerChecksum = p->data[11];
	ip->headerChecksum |= (p->data[10] << 8);
	for (i = 0; i < 4; i++) {
		ip->sourceIp[i] = p->data[12 + i];
		ip->destinationIp[i] = p->data[16 + i];
	}
	l = ip->internetHeaderLength - 5;
	if (l > 5) {
		ip->options = (uint8_t *)malloc(l * 4 * sizeof(uint8_t));
		if (ip->options == NULL) {
			free(ip);
			return NULL;
		}
		for (i = 0; i < l; i++) {
			ip->options[(i * 4) + 0] = p->data[20 + (i * 4)];
			ip->options[(i * 4) + 1] = p->data[21 + (i * 4)];
			ip->options[(i * 4) + 2] = p->data[22 + (i * 4)];
			ip->options[(i * 4) + 3] = p->data[23 + (i * 4)];
		}
	} else {
		ip->options = NULL;
	}
	realLength = ip->totalLength - (ip->internetHeaderLength * 4);
	ip->data = (uint8_t *)malloc(realLength * sizeof(uint8_t));
	if (ip->data == NULL) {
		if (ip->options != NULL) {
			free(ip->options);
		}
		free(ip);
		return NULL;
	}
	for (j = 0; j < realLength; j++) {
		ip->data[j] = p->data[(ip->internetHeaderLength * 4) + j];
	}
	ip->dLength = realLength;
	return ip;
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
	IPv4Packet *ip = macPacketToIpPacket(p);
	free(p->data);
	free(p);
	if (ip == NULL) {
		return; // Not enough memory. Can't process packet!
	}

	// Process IPv4 Packet
	

	if (ip->options != NULL) {
		free(ip->options);
	}
	if (ip->data != NULL) {
		free(ip->data);
	}
	free(ip);
}

// Returns 0 if packet was sent. 1 if destination was unknown.
// Try again later, after ARP response could have arrived...
uint8_t ipv4SendPacket(IPv4Packet *p) {
	return 1;
}

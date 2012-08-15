/*
 * dns.h
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
#ifndef _dns_h
#define _dns_h

#include <net/ipv4.h>
#include <time.h>
#include <net/controller.h>

// #define MAXDNSCACHESIZE 5 // Doesn't need to be defined

typedef struct DnsTableEntry DnsTableEntry;

struct DnsTableEntry {
	IPv4Address ip;
	uint8_t *name;
#ifdef MAXDNSCACHESIZE
	time_t time;
	uint32_t ttl; // Time To Live in seconds
#endif
	DnsTableEntry *next;
};

extern DnsTableEntry *dnsTable;

typedef struct {
	uint8_t *name; // 3www8xythobuz3org0
	uint16_t qType;
	uint16_t qClass;
} DnsQuestion;

typedef struct {
	uint8_t *name;
	uint16_t type;
	uint16_t class;
	uint32_t ttl;
	uint16_t rlength;
	uint8_t *rdata;
} DnsRecord;

typedef struct {
	uint16_t id;

	// From MSB to LSB:
	// QR(1bit), Opcode(4bit), AA(1),
	// TC(1), RD(1), RA(1), Z(3), RCODE(4)
	uint16_t flags;

	uint16_t qdCount;
	uint16_t anCount;
	uint16_t nsCount;
	uint16_t arCount;
} DnsPacket;

void dnsRegisterMessageCallback(void (*debugOutput)(char *));

uint8_t dnsHandler(UdpPacket *up);

// Returns 0 on success, 1 on no mem, 2 on invalid
uint8_t dnsGetIp(uint8_t *domain, IPv4Address ip);

#endif

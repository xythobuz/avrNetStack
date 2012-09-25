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
	time_t time;
	uint32_t ttl; // Time To Live in seconds
	DnsTableEntry *next;
};

extern DnsTableEntry *dnsTable;

void dnsRegisterMessageCallback(void (*debugOutput)(char *));

uint8_t dnsHandler(Packet *p);

// Returns 0 on success, 1 on no mem, 2 on invalid
uint8_t dnsGetIp(uint8_t *domain, IPv4Address ip);

#endif

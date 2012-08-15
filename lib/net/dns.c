/*
 * dns.c
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

#include <net/ipv4.h>
#include <net/udp.h>
#include <time.h>
#include <net/controller.h>
#include <net/utils.h>
#include <net/dns.h>

#ifndef DISABLE_DNS

DnsTableEntry *dnsTable = NULL;

// --------------------------
// |      Internal API      |
// --------------------------

#ifdef MAXDNSCACHESIZE

uint16_t dnsCacheSize(void) {
	uint16_t i = 0;
	DnsTableEntry *d = dnsTable;
	while (d != NULL) {
		i++;
		d = d->next;
	}
	return i;
}

#endif // MAXDNSCACHESIZE

// --------------------------
// |      External API      |
// --------------------------

uint8_t dnsHandler(UdpPacket *up) {
	return 0;
}

// Returns 0 on success, 1 on no mem, 2 on invalid
uint8_t dnsGetIp(char *domain, IPv4Address ip) {
	return 0;
}

#endif // DISABLE_DNS

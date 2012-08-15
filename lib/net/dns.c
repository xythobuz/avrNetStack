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
#include <string.h>

#include <net/ipv4.h>
#include <net/udp.h>
#include <time.h>
#include <net/controller.h>
#include <net/utils.h>
#include <net/dns.h>

#ifndef DISABLE_DNS

DnsTableEntry *dnsTable = NULL;
uint16_t dnsNextId = 0;

// --------------------------
// |      Internal API      |
// --------------------------
void (*debugOutHandler)(char *) = NULL;
#define print(x) if (debugOutHandler != NULL) debugOutHandler(x)

#define DNSHEADERSIZE 12

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

time_t dnsOldestEntry(void) { // Returns UINT64_MAX if no entries...
	DnsTableEntry *d = dnsTable;
	time_t min = UINT64_MAX;
	while (d != NULL) {
		if (d->time < min) {
			min = d->time;
		}
		d = d->next;
	}
	return min;
}

#endif // MAXDNSCACHESIZE

// Returns new offset for data. Size of data only if dq is NULL
uint8_t toDnsQuestion(uint8_t *data, uint8_t offset, DnsQuestion *dq) {
	uint16_t i = offset, max;
	while (data[i] != 0) {
		i++;
	}
	max = i + 1; // Thats the size of our name String
	if (dq != NULL) {
		dq->name = (uint8_t *)malloc((max - offset) * sizeof(uint8_t));
		if (dq->name == NULL) {
			return 0;
		}
		for (i = offset; i < max; i++) {
			dq->name[i - offset] = data[i];
		}
		dq->qType = data[max] << 8;
		dq->qType |= data[max + 1];
		dq->qClass = data[max + 2] << 8;
		dq->qClass |= data[max + 3];
	}
	return max + 4;
}

void toDnsPacket(uint8_t *data, DnsPacket *dp) {
	dp->id = data[0] << 8;
	dp->id |= data[1];
	dp->flags = data[2] << 8;
	dp->flags |= data[3];
	dp->qdCount = data[4] << 8;
	dp->qdCount |= data[5];
	dp->anCount = data[6] << 8;
	dp->anCount |= data[7];
	dp->nsCount = data[8] << 8;
	dp->nsCount |= data[9];
	dp->arCount = data[10] << 8;
	dp->arCount |= data[11];
}

uint8_t toDnsRecord(uint8_t *data, uint8_t off, DnsRecord *dr) {
	uint16_t i = off, max, rlength;
	while(data[i] != 0) {
		i++;
	}
	max = i + 1;
	rlength = data[max + 8] << 8;
	rlength |= data[max + 9];
	if (dr != NULL) {
		dr->name = (uint8_t *)malloc((max - off) * sizeof(uint8_t));
		if (dr->name == NULL) {
			return 0;
		}
		for (i = off; i < max; i++) {
			dr->name[i - off] = data[i];
		}

		dr->type = data[max] << 8;
		dr->type |= data[max + 1];
		dr->class = data[max + 2] << 8;
		dr->class |= data[max + 3];
		dr->ttl = (uint32_t)data[max + 4] << 24;
		dr->ttl |= (uint32_t)data[max + 5] << 16;
		dr->ttl |= (uint32_t)data[max + 6] << 8;
		dr->ttl |= (uint32_t)data[max + 7];
		dr->rlength = rlength;
		dr->rdata = (uint8_t *)malloc(dr->rlength * sizeof(uint8_t));
		if (dr->rdata == NULL) {
			free(dr->name);
			return 0;
		}
		for (i = (max + 10); i < (max + 10 + dr->rlength); i++) {
			dr->rdata[i - (max + 10)] = data[i];
		}
	}
	return max + 10 + rlength;
}

// --------------------------
// |      External API      |
// --------------------------

void dnsRegisterMessageCallback(void (*debugOutput)(char *)) {
	debugOutHandler = debugOutput;
}

uint8_t dnsHandler(UdpPacket *up) {
	DnsPacket *dp = (DnsPacket *)malloc(sizeof(DnsPacket));
	DnsQuestion *dq = (DnsQuestion *)malloc(sizeof(DnsQuestion));
	DnsRecord *dr = (DnsRecord *)malloc(sizeof(DnsRecord));
	DnsTableEntry *d = dnsTable;
	uint8_t off = DNSHEADERSIZE;
	uint16_t i;

	if ((dp == NULL) || (dq == NULL) || (dr == NULL)) {
		free(dp);
		free(dq);
		free(dr);
		freeUdpPacket(up);
		return 1;
	}
	toDnsPacket(up->data, dp);

#ifndef DISABLE_DNS_STRINGS
	switch (dp->flags & 0x0F) {
		case 1:
			print("DNS Error: Format Error\n");
			free(dp);
			freeUdpPacket(up);
			free(dq);
			free(dr);
			return 0;
		case 2:
			print("DNS Error: Server Failure\n");
			free(dp);
			freeUdpPacket(up);
			free(dq);
			free(dr);
			return 0;
		case 3:
			print("DNS Error: Name Error\n");
			free(dp);
			freeUdpPacket(up);
			free(dq);
			free(dr);
			return 0;
		case 4:
			print("DNS Error: Not Implemented\n");
			free(dp);
			freeUdpPacket(up);
			free(dq);
			free(dr);
			return 0;
		case 5:
			print("DNS Error: Refused\n");
			free(dp);
			freeUdpPacket(up);
			free(dq);
			free(dr);
			return 0;
		case 0:
			break; // No Error
		default:
			print("DNS: Reserved Error Code!\n");
			free(dp);
			freeUdpPacket(up);
			free(dq);
			free(dr);
			return 0;
	}
#endif // DISABLE_DNS_STRINGS

	if ((!(dp->flags & 0x8000)) || (dp->flags & 0x78)) {
		// This is a query, not a Response!
		// Or not a "standard query"
#ifndef DISABLE_DNS_STRINGS
		print("DNS: Invalid packet\n");
#endif
		free(dp);
		freeUdpPacket(up);
		free(dq);
		free(dr);
		return 0;
	}

	for (i = 0; i < dp->qdCount; i++) {
		// Go through questions
		if (i == 0) {
			off = toDnsQuestion(up->data, off, dq);
		} else {
			off = toDnsQuestion(up->data, off, NULL); // We only want the first question
		}
		if (off == 0) {
			freeDnsQuestion(dq);
			free(dp);
			freeUdpPacket(up);
			return 1;
		}
	}
	for (i = 0; i < dp->anCount; i++) {
		// Go through answers
		if (i == 0) {
			off = toDnsRecord(up->data, off, dr);
		} else {
			off = toDnsRecord(up->data, off, NULL); // We only want the first answer
		}
	}

	// We don't care for "Authority" or "Additional"
	free(up->data);
	up->data = NULL;

	// Have we sent this query
	while (d != NULL) {
		if (strlen((char *)d->name) == strlen((char *)dq->name)) {
			if (isEqualMem(d->name, dq->name, strlen((char *)d->name))) {
				// Yes, it is from us. Insert IP
				for (i = 0; i < 4; i++) {
					
				}
				break;
			}
		}
		d = d->next;
	}

	freeDnsRecord(dr);
	freeDnsQuestion(dq);
	free(dp);
	freeUdpPacket(up);
	return 0;
}

// Returns 0 on success, 1 on no mem, 2 on invalid
uint8_t dnsGetIp(uint8_t *domain, IPv4Address ip) {
	return 0;
}

#endif // DISABLE_DNS

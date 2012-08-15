/*
 * ntp.c
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
#include <net/ntp.h>
#include <net/dns.h>
#include <net/utils.h>
#include <time.h>
#include <net/controller.h>

#ifndef DISABLE_NTP

#define NTPMessageSize 48
#define NTPFirstByte 0x0B // Version 1, Mode 3 (Client)

#ifndef DISABLE_DNS
uint8_t ntpServerDomain[] = "0.de.pool.ntp.org";
#endif

IPv4Address ntpServer = {78, 46, 85, 230};

uint8_t ntpHandler(UdpPacket *up) {
	// 64bit NTP Timestamp in data[16] to data[23]
	time_t stamp = 0;
	stamp |= (time_t)up->data[16] << 56;
	stamp |= (time_t)up->data[17] << 48;
	stamp |= (time_t)up->data[18] << 40;
	stamp |= (time_t)up->data[19] << 32;
	stamp |= (time_t)up->data[20] << 24;
	stamp |= (time_t)up->data[21] << 16;
	stamp |= (time_t)up->data[22] << 8;
	stamp |= (time_t)up->data[23];
	setNtpTimestamp(stamp);
	freeUdpPacket(up);
	return 0;
}

// 0 on success, 1 if destination unknown, try again later.
// 2 or 4 if there was not enough RAM. 3 on PHY Error
// On Return 0, 1, 2 and 3, up was already freed

// 0 on success, 1 on no mem, 2 on error
uint8_t ntpIssueRequest(void) {
	UdpPacket *up;
	uint8_t *np = (uint8_t *)malloc(NTPMessageSize * sizeof(uint8_t));
	uint8_t i;
	if (np == NULL) {
		return 1;
	}

#ifndef DISABLE_DNS
	dnsGetIp(ntpServerDomain, ntpServer); // If it doesn't work, we rely on the defaults
#endif

	np[0] = NTPFirstByte;
	for (i = 1; i < NTPMessageSize; i++) {
		np[i] = 0x00; // Yes, SNTP is simple...
	}

	up = (UdpPacket *)malloc(sizeof(UdpPacket));
	if (up == NULL) {
		free(np);
		return 1;
	}
	up->data = np;
	up->dLength = NTPMessageSize;
	up->length = 8 + NTPMessageSize;
	up->checksum = 0x00;
	up->destination = 123;
	up->source = 123;

	i = udpSendPacket(up, ntpServer);
	if (!((i == 0) || (i == 1) || (i == 2) || (i == 3))) {
		i = udpSendPacket(up, ntpServer);
		if (!((i == 0) || (i == 1) || (i == 2) || (i == 3))) {
			freeUdpPacket(up);
			return 2;
		}
		if (i == 0) {
			return 0;
		} else if ((i == 2) || (i == 4)) {
			return 1;
		} else {
			return 2;
		}
	}
	if (i == 0) {
		return 0;
	} else if ((i == 2) || (i == 4)) {
		return 1;
	} else {
		return 2;
	}
	return 2;
}

#endif // DISABLE_NTP

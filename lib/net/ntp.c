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

#include <std.h>
#include <time.h>
#include <net/ipv4.h>
#include <net/udp.h>
#include <net/ntp.h>
#include <net/dns.h>
#include <net/utils.h>
#include <net/controller.h>

#ifndef DISABLE_NTP

#define NTPMessageSize 48
#define NTPFirstByte 0x0B // Version 1, Mode 3 (Client)

#ifndef DISABLE_DNS
uint8_t ntpServerDomain[] = "0.de.pool.ntp.org";
#endif

IPv4Address ntpServer = {78, 46, 85, 230};

uint8_t ntpHandler(Packet *p) {
	time_t stamp = 0;
	stamp |= (time_t)p->d[UDPOffset + UDPDataOffset + 16] << 56;
	stamp |= (time_t)p->d[UDPOffset + UDPDataOffset + 17] << 48;
	stamp |= (time_t)p->d[UDPOffset + UDPDataOffset + 18] << 40;
	stamp |= (time_t)p->d[UDPOffset + UDPDataOffset + 19] << 32;
	stamp |= (time_t)p->d[UDPOffset + UDPDataOffset + 20] << 24;
	stamp |= (time_t)p->d[UDPOffset + UDPDataOffset + 21] << 16;
	stamp |= (time_t)p->d[UDPOffset + UDPDataOffset + 22] << 8;
	stamp |= (time_t)p->d[UDPOffset + UDPDataOffset + 23];
	setNtpTimestamp(stamp);
	mfree(p->d, p->dLength);
	mfree(p, sizeof(Packet));
	return 0;
}

// 0 on success, 1 if destination unknown, try again later.
// 2 or 4 if there was not enough RAM. 3 on PHY Error
// On Return 0, 1, 2 and 3, up was already freed

// 0 on success, 1 on no mem, 2 on error
uint8_t ntpIssueRequest(void) {
	uint8_t i;
	Packet *p = (Packet *)mmalloc(sizeof(Packet));
	if (p == NULL) {
		return 1;
	}
	p->dLength = UDPOffset + UDPDataOffset + NTPMessageSize;
	p->d = (uint8_t *)mmalloc(p->dLength);
	if (p->d == NULL) {
		mfree(p, sizeof(Packet));
		return 1;
	}

#ifndef DISABLE_DNS
	dnsGetIp(ntpServerDomain, ntpServer); // If it doesn't work, we rely on the defaults
#endif

	p->d[UDPOffset + UDPDataOffset] = NTPFirstByte;
	for (i = 1; i < NTPMessageSize; i++) {
		p->d[UDPOffset + UDPDataOffset + i] = 0x00; // Yes, SNTP is simple...
	}

	return udpSendPacket(p, ntpServer, 123, 123);
}

#endif // DISABLE_NTP

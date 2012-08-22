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

uint8_t ntpHandler(Packet p) {
	return 0;
}

uint8_t ntpIssueRequest(void) {
	return 0;
}

#endif // DISABLE_NTP

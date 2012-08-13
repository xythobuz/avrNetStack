/*
 * controller.h
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
#ifndef _controller_h
#define _controller_h

#include <net/mac.h>

// #define DISABLE_IPV4_FRAGMENT // Prevent IPv4 Fragmentation
// #define DISABLE_IPV4_CHECKSUM // Prevent IPv4 Checksum calculation
// #define DISABLE_ICMP_STRINGS  // Don't store ICMP Names in Flash
// #define DISABLE_ICMP_CHECKSUM // Prevent ICMP Checksum calculation
// #define DISABLE_ICMP_ECHO     // Prevent answering to Echo Requests (Ping)

#define IPV4 0x0800
#define ARP 0x0806
#define WOL 0x0842
#define RARP 0x8035
#define IPV6 0x86DD

void networkInit(MacAddress a);
void networkHandler(void);

#endif

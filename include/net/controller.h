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

// -----------------------------------
// |        Feature Selection        |
// -----------------------------------

// #define DISABLE_IPV4_FRAGMENT // Prevent IPv4 Fragmentation
// #define DISABLE_IPV4_CHECKSUM // Prevent IPv4 Checksum calculation
// #define DISABLE_ICMP          // Disable complete ICMP Protocol
// #define DISABLE_ICMP_STRINGS  // Don't store ICMP Names in Flash
// #define DISABLE_ICMP_CHECKSUM // Prevent ICMP Checksum calculation
// #define DISABLE_ICMP_ECHO     // Prevent answering to Echo Requests (Ping)
// #define DISABLE_ICMP_UDP_MSG  // Don't send ICMP Error receiving packet for unhandled port.
// #define DISABLE_UDP           // Disable the complete UDP Protocol, needed for DHCP, DNS
// #define DISABLE_UDP_CHECKSUM  // Prevent UDP Checksum calculation
// #define DISABLE_DHCP          // Disable DHCP. Enter valid IP etc. in controller.c
// #define DISABLE_DNS           // Disable DNS. Uses UDP, not TCP!
// #define DISABLE_NTP           // Disable NTP.

// -----------------------------------
// |            RAM Usage            |
// -----------------------------------

// This times 14bytes is max. allocated RAM for ARP Cache
#define ARPMaxTableSize 8 // Should be less than 127

// -----------------------------------
// |          External API           |
// -----------------------------------

void networkInit(MacAddress a);
void networkHandler(void);

#define IPV4 0x0800
#define ARP 0x0806
#define WOL 0x0842
#define RARP 0x8035
#define IPV6 0x86DD

// -----------------------------------
// |          Dependencies           |
// -----------------------------------

#ifdef DISABLE_ICMP
#define DISABLE_ICMP_STRINGS
#define DISABLE_ICMP_CHECKSUM
#define DISABLE_ICMP_ECHO
#define DISABLE_ICMP_UDP_MSG
#endif

#ifdef DISABLE_UDP
#define DISABLE_UDP_CHECKSUM
#define DISABLE_DHCP
#define DISABLE_ICMP_UDP_MSG
#define DISABLE_DNS
#define DISABLE_NTP
#endif

#endif

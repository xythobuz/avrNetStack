/*
 * controller.h
 *
 * Copyright (c) 2012, Thomas Buck <xythobuz@me.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _controller_h
#define _controller_h

#include <time.h>
#include <serial.h>

#include <debug.h>

#define OK 0
#define MEM 1
#define ERROR 2

// -----------------------------------
// |        Feature Selection        |
// -----------------------------------

#define DISABLE_IPV4_FRAGMENT         // IPv4 Fragmentation currently not supported!
// #define DISABLE_IPV4_CHECKSUM         // Prevent IPv4 Checksum calculation
// #define DISABLE_ICMP                  // Disable complete ICMP Protocol
// #define DISABLE_ICMP_CHECKSUM         // Prevent ICMP Checksum calculation
// #define DISABLE_ICMP_ECHO             // Prevent answering to Echo Requests (Ping)
// #define DISABLE_ICMP_UDP_MSG          // Don't send ICMP Error for unhandled port.
// #define DISABLE_UDP                   // Disable the complete UDP Protocol
// #define DISABLE_UDP_CHECKSUM          // Prevent UDP Checksum calculation
#define DISABLE_DHCP                  // Disable DHCP. Enter valid IP etc. in controller.c
#define DISABLE_DNS                   // Disable DNS. Uses UDP, not TCP!
#define DISABLE_DNS_STRINGS           // Disable DNS Debug Output
#define DISABLE_DNS_DOMAIN_VALIDATION // Don't check if domains are valid
// #define DISABLE_NTP                   // Disable NTP.

// -----------------------------------
// |            RAM Usage            |
// -----------------------------------

#define ARPMaxTableSize 10 // This times 14 bytes will be allocated max
#define BUFFSIZE 80 // General String Buffer Size

// -----------------------------------
// |          External API           |
// -----------------------------------

typedef struct {
    uint8_t *d;
    uint16_t dLength;
} Packet;

#include <net/mac.h>
#include <net/ipv4.h>
// Both includes depend on this Packet definition, so we can include them only now!

// d = data, p = position, v = value
#define is16BitEqual(d, p, v) (((d)[(p)] == (((v) & 0xFF00) >> 8)) && ((d)[(p)+1] == ((v) & 0x00FF)))
#define get16Bit(d, p) (((d)[(p)] << 8) | (d)[(p)+1])
#define set16Bit(d, p, v) ({        \
    (d)[(p)] = ((v) & 0xFF00) >> 8; \
    (d)[(p) + 1] = ((v) & 0x00FF);  \
})

extern char buff[BUFFSIZE];
char *timeToString(time_t s);
char *hexToString(uint64_t s);
char *hex2ToString(uint64_t s);

void networkInit(uint8_t *mac, uint8_t *ip, uint8_t *subnet, uint8_t *gateway);
void networkLoop(void);

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

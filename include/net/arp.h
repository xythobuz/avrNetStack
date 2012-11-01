/*
 * arp.h
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
#ifndef _arp_h
#define _arp_h

#include <net/mac.h>
#include <net/ipv4.h>
#include <time.h>
#include <net/controller.h>

#define ARPTableTimeToLive 300000 // Keep unused Cache entries for 5 Minutes
#define ARPTableTimeToRetry 5000 // Request new answer every 5 seconds

// Defined here to allow "userspace" to inspect arp cache.
typedef struct ARPTableEntry ARPTableEntry;
struct ARPTableEntry {
    IPv4Address   ip;
    uint8_t       mac[6];
    time_t        time;
    ARPTableEntry *next;
};

#define ARPPacketSize 22 // Without header
#define ARPOffset MACPreambleSize + 6
#define ARPOperationOffset 0
#define ARPSourceMacOffset 2
#define ARPSourceIpOffset 8
#define ARPDestinationMacOffset 12
#define ARPDestinationIpOffset 18

extern ARPTableEntry *arpTable;

void arpInit(void);
uint8_t arpProcessPacket(Packet *p); // Processes all received ARP Packets
// Returns 0 on success, 1 if there was not enough memory,
// 2 if the packet was no valid ipv4 ethernet arp packet..
// p is freed afterwards!

// Searches in ARP Table. If entry is found, return non-alloced buffer with mac address.
// If there is no entry, issue arp packet and return NULL. Try again later.
uint8_t *arpGetMacFromIp(IPv4Address ip);

#endif

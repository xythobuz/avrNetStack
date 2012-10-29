/*
 * ipv4.h
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
#ifndef _ipv4_h
#define _ipv4_h

#include <net/mac.h>
#include <net/controller.h>

typedef uint8_t IPv4Address[4];

#define IPv4PacketFlagsOffset 6
#define IPv4PacketProtocolOffset 9
#define IPv4PacketSourceOffset 12
#define IPv4PacketDestinationOffset 16
#define IPv4PacketHeaderLength 20

#define IPv4MaxPacketSize (MaxPacketSize - IPv4PacketHeaderLength)

#define ICMP 0x01
#define IGMP 0x02
#define TCP 0x06
#define UDP 0x11

extern IPv4Address ownIpAddress;
extern IPv4Address subnetmask;
extern IPv4Address defaultGateway;

void ipv4Init(IPv4Address ip, IPv4Address subnet, IPv4Address gateway);

uint8_t ipv4ProcessPacket(Packet *p);
// Returns 0 on success, 1 if not enough mem, 2 if packet invalid.

// Gives default values for all fields in the IPv4 Header
// Also computes checksum, if enabled.
// Creates Ethernet and IPv4 Header, gets Target MAC, and off we go.
uint8_t ipv4SendPacket(Packet *p, uint8_t *target, uint8_t protocol);

uint8_t ipv4LastProtocol(void);

void ipv4SendQueue(void); // Send next packet in queue
uint8_t ipv4PacketsToSend(void); // Is something in the queue?

#endif

/*
 * ipv4.c
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
#include <avr/io.h>
#include <stdint.h>
#include <stdlib.h>

#define DEBUG 0 // 0 to receive no debug serial output
// 1 for error and status messages
// 2 for ips of each packet
// 3 for error messages

#include <std.h>
#include <time.h>
#include <net/mac.h>
#include <net/arp.h>
#include <net/controller.h>
#include <net/ipv4.h>
#include <net/icmp.h>
#include <net/udp.h>
#include <net/utils.h>

IPv4Address ownIpAddress;
IPv4Address subnetmask;
IPv4Address defaultGateway;

uint16_t risingIdentification = 1;

// Transmission Buffer, single-linked-list
typedef struct IpElement IpElement;
struct IpElement {
    Packet *p;
    IpElement *next;
};
IpElement *transmissionBuffer = NULL;

// ----------------------
// |    Internal API    |
// ----------------------

uint8_t isBroadcastIp(uint8_t *d) {
    uint8_t i;
    for (i = 0; i < 4; i++) {
        if (subnetmask[i] == 255) {
            if (!((d[i] == 255) || (d[i] == defaultGateway[i]))) {
                // ip does not match subnet or broadcast
                return 0;
            }
        } else {
            if (d[i] != 255) {
                return 0;
            }
        }
    }
    return 1;
}

#if (!defined(DISABLE_IPV4_CHECKSUM)) || (!defined(DISABLE_UDP_CHECKSUM))
uint16_t checksum(uint8_t *addr, uint16_t count) {
    // C Implementation Example from RFC 1071, p. 7, slightly adapted
    // Compute Internet Checksum for count bytes beginning at addr
    uint32_t sum = 0;

    while (count > 1) {
        sum += (((uint16_t)(*addr++)) << 8);
        sum += (*addr++); // Reference Implementation assumes wrong endianness...
        count -= 2;
    }

    // Add left-over byte, if any
    if (count > 0)
        sum += (((uint16_t)(*addr++)) << 8);

    // Fold 32-bit sum to 16 bits
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return ~sum;
}
#endif

uint8_t addToBuffer(Packet *p) {
    IpElement *l = (IpElement *)mmalloc(sizeof(IpElement));
    if (l == NULL) {
        mfree(p->d, p->dLength);
        mfree(p, sizeof(Packet));
        return 1;
    }
    l->p = p;
    l->next = transmissionBuffer;
    transmissionBuffer = l;
    return 0;
}

IpElement *nextPacketReady(IpElement **prev) {
    IpElement *p;
    for (p = transmissionBuffer; p != NULL; p = p->next) {
        if (arpGetMacFromIp(p->p->d + MACPreambleSize + IPv4PacketDestinationOffset) != NULL) {
            return p;
        }
        if (prev != NULL) {
            *prev = p;
        }
    }
    return NULL;
}

// ----------------------
// |    External API    |
// ----------------------

void ipv4Init(IPv4Address ip, IPv4Address subnet, IPv4Address gateway) {
    uint8_t i;
    for (i = 0; i < 4; i++) {
        ownIpAddress[i] = ip[i];
        subnetmask[i] = subnet[i];
        defaultGateway[i] = gateway[i];
    }
#if DEBUG >= 3
    debugPrint("IP: ");
    for (i = 0; i < 4; i++) {
        debugPrint(timeToString(ip[i]));
        if (i < 3) {
            debugPrint(".");
        }
    }
    debugPrint("\nSubnet: ");
    for (i = 0; i < 4; i++) {
        debugPrint(timeToString(subnet[i]));
        if (i < 3) {
            debugPrint(".");
        }
    }
    debugPrint("\nGateway: ");
    for (i = 0; i < 4; i++) {
        debugPrint(timeToString(gateway[i]));
        if (i < 3) {
            debugPrint(".");
        }
    }
    debugPrint("\n");
#endif
}

uint8_t ipLastProtocol = 0x00;

uint8_t ipv4LastProtocol(void) {
    return ipLastProtocol;
}

// Returns 0 on success, 1 if not enough mem, 2 if packet invalid.
uint8_t ipv4ProcessPacket(Packet *p) {
    uint16_t cs = 0x0000, w;
    uint8_t pr;
#if DEBUG >= 2
    uint8_t i;
#endif

    assert(p->dLength > (MACPreambleSize + IPv4PacketHeaderLength)); // Big enough
    assert(p->dLength < MaxPacketSize); // Not too big

#ifndef DISABLE_IPV4_CHECKSUM
    cs = checksum(p->d + MACPreambleSize, IPv4PacketHeaderLength);
#endif
    if ((cs != 0x0000) || ((p->d[MACPreambleSize] & 0xF0) != 0x40)) {
        // Checksum or version invalid
#if DEBUG >= 1
        debugPrint("Checksum: ");
        debugPrint(hexToString(cs));
        debugPrint("  First byte: ");
        debugPrint(hexToString(p->d[MACPreambleSize]));
        debugPrint("!\n");
#endif
        mfree(p->d, p->dLength);
        mfree(p, sizeof(Packet));
        return 2;
    } else {
        debugPrint("Valid IPv4 Packet!\n");
    }

    w = get16Bit(p->d, MACPreambleSize + IPv4PacketFlagsOffset);
    if (w & 0x1FFF) {
#if DEBUG >= 1
        debugPrint("Fragment Offset is ");
        debugPrint(hexToString(w & 0x1FFF));
        debugPrint("!\n");
#endif
        mfree(p->d, p->dLength);
        mfree(p, sizeof(Packet));
        return 2;
    }
    if (w & 0x2000) {
        // Part of a fragmented IPv4 Packet... No support for that
        debugPrint("More Fragments follow!\n");
        mfree(p->d, p->dLength);
        mfree(p, sizeof(Packet));
        return 2;
    }

    if (isBroadcastIp(p->d + MACPreambleSize + IPv4PacketDestinationOffset)) {
        debugPrint("IPv4 Broadcast Packet!\n");
    } else if (isEqualMem(ownIpAddress, p->d + MACPreambleSize + IPv4PacketDestinationOffset, 4)) {
        debugPrint("IPv4 Packet for us!\n");
    } else {
        debugPrint("IPv4 Packet not for us!\n");
        mfree(p->d, p->dLength);
        mfree(p, sizeof(Packet));
        return 0;
    }

    // Packet to act on...
#if DEBUG >= 2
    debugPrint("From: ");
    for (i = 0; i < 4; i++) {
        debugPrint(timeToString(p->d[MACPreambleSize + IPv4PacketSourceOffset + i]));
        if (i < 3) {
            debugPrint(".");
        }
    }
    debugPrint("\nTo: ");
    for (i = 0; i < 4; i++) {
        debugPrint(timeToString(p->d[MACPreambleSize + IPv4PacketDestinationOffset + i]));
        if (i < 3) {
            debugPrint(".");
        }
    }
    debugPrint("\n");
#endif

    pr = p->d[MACPreambleSize + IPv4PacketProtocolOffset];
    ipLastProtocol = pr;
    if (pr == ICMP) {
        debugPrint("Is ICMP Packet!\n");
        return icmpProcessPacket(p);
    } else if (pr == IGMP) {
        debugPrint("Is IGMP Packet!\n");
    } else if (pr == TCP) {
        debugPrint("Is TCP Packet!\n");
    } else if (pr == UDP) {
        debugPrint("Is UDP Packet!\n");
        return udpHandlePacket(p);
    } else {
#if DEBUG >= 1
        debugPrint("No handler for: ");
        debugPrint(hexToString(pr));
        debugPrint("!\n");
#endif
        mfree(p->d, p->dLength);
        mfree(p, sizeof(Packet));
        return 0;
    }

    mfree(p->d, p->dLength);
    mfree(p, sizeof(Packet));
    return 0;
}

uint8_t ipv4SendPacket(Packet *p, uint8_t *target, uint8_t protocol) {
    uint16_t tLength = p->dLength - MACPreambleSize;
    uint8_t *mac = NULL;

    // Prepare Header Data
    p->d[12] = (IPV4 & 0xFF00) >> 8;
    p->d[13] = (IPV4 & 0x00FF); // IPv4 Protocol
    p->d[MACPreambleSize + 0] = (4) << 4; // Version
    p->d[MACPreambleSize + 0] |= 5; // InternetHeaderLength
    p->d[MACPreambleSize + 1] = 0; // Type Of Service
    p->d[MACPreambleSize + 2] = (tLength & 0xFF00) >> 8;
    p->d[MACPreambleSize + 3] = (tLength & 0x00FF);
    p->d[MACPreambleSize + 4] = (risingIdentification & 0xFF00) >> 8;
    p->d[MACPreambleSize + 5] = (risingIdentification++ & 0x00FF);
    p->d[MACPreambleSize + 6] = 0;
    p->d[MACPreambleSize + 7] = 0; // Flags and Fragment Offset
    p->d[MACPreambleSize + 8] = 0xFF; // Time To Live
    p->d[MACPreambleSize + 10] = 0;
    p->d[MACPreambleSize + 11] = 0; // Checksum field
    p->d[MACPreambleSize + IPv4PacketProtocolOffset] = protocol;
    for (tLength = 0; tLength < 4; tLength++) {
        p->d[MACPreambleSize + IPv4PacketSourceOffset + tLength] = ownIpAddress[tLength];
        p->d[MACPreambleSize + IPv4PacketDestinationOffset + tLength] = target[tLength];
    }

#ifndef DISABLE_IPV4_CHECKSUM
    tLength = checksum(p->d + MACPreambleSize, IPv4PacketHeaderLength);
    p->d[MACPreambleSize + 10] = (tLength & 0xFF00) >> 8;
    p->d[MACPreambleSize + 11] = (tLength & 0x00FF);
#endif

    // Aquire MAC
    mac = arpGetMacFromIp(target);
    if (mac != NULL) { // Target MAC known
        // Insert MACs
        for (tLength = 0; tLength < 6; tLength++) {
            p->d[tLength] = mac[tLength]; // Destination
            p->d[6 + tLength] = ownMacAddress[tLength]; // Source
        }

        // Try to send packet...
        tLength = macSendPacket(p);
        if (tLength) {
            // Could not send, so put into buffer to try again later...
            debugPrint("Moved Packet into IPv4 Transmit Buffer (");
            debugPrint(timeToString(tLength));
            debugPrint(")\n");
            return addToBuffer(p);
        }
        mfree(p->d, p->dLength);
        mfree(p, sizeof(Packet));
        return 0;
    } else {
        // MAC Unknown, insert packet into queue
        debugPrint("MAC Unknown. Moved Packet into IPv4 Transmit Buffer.\n");
        return addToBuffer(p);
    }
}

void ipv4SendQueue(void) {
    uint8_t *mac;
    IpElement *prev;
    IpElement *p = nextPacketReady(&prev);
    if (p != NULL) {
        debugPrint("Working on IPv4 Send Queue...\n");
        mac = arpGetMacFromIp(p->p->d + MACPreambleSize + IPv4PacketDestinationOffset);
        for (uint8_t i = 0; i < 6; i++) {
            p->p->d[i] = mac[i]; // Destination
            p->p->d[6 + i] = ownMacAddress[i]; // Source
        }
        p->p->d[12] = (IPV4 & 0xFF00) >> 8;
        p->p->d[13] = (IPV4 & 0x00FF); // IPv4 Protocol

        // Try to send packet...
        if (macSendPacket(p->p) == 0) {
            if (prev == NULL) {
                transmissionBuffer = p->next;
            } else {
                prev->next = p->next;
            }
            mfree(p->p->d, p->p->dLength);
            mfree(p->p, sizeof(Packet));
            mfree(p, sizeof(IpElement));
        }
    }
}

uint8_t ipv4PacketsToSend(void) {
    if (nextPacketReady(NULL) != NULL) {
        return 1;
    } else {
        return 0;
    }
}

uint8_t ipv4PacketsInQueue(void) {
    uint8_t c = 0;
    for (IpElement *p = transmissionBuffer; p != NULL; p = p->next) {
        c++;
    }
    return c;
}

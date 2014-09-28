/*
 * arp.c
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
#include <avr/pgmspace.h>

#define DEBUG 0 // 0 to receive no debug serial output
// 2 to also get a message for every received ARP Request.

#include <std.h>
#include <net/mac.h>
#include <net/ipv4.h>
#include <net/arp.h>
#include <net/utils.h>
#include <serial.h>
#include <net/controller.h>

ARPTableEntry *arpTable = NULL;

#define HEADERLEN 6
const uint8_t ArpPacketHeader[HEADERLEN] PROGMEM = {0x00, 0x01, 0x08, 0x00, 0x06, 0x04};

uint8_t macReturnBuffer[6];

// ------------------------
// |     Internal API     |
// ------------------------

uint8_t sendArpRequest(IPv4Address ip) {
    uint8_t i;
    Packet *p;
#if DEBUG >= 1
    debugPrint("Sending ARP Request for ");
    for (i = 0; i < 4; i++) {
        debugPrint(timeToString(ip[i]));
        if (i < 3) {
            debugPrint(".");
        }
    }
    debugPrint("...");
#endif
    p = (Packet *)mmalloc(sizeof(Packet));
    if (p == NULL) {
        debugPrint("Not enough memory for Packet struct!\n");
        return 0;
    }
    p->dLength = MACPreambleSize + HEADERLEN + ARPPacketSize;
    p->d = (uint8_t *)mmalloc(p->dLength);
    if (p->d == NULL) {
        mfree(p, sizeof(Packet));
        return 0;
    }
    for (i = 0; i < 6; i++) {
        p->d[i] = 0xFF; // Target MAC
        p->d[6 + i] = ownMacAddress[i];
        p->d[MACPreambleSize + i] = pgm_read_byte(&(ArpPacketHeader[i])); // ARP Header
        p->d[MACPreambleSize + HEADERLEN + 2 + i] = ownMacAddress[i];
        p->d[MACPreambleSize + HEADERLEN + 12 + i] = 0xFF;
        if (i < 4) {
            p->d[MACPreambleSize + HEADERLEN + 8 + i] = ownIpAddress[i];
            p->d[MACPreambleSize + HEADERLEN + 18 + i] = ip[i]; // Target IP
        }
    }
    p->d[12] = (ARP & 0xFF00) >> 8;
    p->d[13] = (ARP & 0x00FF); // ARP Packet
    p->d[MACPreambleSize + HEADERLEN] = 0;
    p->d[MACPreambleSize + HEADERLEN + 1] = 1; // Request
    i = macSendPacket(p);
    mfree(p->d, p->dLength);
    mfree(p, sizeof(Packet));
    if (i) {
        debugPrint(" Error!\n");
        return 0;
    } else {
        debugPrint(" Done!\n");
        return 1;
    }
}

uint8_t isIpInThisNetwork(uint8_t *d) {
    uint8_t i;
    for (i = 0; i < 4; i++) {
        if (subnetmask[i] == 255) {
            if (d[i] != defaultGateway[i]) {
                return 0;
            }
        }
    }
    return 1;
}

ARPTableEntry *findIpFromMac(uint8_t *mac) {
    ARPTableEntry *p = arpTable;
    while (p != NULL) {
        if (isEqualMem(mac, p->mac, 6)) {
            return p;
        }
        p = p->next;
    }
    return NULL;
}

ARPTableEntry *findMacFromIp(IPv4Address ip) {
    ARPTableEntry *p = arpTable;
    while (p != NULL) {
        if (isEqualMem(ip, p->ip, 4)) {
            return p;
        }
        p = p->next;
    }
    return NULL;
}

ARPTableEntry *newEntry(void) {
    ARPTableEntry *p = (ARPTableEntry *)mmalloc(sizeof(ARPTableEntry));
    if (p != NULL) {
        p->next = arpTable;
        arpTable = p;
    }
    return p;
}

void addMacIpPair(uint8_t *mac, uint8_t *ip) {
    // Check if IP is already stored without MAC, copy MAC.
    // Else, if the MAC is not stored, store it.
    uint8_t i;

    if (isEqualMem(mac, ownMacAddress, 6) || isEqualMem(ip, ownIpAddress, 4)) {
        return;
    }

    if (findMacFromIp(ip) != NULL) {
        ARPTableEntry *t = findMacFromIp(ip);
        for (i = 0; i < 6; i++) {
            t->mac[i] = mac[i];
        }
    } else if (findIpFromMac(mac) == NULL) {
        ARPTableEntry *t = newEntry();
        if (t != NULL) {
            for (i = 0; i < 6; i++) {
                t->mac[i] = mac[i];
                if (i < 4) {
                    t->ip[i] = ip[i];
                }
            }
            t->time = getSystemTime();
        }
    }
}

// ------------------------
// |     External API     |
// ------------------------

void arpInit(void) {
    uint8_t i;
    arpTable = (ARPTableEntry *)mmalloc(sizeof(ARPTableEntry));
    if (arpTable != NULL) {
        for (i = 0; i < 6; i++) {
            arpTable->mac[i] = 0xFF;
            if (i < 4) {
                arpTable->ip[i] = 0xFF;
            }
        }
        arpTable->time = 0;
        arpTable->next = NULL;
    }
}

uint8_t arpProcessPacket(Packet *p) {
    uint8_t i;

    assert(p->dLength >= (ARPOffset + ARPPacketSize)); // Has correct length?

    if (!(isEqualFlash(p->d + MACPreambleSize, ArpPacketHeader, HEADERLEN) && (p->dLength >= (HEADERLEN + 22 + MACPreambleSize)))) {
        // Packet invalid
        debugPrint("ARP Packet not valid!\n");
        mfree(p->d, p->dLength);
        mfree(p, sizeof(Packet));
        return 2;
    }

    if (p->d[MACPreambleSize + HEADERLEN + 1] == 1) {
        // ARP Request

        // Sender MAC & IP
        addMacIpPair(p->d + MACPreambleSize + HEADERLEN + 2, p->d + MACPreambleSize + HEADERLEN + 8);

        // Check if the request is for us. If so, issue an answer!
        if (isEqualMem(ownIpAddress, p->d + MACPreambleSize + HEADERLEN + 18, 4)) {
            debugPrint("ARP Request for us!");
            p->d[MACPreambleSize + HEADERLEN + 1] = 2; // Reply
            for (i = 0; i < 6; i++) {
                p->d[MACPreambleSize + HEADERLEN + 12 + i] = p->d[MACPreambleSize + HEADERLEN + 2 + i]; // Back to sender
                p->d[MACPreambleSize + HEADERLEN + 2 + i] = ownMacAddress[i]; // Comes from us
                p->d[i] = 255;
                p->d[i + 6] = ownMacAddress[i];
                p->d[12] = (ARP & 0xFF00) >> 8;
                p->d[13] = (ARP & 0x00FF);
                if (i < 4) {
                    p->d[MACPreambleSize + HEADERLEN + 18 + i] = p->d[MACPreambleSize + HEADERLEN + 8 + i];

                    p->d[MACPreambleSize + HEADERLEN + 8 + i] = ownIpAddress[i];
                }
            }
            debugPrint(" Sending Response...");
            if (macSendPacket(p)) {
                mfree(p->d, p->dLength);
                mfree(p, sizeof(Packet));
                debugPrint(" Error!\n");
                return 1;
            }
            mfree(p->d, p->dLength);
            mfree(p, sizeof(Packet));
            debugPrint(" Done!\n");
            return 0;
        } else {
            // Request is not for us. Ignore!
#if DEBUG >= 2
            debugPrint("ARP Request for ");
            for (i = 0; i < 4; i++) {
                debugPrint(timeToString(p->d[MACPreambleSize + HEADERLEN + 18 + i]));
                if (i < 3) {
                    debugPrint(".");
                }
            }
            debugPrint("\n");
#endif
        }

        mfree(p->d, p->dLength);
        mfree(p, sizeof(Packet));
        return 0;

    } else if (p->d[MACPreambleSize + HEADERLEN + 1] == 2) {
        debugPrint("Got ARP Reply\n");
        // ARP Reply. Store the information, if not already present
        // Each packet contains two MAC-IP Combinations. Sender & Target
        addMacIpPair(p->d + MACPreambleSize + HEADERLEN + 2, p->d + MACPreambleSize + HEADERLEN + 8);
        addMacIpPair(p->d + MACPreambleSize + HEADERLEN + 12, p->d + MACPreambleSize + HEADERLEN + 18);
        mfree(p->d, p->dLength);
        mfree(p, sizeof(Packet));
        return 0;
    } else {
        // Neither request nor reply...
        debugPrint("Invalid ARP Packet Type!\n");
        mfree(p->d, p->dLength);
        mfree(p, sizeof(Packet));
        return 2;
    }
    return 0;
}

// Searches in ARP Table. If entry is found, return non-alloced buffer
// with mac address and update the time of the entry.
uint8_t *arpGetMacFromIp(IPv4Address ip) {
    // Clear old entries
    ARPTableEntry *p = arpTable;
    ARPTableEntry *prev = NULL;
    while (p != NULL) {
        if ((p->time + ARPTableTimeToLive) <= getSystemTime()) {
            if (prev == NULL) {
                arpTable = p->next;
                mfree(p, sizeof(ARPTableEntry));
                p = arpTable;
            } else {
                prev->next = p->next;
                mfree(p, sizeof(ARPTableEntry));
                p = prev->next;
            }
        } else {
            prev = p;
            p = p->next;
        }
    }

    if (!isIpInThisNetwork(ip)) {
#if DEBUG >= 1
        debugPrint("ARP Cache Request for IP: ");
        for (i = 0; i < 4; i++) {
            debugPrint(timeToString(ip[i]));
            if (i < 3) {
                debugPrint(".");
            }
        }
        debugPrint("\nRedirecting to default Gateway...\n");
#endif
        return arpGetMacFromIp(defaultGateway);
    }

    p = findMacFromIp(ip);
    if (p != NULL) {
        if (isZero(p->mac, 6)) {
            // We're waiting for an answer
            if ((p->time + ARPTableTimeToRetry) <= getSystemTime()) {
                // Waiting too long, re-issue request
                sendArpRequest(ip);
                return NULL;
            } else {
                // Requested, but not yet answered
                return NULL;
            }
        } else {
            // Answer is present
            for (uint8_t i = 0; i < 6; i++) {
                macReturnBuffer[i] = p->mac[i];
            }
            p->time = getSystemTime();
            return macReturnBuffer;
        }
    } else {
        // No entry present. Create it and request ARP Reply
        p = newEntry();
        if (p == NULL) {
            return NULL;
        }
        for (uint8_t i = 0; i < 6; i++) {
            p->mac[i] = 0;
            if (i < 4) {
                p->ip[i] = ip[i];
            }
        }
        p->time = getSystemTime();
        sendArpRequest(ip);
        return NULL;
    }
}

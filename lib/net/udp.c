/*
 * udp.c
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

#define DEBUG 0

#include <std.h>
#include <net/icmp.h>
#include <net/utils.h>
#include <net/udp.h>
#include <net/utils.h>
#include <net/controller.h>

#ifndef DISABLE_UDP

uint8_t isBroadcastIp(uint8_t *d);

typedef struct {
    uint16_t port;
    uint8_t (*func)(Packet *);
} UdpHandler;

UdpHandler *handlers = NULL;
uint16_t udpRegisteredHandlers = 0;
IPv4Address target;

// --------------------------
// |      Internal API      |
// --------------------------

uint16_t checksum(uint8_t *rawData, uint16_t l); // From ipv4.c

int16_t findHandler(uint16_t port) {
    uint16_t i;
    if (handlers != NULL) {
        for (i = 0; i < udpRegisteredHandlers; i++) {
            if (handlers[i].port == port) {
                return i;
            }
        }
    }
    return -1;
}

#ifndef DISABLE_UDP_CHECKSUM
uint16_t udpChecksum(Packet *p) {
    uint8_t i;
    uint16_t cs;
    // We create the pseudo header in our already present buffer,
    // calculate the checksum,
    // then move the ip addresses back to their old places, so it can be used again...
    // Required: UDP Length, Source and Target IP, UDP Data

    // Move IPs (8 bytes) from IPv4PacketSourceOffset (12) to 8
    for (i = 0; i < 8; i++) {
        p->d[MACPreambleSize + 8 + i] = p->d[MACPreambleSize + IPv4PacketSourceOffset + i];
    }

    // Insert data for Pseudo Header
    p->d[MACPreambleSize + 16] = 0x00; // Zeros
    p->d[MACPreambleSize + 17] = 0x11; // UDP Protocol
    p->d[MACPreambleSize + 18] = p->d[UDPOffset + UDPLengthOffset];
    p->d[MACPreambleSize + 19] = p->d[UDPOffset + UDPLengthOffset + 1]; // UDP Length

    // Clear UDP Checksum field
    p->d[UDPOffset + UDPChecksumOffset] = 0;
    p->d[UDPOffset + UDPChecksumOffset + 1] = 0;

#if DEBUG >= 2
    debugPrint("\nChecksum data size: ");
    debugPrint(timeToString(12 + get16Bit(p->d, UDPOffset + UDPLengthOffset)));
    debugPrint(" bytes.\nPseudo Header: ");
    for (i = 0; i < 12; i++) {
        if (i < 8) {
            debugPrint(timeToString((p->d + MACPreambleSize + 8)[i]));
        } else {
            debugPrint(hex2ToString((p->d + MACPreambleSize + 8)[i]));
        }
        if (i < 11) {
            debugPrint(" ");
        }
    }
    debugPrint("\nUDP Header: ");
    for (i = 0; i < 8; i++) {
        debugPrint(hex2ToString((p->d + MACPreambleSize + 20)[i]));
        if (i < 7) {
            debugPrint(" ");
        }
    }
    debugPrint("\n\n");
#endif

    // Calculate Checksum
    cs = checksum(p->d + MACPreambleSize + 8, 12 + get16Bit(p->d, UDPOffset + UDPLengthOffset));

    // Move IPs back
    for (i = 0; i < 8; i++) {
        p->d[MACPreambleSize + 12 + i] = p->d[MACPreambleSize + 8 + i];
    }
    // Restore IPv4 Protocol and Checksum
    p->d[MACPreambleSize + IPv4PacketProtocolOffset] = 0x11; // UDP
    p->d[MACPreambleSize + IPv4PacketProtocolOffset + 1] = 0;
    p->d[MACPreambleSize + IPv4PacketProtocolOffset + 2] = 0; // Checksum field

    return cs;
}
#endif

// --------------------------
// |      External API      |
// --------------------------

void udpInit(void) {}

// 0 on success, 1 not enough mem, 2 invalid
uint8_t udpHandlePacket(Packet *p) {
    uint8_t i;
    uint16_t ocs = 0x0000, cs = 0x0000;

    assert(p->dLength > (UDPOffset + UDPDataOffset));
    assert(p->dLength < MaxPacketSize);

#ifndef DISABLE_UDP_CHECKSUM
    ocs = get16Bit(p->d, UDPOffset + UDPChecksumOffset);
    cs = udpChecksum(p);
#endif
    if (cs != ocs) {
#if DEBUG >= 1
        debugPrint("UDP Checksum invalid: ");
        debugPrint(hexToString(cs));
        debugPrint(" != ");
        debugPrint(hexToString(ocs));
        debugPrint("\n");
#endif
        mfree(p->d, p->dLength);
        mfree(p, sizeof(Packet));
        return 2;
    }

    // Look for a handler
    for (i = 0; i < udpRegisteredHandlers; i++) {
        if (handlers[i].port == get16Bit(p->d, UDPOffset + UDPDestinationOffset)) {
            // found handler
            return handlers[i].func(p);
        }
    }

    debugPrint("UDP: No handler for ");
    debugPrint(timeToString(get16Bit(p->d, UDPOffset + UDPDestinationOffset)));
    debugPrint("\n");
    mfree(p->d, p->dLength);
    mfree(p, sizeof(Packet));
    return 0;
}


// Overwrites existing handler for this port
// 0 on succes, 1 on not enough RAM
uint8_t udpRegisterHandler(uint8_t (*handler)(Packet *), uint16_t port) {
    uint16_t i;

    // Check if port is already in list
    for (i = 0; i < udpRegisteredHandlers; i++) {
        if (handlers[i].port == port) {
            handlers[i].func = handler;
            return 0;
        }
    }

    // Extend list, add new handler.
    UdpHandler *tmp = (UdpHandler *)mrealloc(handlers, (udpRegisteredHandlers + 1) * sizeof(UdpHandler), udpRegisteredHandlers * sizeof(UdpHandler));
    if (tmp == NULL) {
        return 1;
    }
    handlers = tmp;
    handlers[udpRegisteredHandlers].port = port;
    handlers[udpRegisteredHandlers].func = handler;
    udpRegisteredHandlers++;
    return 0;
}

uint8_t udpSendPacket(Packet *p, uint8_t *targetIp, uint16_t targetPort, uint16_t sourcePort) {
    uint8_t i;
#ifndef DISABLE_UDP_CHECKSUM
    uint16_t cs;
#endif

    // We have to write ip addresses before calculating the checksum...
    for (i = 0; i < 4; i++) {
        p->d[MACPreambleSize + IPv4PacketSourceOffset + i] = ownIpAddress[i];
        p->d[MACPreambleSize + IPv4PacketDestinationOffset + i] = targetIp[i];
    }
    set16Bit(p->d, UDPOffset + UDPSourceOffset, sourcePort);
    set16Bit(p->d, UDPOffset + UDPDestinationOffset, targetPort);
    set16Bit(p->d, UDPOffset + UDPLengthOffset, p->dLength - UDPOffset);
#ifndef DISABLE_UDP_CHECKSUM
    cs = udpChecksum(p);
    set16Bit(p->d, UDPOffset + UDPChecksumOffset, cs);
#endif
    return ipv4SendPacket(p, targetIp, UDP);
}

#endif // DISABLE_UDP

/*
 * controller.c
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
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <string.h>

#define DEBUG 2 // 0 to receive no debug serial output
// 1 -> Init & Error Messages
// 2 -> Message for each received packet and it's type
// 3 -> Enable UDP Debug Port

#include <std.h>
#include <time.h>
#include <serial.h>
#include <tasks.h>
#include <scheduler.h>

#include <net/mac.h>
#include <net/arp.h>
#include <net/icmp.h>
#include <net/udp.h>
#include <net/dhcp.h>
#include <net/dns.h>
#include <net/ntp.h>
#include <net/controller.h>

uint8_t networkHandler(void);

char buff[BUFFSIZE];
uint16_t tl = 0;

char *timeToString(time_t s) {
    return ultoa(s, buff, 10);
}

char *hexToString(uint64_t s) {
    buff[0] = '0';
    buff[1] = 'x';
    ultoa(s, (buff + 2), 16);
    if ((strlen(buff + 2) % 2) != 0) {
        memmove(buff + 3, buff + 2, strlen(buff + 2) + 1); // Make room for...
        buff[2] = '0'; // ...trailing zero
    }
    return buff;
}

char *hex2ToString(uint64_t s) {
    ultoa(s, buff, 16);
    if ((strlen(buff) % 2) != 0) {
        memmove(buff + 1, buff, strlen(buff) + 1); // Make room for...
        buff[0] = '0'; // ...trailing zero
    }
    return buff;
}

#if DEBUG >= 2
char *typeString(uint16_t t) {
    if (tl == IPV4) {
        return "IPv4";
    } else if (tl == ARP) {
        return "ARP";
    } else if (tl == IPV6) {
        return "IPv6";
    }
    return "Unknown";
}
#endif

#if DEBUG >= 3
uint8_t debugUdpHandler(Packet *p) {
    uint16_t i, max;
    max = get16Bit(p->d, UDPOffset + UDPLengthOffset);
    debugPrint("UDP Debug: ");
    for (i = 0; i < max; i++) {
        serialWrite(p->d[UDPOffset + UDPDataOffset + i]);
        if (i < (max - 1)) {
            debugPrint(" ");
        }
    }
    debugPrint("\n");
    return 0;
}
#endif

void networkInit(uint8_t *mac, uint8_t *ip, uint8_t *subnet, uint8_t *gateway) {
    macInitialize(mac);
#if DEBUG >= 1
    debugPrint("Hardware Driver initialized: ");
    for (uint8_t i = 0; i < 6; i++) {
        debugPrint(hex2ToString(mac[i]));
        if (i < 5) {
            debugPrint("-");
        }
    }
    debugPrint("\n");
#endif
    arpInit();
    ipv4Init(ip, subnet, gateway);

#ifndef DISABLE_ICMP
    icmpInit();
    debugPrint("ICMP initialized...\n");
#endif

#ifndef DISABLE_UDP
    udpInit();
    debugPrint("UDP initialized...\n");
#if DEBUG >= 3
    udpRegisterHandler(&debugUdpHandler, 6600);
#endif
#ifndef DISABLE_DHCP
    // udpRegisterHandler(&dhcpHandler, 68);
#endif
#ifndef DISABLE_DNS
    // udpRegisterHandler(&dnsHandler, 53);
#endif
#ifndef DISABLE_NTP
    udpRegisterHandler(&ntpHandler, 123);
#endif
#endif // DISABLE_UDP

    addTask((Task)networkHandler, macHasInterrupt, "Poll"); // Enable polling
    addTask(ipv4SendQueue, ipv4PacketsToSend, "Send"); // Enable transmission

#ifndef DISABLE_NTP
    // addTimedTask((Task)ntpIssueRequest, 1000, 0);
#endif
}

void networkLoop(void) {
    // Run the tasks
    scheduler();
    wdt_reset();
    tasks();
    wdt_reset();
}

uint8_t networkHandler(void) {
    Packet *p;

    if (macLinkIsUp() && (macPacketsReceived() > 0)) {
        p = macGetPacket();

        if (p == NULL) {
            debugPrint("Error while receiving!\n");
            return 1;
        }

        assert(p->dLength > 0);
        assert(p->dLength <= MaxPacketSize);

        if (p->d == NULL) {
            debugPrint("Not enough memory to receive packet with ");
            debugPrint(timeToString(p->dLength));
            debugPrint(" bytes!\n");
            mfree(p, sizeof(Packet));
            return 1;
        }

        tl = get16Bit(p->d, 12);

#if DEBUG >= 2
        debugPrint(timeToString(getSystemTimeSeconds()));
        debugPrint(" - ");
        debugPrint(typeString(tl));
        debugPrint(" Packet with ");
        debugPrint(timeToString(p->dLength));
        debugPrint(" bytes Received!\n");
#endif

        if (tl == IPV4) {
            // IPv4 Packet
            return ipv4ProcessPacket(p);
        } else if (tl == ARP) {
            // Address Resolution Protocol Packet
            return arpProcessPacket(p);
        } else if (tl == WOL) {
            // Wake on Lan Packet
        } else if (tl == RARP) {
            // Reverse Address Resolution Protocol Packet
        } else if (tl <= 0x0600) {
            // Ethernet type I packet. typeLength = Real data length
        } else if (tl == IPV6) {
            // IPv6 Packet
        }

        // Packet unhandled, free it
        mfree(p->d, p->dLength);
        mfree(p, sizeof(Packet));
        return 42;
    }
    return 0xFF;
}

uint16_t networkLastProtocol(void) {
    return tl;
}

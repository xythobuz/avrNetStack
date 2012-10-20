/*
 * ntp.c
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
#include <time.h>
#include <net/ipv4.h>
#include <net/udp.h>
#include <net/ntp.h>
#include <net/dns.h>
#include <net/utils.h>
#include <net/controller.h>

#ifndef DISABLE_NTP

#define NTPMessageSize 48
#define NTPFirstByte 0x0B // Version 1, Mode 3 (Client)

#ifndef DISABLE_DNS
uint8_t ntpServerDomain[] = "0.de.pool.ntp.org";
#endif

IPv4Address ntpServer = { 78, 46, 85, 230 };

uint8_t ntpHandler(Packet *p) {
    time_t stamp = 0;
    debugPrint("Got NTP Response!\n");
    stamp |= (time_t)p->d[UDPOffset + UDPDataOffset + 16] << 24;
    stamp |= (time_t)p->d[UDPOffset + UDPDataOffset + 17] << 16;
    stamp |= (time_t)p->d[UDPOffset + UDPDataOffset + 18] << 8;
    stamp |= (time_t)p->d[UDPOffset + UDPDataOffset + 19];
    setNtpTimestamp(stamp);
    mfree(p->d, p->dLength);
    mfree(p, sizeof(Packet));
    debugPrint("Injected new timestamp!\n");
    return 0;
}

// 0 on success, 1 if destination unknown, try again later.
// 2 or 4 if there was not enough RAM. 3 on PHY Error
// On Return 0, 1, 2 and 3, up was already freed

// 0 on success, 1 on no mem, 2 on error
uint8_t ntpIssueRequest(void) {
    uint8_t i;
    Packet *p = (Packet *)mmalloc(sizeof(Packet));
    if (p == NULL) {
        return 1;
    }
    p->dLength = UDPOffset + UDPDataOffset + NTPMessageSize;
    p->d = (uint8_t *)mmalloc(p->dLength);
    if (p->d == NULL) {
        mfree(p, sizeof(Packet));
        return 1;
    }

#ifndef DISABLE_DNS
    dnsGetIp(ntpServerDomain, ntpServer); // If it doesn't work, we rely on the defaults
#endif

    p->d[UDPOffset + UDPDataOffset] = NTPFirstByte;
    for (i = 1; i < NTPMessageSize; i++) {
        p->d[UDPOffset + UDPDataOffset + i] = 0x00; // Yes, SNTP is simple...
    }

    debugPrint("Sending NTP Request...\n");

    return udpSendPacket(p, ntpServer, 123, 123);
}

#endif // DISABLE_NTP

/*
 * dns.c
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
#include <string.h>

#include <std.h>
#include <net/ipv4.h>
#include <net/udp.h>
#include <time.h>
#include <net/controller.h>
#include <net/utils.h>
#include <net/dns.h>

#ifndef DISABLE_DNS

DnsTableEntry *dnsTable = NULL;
uint16_t dnsNextId = 0;

// --------------------------
// |      Internal API      |
// --------------------------
void (*debugOutHandler)(char *) = NULL;
#define print(x) if (debugOutHandler != NULL) debugOutHandler(x)

#define DNSHEADERSIZE 12

#ifdef MAXDNSCACHESIZE

uint16_t dnsCacheSize(void) {
    uint16_t i = 0;
    DnsTableEntry *d = dnsTable;
    while (d != NULL) {
        i++;
        d = d->next;
    }
    return i;
}

time_t dnsOldestEntry(void) { // Returns UINT64_MAX if no entries...
    DnsTableEntry *d = dnsTable;
    time_t min = UINT64_MAX;
    while (d != NULL) {
        if (d->time < min) {
            min = d->time;
        }
        d = d->next;
    }
    return min;
}

#endif // MAXDNSCACHESIZE



#ifndef DISABLE_DNS_DOMAIN_VALIDATION
uint8_t stringContains(uint8_t *s, uint8_t c) {
    uint8_t count = 0;
    while (*s != '\0') {
        if (*(s++) == c)
            count++;
    }
    return count;
}
#endif

uint8_t stringPartLength(uint8_t *s, uint8_t c, uint8_t part) {
    uint8_t curPart = 0, count = 0;
    while (*s != '\0') {
        if (*s == c) {
            curPart++;
        } else if (curPart == part) {
            count++;
        }
        s++;
    }
    return count;
}

void stringCopyPart(uint8_t *s, uint8_t c, uint8_t part, uint8_t *d, uint8_t size) {
    uint8_t curPart = 0;
    while (*s != '\0') {
        if (*s == c) {
            curPart++;
        } else if (curPart == part) {
            memccpy(d, s, c, size);
            return;
        }
        s++;
    }
}

uint8_t *toDnsName(uint8_t *domain) {
    // Give it something like "www.google.com" to receive:
    // 3"www"6"google"3"com"0
    // to use as name in a dns request
    uint8_t parts, size, v, i, p, sum = 1; // Null byte at end
    uint8_t *name;
#ifndef DISABLE_DNS_DOMAIN_VALIDATION
    if ((parts = stringContains(domain, '.')) == 0) {
        return NULL; // Contains no dot...
    }
    if (strstr((char *)domain, "..") != NULL) {
        // There are two dots after another in this domain
        return NULL;
    }
#endif
    parts++;
    for (i = 0; i < parts; i++) {
        v = stringPartLength(domain, '.', i);
        sum += v + 1; // length byte
    }
    name = (uint8_t *)mmalloc(sum * sizeof(uint8_t));
    if (name == NULL) {
        return NULL; // No memory
    }
    p = 0;
    for (i = 0; i < parts; i++) {
        // Copy parts with length byte
        size = stringPartLength(domain, '.', i);
        name[p++] = size;
        stringCopyPart(domain, '.', i, (name + p), size);
        p += size;
    }
    name[p] = 0; // last null byte
    return name;
}

// --------------------------
// |      External API      |
// --------------------------

void dnsRegisterMessageCallback(void (*debugOutput)(char *)) {
    debugOutHandler = debugOutput;
}

uint8_t dnsHandler(Packet *p) {
    return 0;
}

// Returns 0 on success, 1 on no mem, 2 on invalid
uint8_t dnsGetIp(uint8_t *domain, IPv4Address ip) {
    return 0;
}

#endif // DISABLE_DNS

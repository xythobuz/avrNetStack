/*
 * utils.c
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

#define DEBUG 1 // Enable dumpPacketRaw

#include <net/mac.h>
#include <net/controller.h>

uint8_t isValue(uint8_t *x, uint16_t l, uint8_t c) {
    uint16_t i;
    for (i = 0; i < l; i++) {
        if (x[i] != c) {
            return 0;
        }
    }
    return 1;
}

uint8_t isEqualFlash(const uint8_t *d1, const uint8_t *d2, uint16_t l) {
    uint16_t i;
    for (i = 0; i < l; i++) {
        if (d1[i] != pgm_read_byte(&(d2[i]))) {
            return 0;
        }
    }
    return 1;
}

uint8_t isEqualMem(uint8_t *d1, uint8_t *d2, uint16_t l) {
    uint16_t i;
    for (i = 0; i < l; i++) {
        if (d1[i] != d2[i]) {
            return 0;
        }
    }
    return 1;
}

void dumpPacketRaw(Packet *p) {
#if DEBUG >= 1
    uint16_t i;
    debugPrint("Raw Packet Dump: ");
    for (i = 0; i < p->dLength; i++) {
        debugPrint(hex2ToString(p->d[i]));
        if (i < (p->dLength - 1)) {
            debugPrint(" ");
        }
    }
    debugPrint("\n");
#endif
}

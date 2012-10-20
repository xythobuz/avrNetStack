/*
 * mrf24wb0ma.c
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

#include <tasks.h>
#include <net/mac.h>
#include <net/controller.h>

#include "asynclabs/witypes.h"
#include "asynclabs/spi.h"
#include "asynclabs/g2100.h"

#define INTPORT PORTD
#define INTPORTPIN PIND
#define INTPIN PD2
#define INTDDR DDRD

uint8_t ownMacAddress[6];

uint8_t zg2100IsrEnabled; // In asynclabs spi.h

uint8_t zgInterruptCheck(void) {
    if (zg2100IsrEnabled) {
        if (INTPORTPIN & (1 << INTPIN)) {
            return 1;
        }
    }
    return 0;
}

uint8_t macInitialize(uint8_t *address) { // 0 if success, 1 on error
    uint8_t i;
    uint8_t *p;

    INTDDR &= ~(1 << INTPIN); // Interrupt PIN

    zg_init();

    addConditionalTask(zg_isr, zgInterruptCheck);
    addConditionalTask(zg_drv_process, taskTestAlways);

    p = zg_get_mac(); // Global Var. in g2100.c
    for (i = 0; i < 6; i++) {
        ownMacAddress[i] = p[i];
        address[i] = ownMacAddress[i];
    }

    return 0;
}

void macReset(void) {
    zg_chip_reset();
}

uint8_t macLinkIsUp(void) { // 0 if down, 1 if up
    return zg_get_conn_state();
}

uint8_t macSendPacket(Packet *p) { // 0 on success, 1 on error
    zg_sendPacket(p);
    return 0; // no way to know this?
}

uint8_t macPacketsReceived(void) { // 0 if no packet, 1 if packet ready
    if (zg_get_rx_status()) {
        return 1;
    }
    return 0;
}

Packet *macGetPacket(void) { // Returns NULL on error
    Packet *p = NULL;
    if (zg_get_rx_status()) {
        p = zg_buffAsPacket();
        zg_clear_rx_status();
    }
    return p;
}

uint8_t macHasInterrupt(void) {
    if (INTPORTPIN & (1 << INTPIN)) {
        return 0;
    } else {
        return 1;
    }
}

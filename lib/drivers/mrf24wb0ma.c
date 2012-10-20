/*
 * mrf24wb0ma.c
 *
 * Copyright 2012 Thomas Buck <xythobuz@me.com>
 *
 * This file is part of avrNetStack.
 *
 * avrNetStack is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * avrNetStack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with avrNetStack.  If not, see <http://www.gnu.org/licenses/>.
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

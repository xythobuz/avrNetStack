/*
 * uartStub.c
 *
 * Copyright 2012 Thomas Buck <xythobuz@me.com>
 *
 * This file is part of avrNetStack.
 *
 * avrNetStack is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * avrNetStack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with avrNetStack.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <avr/io.h>
#include <stdint.h>
#include <stdlib.h>

#include "../../include/net/mac.h"
#include "../../include/serial.h"

#define BUFFSIZE 20
char buffer[BUFFSIZE];

char *buffToString(uint8_t *d, uint8_t length) {
	uint8_t i;
	for (i = 0; i < BUFFSIZE; i++) {
		buffer[i] = '-';
	}
	buffer[BUFFSIZE - 1] = '\0';

	for (i = 0; i < length; i++) {
		utoa(d[i], (buffer + (3 * i)), 16);
	}
	return buffer;
}

char *wideToString(uint16_t d) {
	return utoa(d, buffer, 16);
}

uint8_t macInitialize(MacAddress address) { // 0 if success, 1 on error
	serialInit(BAUD(19200, F_CPU), 8, NONE, 1);
	serialWriteString("UART-Stub MAC Initialized!\n");
	return 0;
}

void macReset(void) {
	serialWriteString("Resetting MAC!\n");
}

uint8_t macLinkIsUp(void) { // 0 if down, 1 if up
	serialWriteString("We pretend our link is up...\n");
	return 1;
}

uint8_t macSendPacket(MacPacket *p) { // 0 on success, 1 on error
	serialWriteString("Sending Packet...\n");
	serialWriteString("Destination: ");
	serialWriteString(buffToString(p->destination, 6));
	serialWriteString("\nSource: ");
	serialWriteString(buffToString(p->source, 6));
	serialWriteString("\nType/Length: ");
	serialWriteString(wideToString(p->typeLength));
	serialWriteString("\nContent ommited...\n\n");
	return 0;
}

uint8_t macPacketsReceived(void) { // 0 if no packet, 1 if packet ready
	return 0;
}

MacPacket* macGetPacket(void) { // Returns NULL on error
	return NULL;
}

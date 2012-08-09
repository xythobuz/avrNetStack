/*
 * enc28j60.c
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

#include "../../include/mac.h"
#include "../../include/spi.h"

#define CSPORT PORTB
#define CSPIN PB0
#define CSDDR DDRB

#define ACTIVATE() (CSPORT &= ~(1 << CSPIN))
#define DEACTIVATE() (CSPORT |= (1 << CSPIN))

uint8_t currentBank = 0;

// ENC28J60 Commands
uint8_t readControlRegister(uint8_t a) {
	uint8_t r;
	// Opcode: 000
	// Argument: aaaaa
	ACTIVATE();
	spiSendByte(a & 0x1F);
	// Dummy byte if MAC or MII Register
	if (((currentBank == 2) && (a < 0x1B))
		|| ((currentBank == 3) && ((a <= 0x05) || (a == 0x0A)))) {
		spiReadByte();
	}

	r = spiReadByte();
	DEACTIVATE();
	return r;
}

uint8_t readBufferMemory(void) {
	uint8_t r;
	// Opcode: 001
	// Argument: 11010
	ACTIVATE();
	spiSendByte(0x3A);
	r = spiReadByte();
	DEACTIVATE();
	return r;
}

uint8_t *readBufferMemoryArray(uint8_t length) {
	uint8_t i, *d = (uint8_t *)malloc(length * sizeof(uint8_t));
	if (d == NULL) { return NULL; }
	ACTIVATE();
	spiSendByte(0x3A);
	for (i = 0; i < length; i++) {
		d[i] = spiReadByte();
	}
	DEACTIVATE();
	return d;
}

void writeControlRegister(uint8_t a, uint8_t d) {
	// Opcode: 010
	// Argument: aaaaa
	// Following: dddddddd
	ACTIVATE();
	spiSendByte(0x40 | (a & 0x1F));
	spiSendByte(d);
	DEACTIVATE();
}

void writeBufferMemory(uint8_t *d, uint8_t length) {
	uint8_t i;
	// Opcode: 011
	// Argument: 11010
	// Following: dddddddd
	ACTIVATE();
	spiSendByte(0x7A);
	for (i = 0; i < length; i++) {
		spiSendByte(d[i]);
	}
	DEACTIVATE();
}

void bitFieldSet(uint8_t a, uint8_t d) {
	// Opcode: 100
	// Argument: aaaaa
	// Following: dddddddd
	ACTIVATE();
	spiSendByte(0x80 | (a & 0x1F));
	spiSendByte(d);
	DEACTIVATE();
}

void bitFieldClear(uint8_t a, uint8_t d) {
	// Opcode: 101
	// Argument: aaaaa
	// Following: dddddddd
	ACTIVATE();
	spiSendByte(0xA0 | (a & 0x1F));
	spiSendByte(d);
	DEACTIVATE();
}

void systemResetCommand(void) {
	// Opcode 111
	// Argument: 11111
	ACTIVATE();
	spiSendByte(0xFF);
	DEACTIVATE();
}

// Driver API

uint8_t macInitialize(MacAddress address) { // 0 if success, 1 on error
	CSDDR |= (1 << CSPIN); // Chip Select as Output
	CSPORT |= (1 << CSPIN); // Deselect

	return 0;
}

void macReset(void) {

}

uint8_t macLinkIsUp(void) { // 0 if down, 1 if up
	return 0;
}

uint8_t macSendPacket(MacPacket p) { // 0 on success, 1 on error
	return 1;
}

uint8_t macPacketsRecieved(void) { // 0 if no packet, 1 if packet ready
	return 0;
}

MacPacket* macGetPacket(void) { // Returns NULL on error
	return NULL;
}

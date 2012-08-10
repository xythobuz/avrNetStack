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

// ----------------------------------
// |      ENC28J60 Command Set      |
// ----------------------------------

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

// ----------------------------------
// |       Internal Functions       |
// ----------------------------------

void selectBank(uint8_t bank) {
	if (bank < 4) {
		currentBank = bank;
		bitFieldClear(0x1F, 0x03); // Clear bank selection in ECON1
		if (bank > 0)
			bitFieldSet(0x1F, bank & 0x03); // Set new bank
	}
}

uint16_t readPhyRegister(uint8_t a) {
	uint16_t reg = 0;

	selectBank(2);
	writeControlRegister(0x14, (a & 0x1F)); // Set MIREGADR
	bitFieldSet(0x12, 0x01); // Set MICMD.MIIRD, read operation begins
	selectBank(3);
	while(readControlRegister(0x0A) & 0x01); // Wait for MISTAT.BUSY to go 0
	bitFieldClear(0x12, 0x01); // Clear MICMD.MIIRD
	selectBank(2);
	reg |= readControlRegister(0x18);
	reg |= (readControlRegister(0x19) << 8);
	selectBank(0); // Reset bank selection!
	return reg;
}

void writePhyRegister(uint8_t a, uint16_t d) {
	selectBank(2);
	writeControlRegister(0x14, (a & 0x1F)); // Set MIREGADR
	writeControlRegister(0x16, (uint8_t)(d & 0xFF)); // Set MIWRL
	writeControlRegister(0x17, (uint8_t)((d & 0xFF00) >> 8)); // Set MIWRH
	selectBank(3);
	while(readControlRegister(0x0A) & 0x01); // Wait for MISTAT.BUSY
}

// ----------------------------------
// |            MAC API             |
// ----------------------------------

uint8_t macInitialize(MacAddress address) { // 0 if success, 1 on error
	uint16_t phy = 0;

	CSDDR |= (1 << CSPIN); // Chip Select as Output
	CSPORT |= (1 << CSPIN); // Deselect

	selectBank(0);

	// Initialization as described in the datasheet, p. 35ff
	// Set Recieve Buffer Size
	writeControlRegister(0x08, 0xFF); // set ERXSTL
	writeControlRegister(0x09, 0x05); // set ERXSTH --> 0x05FF
	writeControlRegister(0x0A, 0xFF); // set ERXNDL
	writeControlRegister(0x0B, 0x1F); // set ERXNDH --> 0x1FFF
	writeControlRegister(0x0C, 0xFF); // set ERXRDPTL
	writeControlRegister(0x0D, 0x05); // set ERXRDPTH --> 0x05FF

	// Set Transmit Buffer Size
	writeControlRegister(0x04, 0x00); // set ETXSTL
	writeControlRegister(0x05, 0x00); // set ETXSTH --> 0x0000
	writeControlRegister(0x06, 0xFD); // set ETXNDL
	writeControlRegister(0x07, 0x05); // set ETXNDH --> 0x05FD
	writeControlRegister(0x00, 0x00); // set ERDPTL
	writeControlRegister(0x01, 0x00); // set ERDPTH --> 0x0000

	// Setup Receive Filters
	
	// Wait for OST
	while(!(readControlRegister(0x1D) & 0x01)); // Wait until CLKRDY == 1

	// Initialize MAC Settings
	// 1) Set MARXEN to recieve frames. Don't configure full-duplex mode
	selectBank(2);
	bitFieldSet(0x00, 0x01);
	// 2) Configure PADCFG, TXCRCEN.
	bitFieldSet(0x02, 0xF2); // Pad to 64bytes, auto CRC, check Framelength
	// 3) Configure MACON4, for conformance set DEFER
	bitFieldSet(0x03, 0x40);
	// 4) Program MAMXFL to 0x5ee --> max frame length 
	writeControlRegister(0x0A, 0xEE); // MAMXFLL -> 0xEE
	writeControlRegister(0x0B, 0x05); // MAMXFLH -> 0x05
	// 5) Configure MABBIPG with 0x15 (full duplex) or 0x12 (half duplex)
	writeControlRegister(0x04, 0x12);
	// 6) Set MAIPGL to 0x12
	writeControlRegister(0x06, 0x12);
	// 7) If half duplex, set MAIPGH to 0x0C
	writeControlRegister(0x07, 0x0C);
	// 8) For half duplex, set MACLCON1 & 2 to their default values?
	// 9) Write local MAC Address into MAADR1:MAADR6
	selectBank(3);
	writeControlRegister(0x04, address[0]);
	writeControlRegister(0x05, address[1]);
	writeControlRegister(0x02, address[2]);
	writeControlRegister(0x03, address[3]);
	writeControlRegister(0x00, address[4]);
	writeControlRegister(0x01, address[5]);
	// Always reset bank selection to zero!!
	selectBank(0);

	// Initialize PHY Settings
	// Duplex should be configured by LEDB polarity.
	// We force half-duplex anyways!
	phy = readPhyRegister(0x00); // Read PHCON1
	phy &= ~(1 << 8); // Clear PDPXMD --> Half duplex mode!
	writePhyRegister(0x00, phy);
	phy = readPhyRegister(0x10); // Read PHCON2
	phy |= (1 << 8); // Set HDLDIS to prevent auto loopback in half-duplex mode
	writePhyRegister(0x10, phy);

	return 0;
}

void macReset(void) {
	systemResetCommand();
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

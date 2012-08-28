/*
 * enc28j60.c
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

#define DEBUG 1 // 0 to 3

#include <net/mac.h>
#include <spi.h>
#include <net/controller.h>

#define CSPORT PORTA
#define CSPIN PA1
#define CSDDR DDRA

#define ACTIVATE() (CSPORT &= ~(1 << CSPIN))
#define DEACTIVATE() (CSPORT |= (1 << CSPIN))

uint8_t currentBank = 0;
uint16_t nextPacketPointer = 0x05FF; // Start of receive buffer

MacAddress ownMacAddress;

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

uint8_t *readBufferMemory(uint8_t *d, uint8_t length) {
	uint8_t i;
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

	selectBank(2);
	bitFieldClear(0x12, 0x01); // Clear MICMD.MIIRD
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

	selectBank(0);
}

void discardPacket(void) {
	writeControlRegister(0x0C, (uint8_t)(nextPacketPointer & 0xFF)); // set ERXRDPTL
	writeControlRegister(0x0D, (uint8_t)((nextPacketPointer & 0xFF00) >> 8)); // set ERXRDPTH --> nextPacketPointer
	bitFieldSet(0x1E, (1 << 6)); // Set ECON2.PKTDEC
}

// ----------------------------------
// |            MAC API             |
// ----------------------------------

uint8_t macInitialize(MacAddress address) { // 0 if success, 1 on error
	uint16_t phy = 0;
	uint8_t i;

	CSDDR |= (1 << CSPIN); // Chip Select as Output
	CSPORT |= (1 << CSPIN); // Deselect

	spiInit();

	systemResetCommand();

	for (i = 0; i < 6; i++) {
		ownMacAddress[i] = address[i];
	}

	selectBank(0);

	// Initialization as described in the datasheet, p. 35ff
	// Set Receive Buffer Size
	writeControlRegister(0x08, 0xFF); // set ERXSTL
	writeControlRegister(0x09, 0x05); // set ERXSTH --> 0x05FF
	writeControlRegister(0x0A, 0xFF); // set ERXNDL
	writeControlRegister(0x0B, 0x1F); // set ERXNDH --> 0x1FFF
	writeControlRegister(0x0C, 0xFF); // set ERXRDPTL
	writeControlRegister(0x0D, 0x05); // set ERXRDPTH --> 0x05FF

	// Default Receive Filters are acceptable.
	// We get unicast and broadcast packets as long as the crc is correct.
	
	// Wait for OST
	while(!(readControlRegister(0x1D) & 0x01)); // Wait until ESTAT.CLKRDY == 1

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

	// Enable Auto Increment for Buffer Writes
	bitFieldSet(0x1E, (1 << 7)); // Set ECON2.AUTOINC

	// If desired, you could enable interrupts here

	// Set LED Mode. LEDA Receive and Link, LEDB Transmit --> PHLCON = 0x3C12
	writePhyRegister(0x14, 0x3C12);

	// Enable packet reception
	bitFieldSet(0x1F, (1 << 2)); // Set ECON1.RXEN

#if DEBUG >= 1
	selectBank(3);
	debugPrint("ENC28J60 - Version ");
	i = readControlRegister(0x12);
	if (i == 0x02) {
		debugPrint("B1");
	} else if (i == 0x04) {
		debugPrint("B4");
	} else if (i == 0x05) {
		debugPrint("B5");
	} else if (i == 0x06) {
		debugPrint("B7");
	} else {
		debugPrint("Unknown");
	}
	debugPrint("!\n");
	selectBank(0);
#endif

	return 0;
}

void macReset(void) {
	systemResetCommand();
}

uint8_t macLinkIsUp(void) { // 0 if down, 1 if up
	uint16_t p = readPhyRegister(0x11); // Read PHSTAT2
#if DEBUG >= 3
	debugPrint("PHSTAT1: ");
	debugPrint(hexToString(readPhyRegister(0x01)));
	debugPrint("\nPHSTAT2: ");
	debugPrint(hexToString(p));
	debugPrint("\n");
#endif
	if (p & 0x0400) { // if LSTAT is set
		return 1;
	} else {
		return 0;
	}
}

uint8_t macSendPacket(Packet *p) { // 0 on success, 1 on error
	// Place Frame data in buffer, with a preceding control byte
	// This control byte can be 0x00, as we set everything needed in MACON3
	uint8_t i = 0x00;
#if DEBUG >= 2
	uint16_t a;
	uint8_t *po;
#endif

	selectBank(0);
	writeControlRegister(0x04, 0x00); // set ETXSTL
	writeControlRegister(0x05, 0x00); // set ETXSTH --> 0x0000

	// Write packet data into buffer
	writeControlRegister(0x02, 0x00); // EWRPTL
	writeControlRegister(0x03, 0x00); // EWRPTH --> 0x0000
	writeBufferMemory(&i, 1); // Write 0x00 as control byte
	writeBufferMemory(p->d, p->dLength); // Write data payload

	writeControlRegister(0x06, (uint8_t)(p->dLength & 0x00FF)); // ETXNDL
	writeControlRegister(0x07, (uint8_t)((p->dLength & 0xFF00) >> 8)); // ETXNDH --> dLength

	bitFieldSet(0x1F, 0x08); // ECON1.TXRTS --> start transmission

	free(p->d);
	free(p);

	while(readControlRegister(0x1F) & 0x08); // Wait for finish or abort, ECON1.TXRTS
#if DEBUG >= 2
	// Print status vector
	po = (uint8_t *)malloc(7 * sizeof(uint8_t));
	if (po != NULL) {
		a = 1 + p->dLength;
		writeControlRegister(0x00, (uint8_t)(a & 0xFF)); // Set ERDPTL
		writeControlRegister(0x01, (uint8_t)((a & 0xFF00) >> 8)); // Set ERDPTH
		readBufferMemory(po, 7); // Read status vector
		debugPrint("Status Vector:\n");
		for (i = 0; i < 7; i++) {
			debugPrint(timeToString(i));
			debugPrint(": ");
			debugPrint(hexToString(po[i]));
			debugPrint("\n");
		}
		free(po);
	}
#endif
	if (readControlRegister(0x1D) & (1 << 1)) { // If ESTAT.TXABRT is set
		return 1; // error
	} else {
		return 0;
	}
}

uint8_t macPacketsReceived(void) { // Returns number of packets ready
	uint8_t r;
	selectBank(1);
	r = readControlRegister(0x19); // EPKTCNT
	selectBank(0);
	return r;
}

Packet *macGetPacket(void) { // Returns NULL on error
	// Read and store next packet pointer,
	// check receive status vector for errors, if they exist, throw packet away
	// else read packet, build MacPacket, return it
	uint8_t d;
	uint8_t header[6];
	uint16_t fullLength;
	Packet *p = (Packet *)malloc(sizeof(Packet));
	if (p == NULL) {
		return NULL;
	}

	p->d = NULL;
	p->dLength = 0;

	if (macPacketsReceived() < 1) {
		return p;
	}

	writeControlRegister(0x00, (uint8_t)(nextPacketPointer & 0xFF)); // Set ERDPTL
	writeControlRegister(0x01, (uint8_t)((nextPacketPointer & 0xFF00) >> 8)); // Set ERDPTH

	readBufferMemory(&d, 1);
	nextPacketPointer = (uint16_t)d;
	readBufferMemory(&d, 1);
	nextPacketPointer |= ((uint16_t)d << 8);
	
	readBufferMemory(header, 4); // Read status vector
	fullLength = (uint16_t)header[0];
	fullLength |= (((uint16_t)header[1]) << 8);
	
	if (header[2] & (1 << 7)) {
		// Received OK
		p->dLength = fullLength;
		p->d = (uint8_t *)malloc(p->dLength * sizeof(uint8_t));
		if (p->d == NULL) {
			// discardPacket();
			return p;
		}
		readBufferMemory(p->d, p->dLength); // Read payload
		discardPacket();
		return p;
	} else {
		discardPacket();
		return p;
	}
}

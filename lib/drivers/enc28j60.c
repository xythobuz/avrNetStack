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
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/delay.h>

#define DEBUG 3
// 1 --> ENC28J60 Revision
// 2 --> 1 + Received and Sent Packets
// 3 --> 1 + 2 + Raw Sent Packet Dump
// 4 --> 1 + 2 + 3 + Sent Packets Status Vector
// 5 --> 1 + 2 + 3 + 4 + PHSTAT Registers on LinkIsUp

#include <std.h>
#include <net/mac.h>
#include <spi.h>
#include <net/controller.h>
#include <net/utils.h>

#define CSPORT PORTA
#define CSPIN PA1
#define CSDDR DDRA

#define INTPORT PORTD
#define INTPORTPIN PIND
#define INTPIN PD2
#define INTDDR DDRD

#define ACTIVATE() (CSPORT &= ~(1 << CSPIN))
#define DEACTIVATE() (CSPORT |= (1 << CSPIN))

#define RXSTART 0x0000
#define RXEND 0x17FF
#define TXSTART 0x1800
#define TXEND 0x1FFF

uint8_t currentBank = 0;
uint16_t nextPacketPointer = RXSTART; // Start of receive buffer
uint8_t macInitialized = 0;

MacAddress ownMacAddress;

#define A_BLOCK() ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
// #define A_BLOCK() if(1)

// ----------------------------------
// |      ENC28J60 Command Set      |
// ----------------------------------

uint8_t readControlRegister(uint8_t a) {
	uint8_t r;
	// Opcode: 000
	// Argument: aaaaa
	A_BLOCK() {
		ACTIVATE();
		spiSendByte(a & 0x1F);
		// Dummy byte if MAC or MII Register
		if (((currentBank == 2) && (a < 0x1B))
			|| ((currentBank == 3) && ((a <= 0x05) || (a == 0x0A)))) {
			spiReadByte();
		}

		r = spiReadByte();
		DEACTIVATE();
	}
	return r;
}

uint8_t *readBufferMemory(uint8_t *d, uint8_t length) {
	uint8_t i;
	A_BLOCK() {
		ACTIVATE();
		spiSendByte(0x3A);
		for (i = 0; i < length; i++) {
			d[i] = spiReadByte();
		}
		DEACTIVATE();
	}
	return d;
}

void writeControlRegister(uint8_t a, uint8_t d) {
	// Opcode: 010
	// Argument: aaaaa
	// Following: dddddddd
	A_BLOCK() {
		ACTIVATE();
		spiSendByte(0x40 | (a & 0x1F));
		spiSendByte(d);
		DEACTIVATE();
	}
}

void writeBufferMemory(uint8_t *d, uint8_t length) {
	uint8_t i;
	// Opcode: 011
	// Argument: 11010
	// Following: dddddddd
	A_BLOCK() {
		ACTIVATE();
		spiSendByte(0x7A);
		for (i = 0; i < length; i++) {
			spiSendByte(d[i]);
		}
		DEACTIVATE();
	}
}

void bitFieldSet(uint8_t a, uint8_t d) {
	// Opcode: 100
	// Argument: aaaaa
	// Following: dddddddd
	A_BLOCK() {
		ACTIVATE();
		spiSendByte(0x80 | (a & 0x1F));
		spiSendByte(d);
		DEACTIVATE();
	}
}

void bitFieldClear(uint8_t a, uint8_t d) {
	// Opcode: 101
	// Argument: aaaaa
	// Following: dddddddd
	A_BLOCK() {
		ACTIVATE();
		spiSendByte(0xA0 | (a & 0x1F));
		spiSendByte(d);
		DEACTIVATE();
	}
}

void systemResetCommand(void) {
	// Opcode 111
	// Argument: 11111
	A_BLOCK() {
		ACTIVATE();
		spiSendByte(0xFF);
		DEACTIVATE();
	}
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

void macReset(void) {
	systemResetCommand();
	macInitialized = 0;
}

uint8_t macHasInterrupt(void) {
	if (INTPORTPIN & (1 << INTPIN)) {
		return 0;
	} else {
		return 1;
	}
}

uint8_t macInitialize(MacAddress address) { // 0 if success, 1 on error
	uint16_t phy = 0;
	uint8_t i;

	CSDDR |= (1 << CSPIN); // Chip Select as Output
	CSPORT |= (1 << CSPIN); // Deselect
	INTDDR &= ~(1 << INTPIN); // Interrupt PIN

	spiInit();
	macReset();

	_delay_ms(1); // See Silicon Errata Page 2

	if ((address != NULL) && (address != ownMacAddress)) {
		for (i = 0; i < 6; i++) {
			ownMacAddress[i] = address[i];
		}
	}

	selectBank(0);

	// Initialization as described in the datasheet, p. 35ff
	// Set Receive Buffer Size
	writeControlRegister(0x08, (RXSTART & 0xFF)); // set ERXSTL
	writeControlRegister(0x09, (RXSTART & 0xFF00) >> 8); // set ERXSTH --> RXSTART
	writeControlRegister(0x0A, (RXEND & 0xFF)); // set ERXNDL
	writeControlRegister(0x0B, (RXEND & 0xFF00) >> 8); // set ERXNDH --> RXEND
	writeControlRegister(0x08, (RXSTART & 0xFF)); // set ERXRDPTH
	writeControlRegister(0x09, (RXSTART & 0xFF00) >> 8); // set ERXRDPTH --> RXSTART

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
	// 8) For half duplex, set MACLCON1 & 2 to their default values
	// 9) Write local MAC Address into MAADR1:MAADR6
	selectBank(3);
	writeControlRegister(0x04, ownMacAddress[0]);
	writeControlRegister(0x05, ownMacAddress[1]);
	writeControlRegister(0x02, ownMacAddress[2]);
	writeControlRegister(0x03, ownMacAddress[3]);
	writeControlRegister(0x00, ownMacAddress[4]);
	writeControlRegister(0x01, ownMacAddress[5]);
	// Always reset bank selection to zero!!
	selectBank(0);

	debugPrint("Preparing PHY...");
	// Initialize PHY Settings
	// Duplex should be configured by LEDB polarity.
	// We force half-duplex anyways!
	phy = readPhyRegister(0x10); // Read PHCON2
	phy |= (1 << 8); // Set HDLDIS to prevent auto loopback in half-duplex mode
	writePhyRegister(0x10, phy);
	phy = readPhyRegister(0x00); // Read PHCON1
	phy &= ~(1 << 8); // Clear PDPXMD --> Half duplex mode!
	writePhyRegister(0x00, phy);
	debugPrint(" Done!\n");

	// Enable Auto Increment for Buffer Writes
	bitFieldSet(0x1E, (1 << 7)); // Set ECON2.AUTOINC

	// Enable Packet Receive Interrupt on falling edge
	// Set EIE.PKTIE and EIE.INTIE
	bitFieldSet(0x1B, ((1 << 7) | (1 << 6)));

	// Set LED Mode. LEDA Receive and Link, LEDB Transmit --> PHLCON = 0x3C12
	writePhyRegister(0x14, 0x3C12);

#if DEBUG >= 1
	debugPrint("ENC28J60 - Version ");
	selectBank(3);
	i = readControlRegister(0x12);
	selectBank(0);
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
#endif

	// Enable packet reception
	bitFieldSet(0x1F, (1 << 2)); // Set ECON1.RXEN

	macInitialized = 1;

	return 0;
}

uint8_t macLinkIsUp(void) { // 0 if down, 1 if up
	uint16_t p = readPhyRegister(0x11); // Read PHSTAT2
	if (!macInitialized) {
		return 0;
	}
#if DEBUG >= 5
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
#if DEBUG >= 4
	uint16_t a;
	uint8_t *po;
#endif

	if (!macInitialized) {
		return 1;
	}

	assert(p->dLength > 0);
	assert(p->dLength < 1500);

	selectBank(0);
	writeControlRegister(0x04, (TXSTART & 0xFF)); // set ETXSTL
	writeControlRegister(0x05, (TXSTART & 0xFF00) >> 8); // set ETXSTH --> TXSTART

	if ((TXSTART + p->dLength) >= TXEND) {
		return 1;
	}

	// Write packet data into buffer
	writeControlRegister(0x02, (TXSTART & 0xFF)); // EWRPTL
	writeControlRegister(0x03, (TXSTART & 0xFF00) >> 8); // EWRPTH --> TXSTART
	writeBufferMemory(&i, 1); // Write 0x00 as control byte
	writeBufferMemory(p->d, p->dLength); // Write data payload

	writeControlRegister(0x06, (uint8_t)(p->dLength & 0x00FF)); // ETXNDL
	writeControlRegister(0x07, (uint8_t)((p->dLength & 0xFF00) >> 8)); // ETXNDH --> dLength

	bitFieldSet(0x1F, 0x08); // ECON1.TXRTS --> start transmission

#if DEBUG >= 3
	dumpPacketRaw(p);
#endif

	while(readControlRegister(0x1F) & 0x08); // Wait for finish or abort, ECON1.TXRTS

#if DEBUG >= 2
	debugPrint("Sent Packet with ");
	debugPrint(timeToString(p->dLength));
	debugPrint(" bytes...\n");
#endif

#if DEBUG >= 4
	// Print status vector
	po = (uint8_t *)mmalloc(7 * sizeof(uint8_t));
	if (po != NULL) {
		a = TXSTART + 1 + p->dLength;
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
		mfree(po, 7 * sizeof(uint8_t));
	}
#endif
	if (readControlRegister(0x1D) & (1 << 1)) { // If ESTAT.TXABRT is set
		debugPrint("Error while sending Packet!\n");
		return 1; // error
	} else {
		return 0;
	}
}

uint8_t macPacketsReceived(void) { // Returns number of packets ready
	uint8_t r;
	if (!macInitialized) {
		return 0;
	}
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
	uint8_t header[4];
	uint16_t fullLength;
	Packet *p;
	
	if (!macInitialized) {
		return NULL;
	}
	
	p = (Packet *)mmalloc(sizeof(Packet));
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

	assert(fullLength > 0);
	assert(fullLength < 1500);

	if (header[2] & (1 << 7)) {
		// Received OK
#if DEBUG >= 2
		debugPrint("Received Packet with ");
		debugPrint(timeToString(fullLength));
		debugPrint(" bytes...\n");
#endif
		p->dLength = fullLength;
		p->d = (uint8_t *)mmalloc(p->dLength * sizeof(uint8_t));
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

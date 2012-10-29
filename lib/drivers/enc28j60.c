/*
 * enc28j60.c
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
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/delay.h>

#define DEBUG 4
// 1 --> ENC28J60 Revision
// 2 --> 1 + Received and Sent Packets
// 3 --> 1 + 2 + Raw Sent Packet Dump
// 4 --> 1 + 2 + 3 + Status Vectors
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

// Silicon Errata Issue 5
#define RXSTART 0x0000
#define RXEND 0x17FF
#define TXSTART 0x1800
#define TXEND 0x1FFF

#if RXSTART > RXEND
#error "ENC28J60 Receive Buffer Overlap not supported!"
#endif
#if TXSTART > TXEND
#error "ENC28J60 Transmit Buffer Overlap not supported!"
#endif
#if (RXEND-RXSTART) < 1500
#warning "ENC28J60 Receive Buffer may be too small..."
#endif
#if (TXEND-TXSTART) < 1500
#warning "ENC28J60 Transmit Buffer may be too small..."
#endif

uint8_t currentBank = 0;
uint16_t nextPacketPointer = RXSTART; // Start of receive buffer
uint8_t macInitialized = 0;
uint8_t statusVector[7];

uint8_t ownMacAddress[6];

uint8_t readControlRegister(uint8_t a);
uint8_t *readBufferMemory(uint8_t *d, uint8_t length);
void writeControlRegister(uint8_t a, uint8_t d);
void writeBufferMemory(uint8_t *d, uint8_t length);
void bitFieldSet(uint8_t a, uint8_t d);
void bitFieldClear(uint8_t a, uint8_t d);
void systemResetCommand(void);

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
    // Silicon Errata Issue 14: Only odd values into ERXRDPT, nextPacketPointer always even...
    if (nextPacketPointer == RXSTART) {
        nextPacketPointer = RXEND;
    } else {
        nextPacketPointer--;
    }
    writeControlRegister(0x08, (nextPacketPointer & 0xFF)); // set ERXRDPTH
    writeControlRegister(0x09, (nextPacketPointer & 0xFF00) >> 8); // set ERXRDPTH
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

uint8_t macInitialize(uint8_t *address) { // 0 if success, 1 on error
    uint16_t phy = 0;
    uint8_t i;

    CSDDR |= (1 << CSPIN); // Chip Select as Output
    CSPORT |= (1 << CSPIN); // Deselect
    INTDDR &= ~(1 << INTPIN); // Interrupt PIN

    spiInit();
    macReset();

    _delay_ms(1); // See Silicon Errata Issue 2

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

    // Silicon Errata Issue 14: Only odd values into ERXRDPT, nextPacketPointer always even...
    if (nextPacketPointer == RXSTART) {
        phy = RXEND;
    } else {
        phy = (nextPacketPointer - 1);
    }
    writeControlRegister(0x08, (phy & 0xFF)); // set ERXRDPTH
    writeControlRegister(0x09, (phy & 0xFF00) >> 8); // set ERXRDPTH

    // Default Receive Filters are acceptable.
    // We get unicast and broadcast packets as long as the crc is correct.

    // Wait for OST
    while(!(readControlRegister(0x1D) & 0x01)); // Wait until ESTAT.CLKRDY == 1

    // Initialize MAC Settings
    // 1) Set MARXEN to recieve frames. Configure full-duplex mode.
    selectBank(2);
    bitFieldSet(0x00, 0x0D); // Set MACON1.MARXEN, RXPAUS, TXPAUS

    // 2) Configure PADCFG, TXCRCEN, FULDPX.
    bitFieldSet(0x02, 0xF3); // Pad to 64bytes, auto CRC, check Framelength, Full Duplex
    // 3) Configure MACON4, for conformance set DEFER
    bitFieldSet(0x03, 0x40);
    // 4) Program MAMXFL to 0x5EE --> max frame length
    writeControlRegister(0x0A, 0xEE); // MAMXFLL -> 0xEE
    writeControlRegister(0x0B, 0x05); // MAMXFLH -> 0x05
    // 5) Configure MABBIPG with 0x15 (full duplex) or 0x12 (half duplex)
    writeControlRegister(0x04, 0x15);
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
    phy |= (1 << 8); // Set PDPXMD --> Full duplex mode!
    writePhyRegister(0x00, phy);
    debugPrint(" Done!\n");

    // Enable Auto Increment for Buffer Writes
    bitFieldSet(0x1E, (1 << 7)); // Set ECON2.AUTOINC

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

    // Clear Packet Counter
    writeControlRegister(0x19, 0); // EPKTCNT

    // Clear Interrupt Flags
    bitFieldClear(0x1C, 0x7B); // Clear Flags in EIR

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
    uint16_t a;

    if (!macInitialized) {
        return 1;
    }

    assert(p->dLength > 0);
    assert(p->dLength <= MaxPacketSize);

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

    writeControlRegister(0x06, (uint8_t)((TXSTART + p->dLength) & 0x00FF)); // ETXNDL
    writeControlRegister(0x07, (uint8_t)(((TXSTART + p->dLength) & 0xFF00) >> 8)); // ETXNDH --> dLength + TXSTART

    // Silicon Errata Issue 12: Reset Transmit Logic before starting transmission
    bitFieldSet(0x1F, 0x80); // Set ECON1.TXRST
    bitFieldClear(0x1F, 0x80); // Clear ECON1.TXRST

    // Silicon Errata Issue 13
    bitFieldClear(0x1C, 0x0A); // Clear EIR.TXERIF & TXIF

    bitFieldSet(0x1F, 0x08); // ECON1.TXRTS --> start transmission

#if DEBUG >= 3
    dumpPacketRaw(p);
#endif

    while((readControlRegister(0x1C) & 0x0A) == 0); // Wait for TXIF & TXERIF

    bitFieldClear(0x1F, 0x08); // Clear ECON1.TXRTS, Silicon Errata Issue 13

#if DEBUG >= 2
    debugPrint("Sending Packet with ");
    debugPrint(timeToString(p->dLength));
    debugPrint(" bytes...\n");
#endif

    // Get status vector
    a = TXSTART + 1 + p->dLength; // 1 Control byte in front
    writeControlRegister(0x00, (uint8_t)(a & 0xFF)); // Set ERDPTL
    writeControlRegister(0x01, (uint8_t)((a & 0xFF00) >> 8)); // Set ERDPTH
    readBufferMemory(statusVector, 7); // Read status vector

#if DEBUG >= 4
    // Print status vector
    debugPrint("Transmit");
    debugPrint(" Status Vector: ");
    for (i = 0; i < 7; i++) {
        debugPrint(hex2ToString(statusVector[i]));
        if (i < 6)
            debugPrint(" ");
        else
            debugPrint("\n");
    }
#endif

    // Retransmit logic as described in silicon errata issue 13
    // Only needed for half duplex operation
    // for (i = 0; i < 15; i++) {
    // 	// if TXERIF and late collision
    // 	if ((readControlRegister(0x1C) & 0x02) && (statusVector[3] & 0x20)) {
    // 		debugPrint("Retransmitting packet...\n");
    // 		bitFieldSet(0x1F, 0x80); // Set ECON1.TXRST
    // 		bitFieldClear(0x1F, 0x80); // Clear ECON1.TXRST
    // 		bitFieldClear(0x1C, 0x0A); // Clear EIR.TXERIF & TXIF
    // 		bitFieldSet(0x1F, 0x08); // ECON1.TXRTS --> start transmission
    // 		while((readControlRegister(0x1C) & 0x0A) == 0); // Wait for TXIF & TXERIF
    // 		bitFieldClear(0x1F, 0x08); // Clear ECON1.TXRTS
    // 		a = TXSTART + 1 + p->dLength; // 1 Control byte in front
    // 		writeControlRegister(0x00, (uint8_t)(a & 0xFF)); // Set ERDPTL
    // 		writeControlRegister(0x01, (uint8_t)((a & 0xFF00) >> 8)); // Set ERDPTH
    // 		readBufferMemory(statusVector, 7); // Read status vector
    // 	} else {
    // 		break;
    // 	}
    // }

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

Packet *macGetPacket(void) { // Returns NULL or Packet with d == NULL on error
    // Read and store next packet pointer,
    // check receive status vector for errors, if they exist, throw packet away
    // else read packet, return it
    uint8_t d;
    uint8_t header[4];
    uint16_t fullLength;
    Packet *p;

    if (!macInitialized) {
        return NULL;
    }

    if (macPacketsReceived() < 1) {
        return NULL;
    }

    p = (Packet *)mmalloc(sizeof(Packet));
    if (p == NULL) {
        return NULL;
    }

    p->d = NULL;
    p->dLength = 0;

    writeControlRegister(0x00, (uint8_t)(nextPacketPointer & 0xFF)); // Set ERDPTL
    writeControlRegister(0x01, (uint8_t)((nextPacketPointer & 0xFF00) >> 8)); // Set ERDPTH

    readBufferMemory(&d, 1);
    nextPacketPointer = (uint16_t)d;
    readBufferMemory(&d, 1);
    nextPacketPointer |= ((uint16_t)d << 8);

    readBufferMemory(header, 4); // Read status vector
    fullLength = (uint16_t)header[0];
    fullLength |= (((uint16_t)header[1]) << 8);

#if DEBUG >= 2
    debugPrint("Received Packet with ");
    debugPrint(timeToString(fullLength));
    debugPrint(" bytes...\n");
#endif

#if DEBUG >= 4
    debugPrint("Receive");
    debugPrint(" Status Vector: ");
    for (d = 0; d < 4; d++) {
        debugPrint(hex2ToString(header[d]));
        if (d < 3)
            debugPrint(" ");
        else
            debugPrint("\n");
    }
#endif

    if (header[2] & (1 << 7)) {
        // Received OK
        assert(fullLength > 0);
        assert(fullLength <= MaxPacketSize);
        p->dLength = fullLength;
        p->d = (uint8_t *)mmalloc(p->dLength * sizeof(uint8_t));
        if (p->d == NULL) {
            // discardPacket(); // Try again later
            return p;
        }
        readBufferMemory(p->d, p->dLength); // Read payload
        discardPacket();
        return p;
    } else {
        discardPacket();
        return p; // p->d is NULL
    }
}

// ----------------------------------
// |      ENC28J60 Command Set      |
// ----------------------------------

#define A_BLOCK() ATOMIC_BLOCK(ATOMIC_RESTORESTATE)

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

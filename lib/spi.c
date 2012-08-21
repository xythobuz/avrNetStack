/*
 * spi.c
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
#include <stdint.h>
#include <avr/io.h>

#include <spi.h>

#include <serial.h> // debug output

void spiInit(void) {
#if defined(__AVR_ATmega168__)
	DDRB |= (1 << PB3) | (1 << PB5); // MOSI & SCK
#elif defined(__AVR_ATmega2560__)
	DDRB |= (1 << PB2) | (1 << PB1);
#elif defined(__AVR_ATmega32__)
	DDRB |= (1 << PB5) | (1 << PB7);
#else
#error MCU not supported by SPI module. DIY!
#endif

	SPCR |= (1 << MSTR) | (1 << SPE); // Enable SPI, Master mode
	SPSR |= (1 << SPI2X); // Double speed --> F_CPU/2
}

void spiSendByte(uint8_t d) {
	serialWriteString("SPI Send Byte()...");
	SPDR = d;
	while (!(SPSR & (1 << SPIF))); // Wait for transmission
	serialWriteString(" Done!\n");
}

uint8_t spiReadByte(void) {
	SPDR = 0x00; // Send 0 byte. While sending this a byte is recieved...
	while (!(SPSR & (1 << SPIF))); // Wait for transmission
	return SPDR;
}

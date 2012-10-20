/*
 * spi.c
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
#include <stdint.h>
#include <avr/io.h>

#include <spi.h>

void spiInit(void) {
#if defined(__AVR_ATmega168__)
    DDRB |= (1 << PB3) | (1 << PB5) | (1 << PB2); // MOSI & SCK & SS
#elif defined(__AVR_ATmega2560__)
    DDRB |= (1 << PB2) | (1 << PB1) | (1 << PB0);
#elif defined(__AVR_ATmega32__)
    DDRB |= (1 << PB5) | (1 << PB7) | (1 << PB4);
#else
#error MCU not supported by SPI module. DIY!
#endif

    SPCR |= (1 << MSTR) | (1 << SPE); // Enable SPI, Master mode
    SPSR |= (1 << SPI2X); // Double speed --> F_CPU/2
}

uint8_t spiSendByte(uint8_t d) {
    SPDR = d;
    while (!(SPSR & (1 << SPIF))); // Wait for transmission
    return SPDR;
}

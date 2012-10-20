/*
 * serial.h
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
#ifndef _serial_h
#define _serial_h

/* You always write Strings with '\n' (LF) as line ending.
 * If you define this, a '\r' (CR) will be put in front of the LF
 */
// #define SERIALINJECTCR

// RX & TX buffer size in bytes
#ifndef RX_BUFFER_SIZE
#define RX_BUFFER_SIZE 32
#endif

#ifndef TX_BUFFER_SIZE
#define TX_BUFFER_SIZE 16
#endif

#define BAUD(baudRate,xtalCpu) ((xtalCpu)/((baudRate)*16l)-1)

void serialInit(uint16_t baud);
void serialClose(void);

void setFlow(uint8_t on);

// Reception
uint8_t serialHasChar(void);
uint8_t serialGet(void); // Get a character
uint8_t serialGetBlocking(void);
uint8_t serialRxBufferFull(void); // 1 if full
uint8_t serialRxBufferEmpty(void); // 1 if empty

// Transmission
void serialWrite(uint8_t data);
void serialWriteString(const char *data);
uint8_t serialTxBufferFull(void); // 1 if full
uint8_t serialTxBufferEmpty(void); // 1 if empty

#if  defined(__AVR_ATmega8__) || defined(__AVR_ATmega16__) || defined(__AVR_ATmega32__) \
    || defined(__AVR_ATmega8515__) || defined(__AVR_ATmega8535__) \
|| defined(__AVR_ATmega323__)
#define SERIALRECIEVEINTERRUPT USART_RXC_vect
#define SERIALTRANSMITINTERRUPT USART_UDRE_vect
#define SERIALDATA UDR
#define SERIALB UCSRB
#define SERIALIE UDRIE
#define SERIALC UCSRC
#define SERIALUPM1 UPM1
#define SERIALUPM0 UPM0
#define SERIALUSBS USBS
#define SERIALUCSZ0 UCSZ0
#define SERIALUCSZ1 UCSZ1
#define SERIALUCSZ2 UCSZ2
#define SERIALRXCIE RXCIE
#define SERIALRXEN RXEN
#define SERIALTXEN TXEN
#define SERIALA UCSRA
#define SERIALUDRIE UDRIE
#define SERIALUDRE UDRE
#define SERIALBAUD8
#define SERIALUBRRH UBRRH
#define SERIALUBRRL UBRRL
#elif defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__) || defined(__AVR_ATmega1280__) \
    || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega640__)
// These definitions reflect the registers for the first UART. Change the 0s to 1s and you will use the second one.serial.c
#define SERIALRECIEVEINTERRUPT USART0_RXC_vect
#define SERIALTRANSMITINTERRUPT USART0_UDRE_vect
#define SERIALDATA UDR0
#define SERIALB UCSR0B
#define SERIALIE UDRIE0
#define SERIALC UCSR0C
#define SERIALUPM1 UPM01
#define SERIALUPM0 UPM00
#define SERIALUSBS USBS0
#define SERIALUCSZ0 UCSZ00
#define SERIALUCSZ1 UCSZ01
#define SERIALUCSZ2 UCSZ02
#define SERIALRXCIE RXCIE0
#define SERIALRXEN RXEN0
#define SERIALTXEN TXEN0
#define SERIALA UCSR0A
#define SERIALUDRIE UDRIE0
#define SERIALUDRE UDRE0
#define SERIALUBRR UBRR0
#elif defined(__AVR_ATmega168__)
#define SERIALRECIEVEINTERRUPT USART_RX_vect
#define SERIALTRANSMITINTERRUPT USART_UDRE_vect
#define SERIALDATA UDR0
#define SERIALB UCSR0B
#define SERIALIE UDRIE0
#define SERIALC UCSR0C
#define SERIALUPM1 UPM01
#define SERIALUPM0 UPM00
#define SERIALUSBS USBS0
#define SERIALUCSZ0 UCSZ00
#define SERIALUCSZ1 UCSZ01
#define SERIALUCSZ2 UCSZ02
#define SERIALRXCIE RXCIE0
#define SERIALRXEN RXEN0
#define SERIALTXEN TXEN0
#define SERIALA UCSR0A
#define SERIALUDRIE UDRIE0
#define SERIALUDRE UDRE0
#define SERIALUBRR UBRR0
#else
#error "AvrSerialLibrary not compatible with your MCU!"
#endif

#endif // _serial_h

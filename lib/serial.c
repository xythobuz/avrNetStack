/*
 * serial.c
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
#include <avr/interrupt.h>
#include <stdint.h>

#include <serial.h>

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

#ifdef SERIALNONBLOCK
uint8_t volatile rxBuffer[RX_BUFFER_SIZE];
uint8_t volatile txBuffer[TX_BUFFER_SIZE];
uint16_t volatile rxRead = 0;
uint16_t volatile rxWrite = 0;
uint16_t volatile txRead = 0;
uint16_t volatile txWrite = 0;
uint8_t volatile shouldStartTransmission = 1;

ISR(SERIALRECIEVEINTERRUPT) { // Receive complete
    rxBuffer[rxWrite] = SERIALDATA;
	if (rxWrite < (RX_BUFFER_SIZE - 1)) {
		rxWrite++;
	} else {
		rxWrite = 0;
	}
}

ISR(SERIALTRANSMITINTERRUPT) { // Data register empty
    if (txRead != txWrite) {
		SERIALDATA = txBuffer[txRead];
		if (txRead < (TX_BUFFER_SIZE -1)) {
			txRead++;
		} else {
			txRead = 0;
		}
	} else {
		shouldStartTransmission = 1;
		SERIALB &= ~(1 << SERIALUDRIE); // Disable Interrupt
	}
}
#endif

uint8_t serialInit(uint16_t baud, uint8_t databits, uint8_t parity, uint8_t stopbits) {
	if (parity > ODD) {
		return 1;
	}
	if ((databits < 5) || (databits > 8)) {
		return 1;
	}
	if ((stopbits < 1) || (stopbits > 2)) {
		return 1;
	}

	if (parity != NONE) {
		SERIALC |= (1 << SERIALUPM1);
		if (parity == ODD) {
			SERIALC |= (1 << SERIALUPM0);
		}
	}
	if (stopbits == 2) {
		SERIALC |= (1 << SERIALUSBS);
	}
	if (databits != 5) {
		if ((databits == 6) || (databits >= 8)) {
			SERIALC |= (1 << SERIALUCSZ0);
		}
		if (databits >= 7) {
			SERIALC |= (1 << SERIALUCSZ1);
		}
		if (databits == 9) {
			SERIALB |= (1 << SERIALUCSZ2);
		}
	}
#ifdef SERIALBAUD8
	SERIALUBRRH = (baud >> 8);
	SERIALUBRRL = baud;
#else
	SERIALUBRR = baud;
#endif

#ifdef SERIALNONBLOCK
	SERIALB |= (1 << SERIALRXCIE); // Enable Interrupts
#endif

	SERIALB |= (1 << SERIALRXEN) | (1 << SERIALTXEN); // Enable Receiver/Transmitter

	return 0;
}

uint8_t serialHasChar() {
#ifdef SERIALNONBLOCK
	if (rxRead != rxWrite) { // True if char available
#else
	if (SERIALA & RXC) {
#endif
		return 1;
	} else {
		return 0;
	}
}

uint8_t serialGet() {
#ifdef SERIALNONBLOCK
	uint8_t c;
	if (rxRead != rxWrite) {
		c = rxBuffer[rxRead];
		rxBuffer[rxRead] = 0;
		if (rxRead < (RX_BUFFER_SIZE - 1)) {
			rxRead++;
		} else {
			rxRead = 0;
		}
		return c;
	} else {
		return 0;
	}
#else
	while(!serialHasChar());
	return SERIALDATA;
#endif
}

uint8_t serialBufferSpaceRemaining() {
#ifdef SERIALNONBLOCK
	return (((txWrite + 1) == txRead) || ((txRead == 0) && ((txWrite + 1) == TX_BUFFER_SIZE)));
#else
	return 1;
#endif
}

void serialWrite(uint8_t data) {
#ifdef SERIALNONBLOCK
	while (((txWrite + 1) == txRead) || ((txRead == 0) && ((txWrite + 1) == TX_BUFFER_SIZE))); // Buffer is full, wait!
    txBuffer[txWrite] = data;
	if (txWrite < (TX_BUFFER_SIZE - 1)) {
		txWrite++;
	} else {
		txWrite = 0;
	}
	if (shouldStartTransmission == 1) {
		shouldStartTransmission = 0;
		SERIALB |= (1 << SERIALUDRIE); // Enable Interrupt
		SERIALA |= (1 << SERIALUDRE); // Trigger Interrupt
	}
#else
	while (!(SERIALA & (1 << UDRE))); // Wait for empty buffer
	SERIALDATA = data;
#endif
}

void serialWriteString(const char *data) {
	while (*data != '\0') {
		serialWrite(*data++);
	}
}

uint8_t transmitBufferEmpty(void) {
#ifdef SERIALNONBLOCK
	if (txRead != txWrite) {
		return 1;
	} else {
		return 0;
	}
#else
	return 1;
#endif
}

void serialClose() {
	SERIALB = 0;
	SERIALC = 0;
#ifdef SERIALBAUD8
	SERIALUBRRH = 0;
	SERIALUBRRL = 0;
#else
	SERIALUBRR = 0;
#endif
#ifdef SERIALNONBLOCK
	rxRead = 0;
	txRead = 0;
	rxWrite = 0;
	txWrite = 0;
#endif
}

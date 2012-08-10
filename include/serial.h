/*
 * serial.h
 *
 * Copyright 2011 Thomas Buck <xythobuz@me.com>
 *
 * This file is part of AvrSerialLibrary.
 *
 * AvrSerialLibrary is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AvrSerialLibrary is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with AvrSerialLibrary.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef serial_h_
#define serial_h_

// RX & TX buffer size in bytes
#define RX_BUFFER_SIZE 32
#define TX_BUFFER_SIZE 32

// Select Baudrate with this macro
#define BAUD(baudRate,xtalCpu) ((xtalCpu)/((baudRate)*16l)-1)

#define ODD 2
#define EVEN 1
#define NONE 0

/* uint8_t serialInit(uint16_t baud, uint8_t databits, uint8_t parity, uint8_t stopbits)
 *	- baud:
 *		Use the BAUD(baudrate,fosc) Macro here
 *	- databits:
 *		Number between 5 and 8
 *	- parity:
 *		Use Definitions ODD, EVEN or NONE
 *	- stopbits:
 *		1 or 2
 *
 * returns 0 if success, 1 if argument invalid.
 */
uint8_t serialInit(uint16_t baud, uint8_t databits, uint8_t parity, uint8_t stopbits);

/* uint8_t serialHasChar(void)
 *
 * returns 0 if there are no characters available, 1 otherwise.
 */
uint8_t serialHasChar(void);

/* uint8_t serialGet(void)
 *
 * returns next available char. If there is none, it blocks.
 * define SERIALNONBLOCK in this file to let it return 0 if there are no chars available.
 */
uint8_t serialGet(void);

/* uint8_t serialBufferSpaceRemaining(void)
 *
 * returns 0 if you can't store another char to transmit.
 */
uint8_t serialBufferSpaceRemaining(void); // 0 if full

/* void serialWrite(uint8_t data)
 *
 * Blocks if the buffer is full!
 */
void serialWrite(uint8_t data);

/* void serialWriteString(char *data)
 *
 * Blocks if the buffer is full!
 */
void serialWriteString(char *data);

/* void serialClose(void)
 *
 * Restore normal port operation and reset buffers.
 */
void serialClose(void);

#endif /* SERIAL_H_ */

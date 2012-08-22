/*
 * mac.h
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

/*
 * This file defines the standard API implemented by different drivers
 * for eg. the ENC28J60
 */
#ifndef _mac_h
#define _mac_h

typedef uint8_t MacAddress[6];

#include <net/controller.h>

#define MACPreambleSize 0x0E

/* typedef struct {
	MacAddress destination;
	MacAddress source;
	uint16_t   typeLength; // type/length field in packet

	uint8_t    *data;
	uint16_t   dLength; // real length of data[]
} MacPacket; */

extern MacAddress ownMacAddress;

uint8_t macInitialize(MacAddress address); // 0 if success, 1 on error
void    macReset(void);
uint8_t macLinkIsUp(void); // 0 if down, 1 if up

uint8_t macSendPacket(Packet p); // 0 on success, 1 on PHY error
// p is freed afterwards

uint8_t macPacketsReceived(void); // number of packets ready
Packet macGetPacket(void);

#endif

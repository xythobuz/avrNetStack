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

#include <net/controller.h>

#define MACPreambleSize 14
#define MACDestinationOffset 0
#define MACSourceOffset 6
#define MACTypeOffset 12

#define MaxPacketSize 1518 // Max EthernetII Packet Size

extern uint8_t ownMacAddress[6];

uint8_t macInitialize(uint8_t *address); // 0 if success, 1 on error
void    macReset(void);
uint8_t macLinkIsUp(void); // 0 if down, 1 if up

uint8_t macSendPacket(Packet *p); // 0 on success, 1 on PHY error

uint8_t macPacketsReceived(void); // number of packets ready
Packet *macGetPacket(void);

uint8_t macHasInterrupt(void);

#endif

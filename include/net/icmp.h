/*
 * icmp.h
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
#ifndef _icmp_h
#define _icmp_h

#include <net/ipv4.h>

typedef struct {
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint32_t restOfHeader;
	uint8_t *data; // Optional
	uint16_t dLength;
} IcmpPacket;

void icmpInit(void);

// 0 success, 1 not enough mem, 2 invalid
// ip freed afterwards
uint8_t icmpProcessPacket(IPv4Packet *ip);

// 0 on success, 1 if destination unknown, try again later.
// 2 or 4 if there was not enough RAM. 3 on PHY Error
// On Return 0, 1, 2 and 3, ic was already freed
uint8_t icmpSendPacket(IcmpPacket *ic, IPv4Address target);

#ifndef DISABLE_ICMP_ERROR_STRINGS
void icmpRegisterMessageCallback(void (*debugOutput)(char *));
char *icmpMessage(uint8_t type, uint8_t code);
#endif

#endif

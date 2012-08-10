/*
 * arp.h
 *
 * Copyright 2012 Thomas Buck <xythobuz@me.com>
 *
 * This file is part of avrNetStack.
 *
 * avrNetStack is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * avrNetStack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with avrNetStack.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _arp_h
#define _arp_h

#include <net/mac.h>
#include <net/ipv4.h>

#define ARPTableSize 6

typedef struct {
	IPv4Address ip;
	MacAddress mac;
	uint64_t time;
} ARPTableEntry;

ARPTableEntry arpTable[ARPTableSize];

void arpInit(void);
void arpProcessPacket(MacPacket *p);

#endif

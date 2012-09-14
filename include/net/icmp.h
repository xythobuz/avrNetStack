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

#include <net/mac.h>
#include <net/ipv4.h>

#define ICMPOffset (MACPreambleSize + IPv4PacketHeaderLength)
#define ICMPPacketSize 4
#define ICMPTypeOffset 0
#define ICMPCodeOffset 1
#define ICMPChecksumOffset 2
#define ICMPDataOffset 4

void icmpInit(void);

// 0 success, 1 not enough mem, 2 invalid
// p freed afterwards
uint8_t icmpProcessPacket(Packet *p);

#endif

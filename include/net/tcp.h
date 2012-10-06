/*
 * tcp.h
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
#ifndef _tcp_h
#define _tcp_h

#include <net/mac.h>
#include <net/ipv4.h>
#include <net/controller.h>

#define TCPPacketLength 20
#define TCPOffset (MACPreambleSize + IPv4PacketHeaderLength)
#define TCPSourceOffset 0 // 16bit
#define TCPDestinationOffset 2 // 16bit
#define TCPSeqOffset 4 // 32bit
#define TCPAckOffset 8 // 32bit
#define TCPOffsetOffset 12 // 4bit, High Nibble only! In 32bit words 
#define TCPFlagsOffset 13 // 8bit
#define TCPWindowOffset 14 // 16bit
#define TCPChecksumOffset 16 // 16bit
#define TCPUrgentOffset 18 // 16bit
#define TCPDataOffset 20 // Data, or Options if Data Offset > 5

void tcpInit(void);

// 0 on success, 1 if not enough mem, 2 invalid
uint8_t tcpHandlePacket(Packet *p);

// Overwrites existing handler for this port
// 0 on succes, 1 on not enough RAM
// Handler has to free the Packet!
uint8_t tcpRegisterHandler(uint8_t (*handler)(Packet *), uint16_t port);

// Allocate large enough Packet, fill in data, then call this.
// TCP Header will be filled, then the Packet goes into the
// IPv4 Transmission Buffer...
uint8_t tcpSendPacket(Packet *p, uint8_t *targetIp, uint16_t targetPort, uint16_t sourcePort);

#endif

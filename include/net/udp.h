/*
 * udp.h
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
#ifndef _udp_h
#define _udp_h

#include <net/mac.h>
#include <net/ipv4.h>
#include <net/controller.h>

#define UDPOffset (MACPreambleSize + IPv4PacketHeaderLength)
#define UDPSourceOffset 0
#define UDPDestinationOffset 2
#define UDPLengthOffset 4
#define UDPChecksumOffset 6
#define UDPDataOffset 8

void udpInit(void);

// 0 on success, 1 if not enough mem, 2 invalid
uint8_t udpHandlePacket(Packet *p);

// Overwrites existing handler for this port
// 0 on succes, 1 on not enough RAM
// Handler has to free the UdpPacket!
uint8_t udpRegisterHandler(uint8_t (*handler)(Packet *), uint16_t port);

// Allocate large enough Packet, fill in data, then call this.
// UDP Header will be filled, then the Packet goes into the
// IPv4 Transmission Buffer...
uint8_t udpSendPacket(Packet *p, uint8_t *targetIp, uint16_t targetPort, uint16_t sourcePort);

#endif

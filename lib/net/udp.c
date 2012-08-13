/*
 * udp.c
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
#include <stdint.h>
#include <stdlib.h>

#include <net/udp.h>

void udpInit(void) {}

// 0 on success, 1 not enough mem, 2 invalid
uint8_t udpHandlePacket(IPv4Packet *ip) {
	return 1;
}

uint8_t udpSendPacket(UdpPacket *up) {
	return 1;
}

// Overwrites existing handler for this port
// 0 on succes, 1 on not enough RAM
uint8_t udpRegisterHandler(void (*handler)(UdpPacket *), uint16_t port) {
	return 1;
}

/*
 * dhcp.c
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

#include <std.h>
#include <net/udp.h>
#include <time.h>
#include <net/dhcp.h>

#define DHCPHeader1Length 0xF0
#define DHCPHeader2Length 0xE8

uint8_t isEqualMem(uint8_t *d1, uint8_t *d2, uint8_t l);

IPv4Address dest = {255, 255, 255, 255};
IPv4Address server = {0, 0, 0, 0};
IPv4Address self = {0, 0, 0, 0};
uint8_t acked = 0;

// 0 on success, 1 no mem, 2 invalid
uint8_t dhcpHandler(Packet p) {
	return 0;
}

uint8_t dhcpIssueRequest(void) {
	return 0;
}

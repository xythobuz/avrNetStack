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

#include <net/icmp.h>
#include <net/utils.h>
#include <net/udp.h>

#ifndef DISABLE_UDP

uint8_t isBroadcastIp(uint8_t *d);

typedef struct {
	uint16_t port;
	uint8_t (*func)(Packet);
} UdpHandler;

UdpHandler *handlers = NULL;
uint16_t registeredHandlers = 0;
IPv4Address target;
// --------------------------
// |      Internal API      |
// --------------------------

int16_t findHandler(uint16_t port) {
	uint16_t i;
	if (handlers != NULL) {
		for (i = 0; i < registeredHandlers; i++) {
			if (handlers[i].port == port) {
				return i;
			}
		}
	}
	return -1;
}

#ifndef DISABLE_UDP_CHECKSUM

#endif

// --------------------------
// |      External API      |
// --------------------------

void udpInit(void) {}

// 0 on success, 1 not enough mem, 2 invalid
// 3 on success, 5 or 7 on not enough mem, 6 on PHY error
// 4 on unknown destination, try again later
// ip is always freed!
uint8_t udpHandlePacket(Packet p) {
		return 0;
}


// Overwrites existing handler for this port
// 0 on succes, 1 on not enough RAM
uint8_t udpRegisterHandler(uint8_t (*handler)(Packet), uint16_t port) {
	uint16_t i;

	// Check if port is already in list
	for (i = 0; i < registeredHandlers; i++) {
		if (handlers[i].port == port) {
			handlers[i].func = handler;
			return 0;
		}
	}

	// Extend list, add new handler.
	UdpHandler *tmp = (UdpHandler *)realloc(handlers, (registeredHandlers + 1) * sizeof(UdpHandler));
	if (tmp == NULL) {
		return 1;
	}
	handlers = tmp;
	handlers[registeredHandlers].port = port;
	handlers[registeredHandlers].func = handler;
	registeredHandlers++;
	return 0;
}

#endif // DISABLE_UDP

/*
 * enc28j60.c
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
#include <avr/io.h>
#include <stdint.h>
#include <stdlib.h>

#include "../../include/mac.h"

uint8_t macInitialize(MacAddress address) {
	return 1;
}

void macReset(void) {

}

uint8_t macLinkIsUp(void) {
	return 0;
}

uint8_t macSendPacket(MacPacket p) {
	return 1;
}

uint8_t macPacketsRecieved(void) {
	return 0;
}

MacPacket* macGetPacket(void) {
	return NULL;
}

/*
 * mac.h
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

/*
 * This file defines the standard API implemented by different drivers
 * for eg. the ENC28J60
 */

typedef uint8_t MacAddress[6];

typedef struct {
	MacAddress destination;
	MacAddress source;
	uint16_t   length;
	uint8_t    *data;
} MacPacket;

uint8_t    macInitialize(MacAddress address);
void       macReset(void);
uint8_t    macLinkIsUp(void);

uint8_t    macSendPacket(MacPacket p);

uint8_t    macPacketsRecieved(void);
MacPacket* macGetPacket(void);

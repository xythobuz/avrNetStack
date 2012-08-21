/*
 * main.c
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
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include <net/controller.h>
#include <net/mac.h>
#include <net/icmp.h>
#include <net/dhcp.h>
#include <net/ntp.h>
#include <net/arp.h>
#include <time.h>
#include <serial.h>

char *getString(uint8_t id);

// Thats, the MAC of my WLAN Module, with some bytes swapped...
MacAddress mac = {0x00, 0x1E, 0x99, 0x02, 0xC0, 0x42};

void printArpTable(void) {
	uint8_t i, j;
	serialWriteString(getString(12));
	for (i = 0; i < arpTableSize; i++) {
		for (j = 0; j < 6; j++) {
			serialWriteString(hex2ToString(arpTable[i].mac[j]));
			if (j < 5) {
				serialWrite('-');
			}
		}
		serialWriteString(getString(13));
		for (j = 0; j < 4; j++) {
			serialWriteString(timeToString(arpTable[i].ip[j]));
			if (j < 3) {
				serialWrite('.');
			}
		}
		serialWrite('\n');
	}
}

void icmpCallBack(char *s) {
	serialWriteString(getString(5));
	serialWriteString(s);
	serialWrite('\n');
}

int main(void) {
	char c;
	uint8_t i;
	uint16_t j;

	MCUSR = 0;
	wdt_disable();

	serialInit(BAUD(39400, F_CPU), 8, NONE, 1);

	DDRA = 0xC0;
	PORTA |= 0xC0; // LEDs on

	sei(); // Enable Interrupts so we get UART data before entering networkInit

	networkInit(mac);
	icmpRegisterMessageCallback(icmpCallBack);

	PORTA &= ~(0xC0); // LEDs off

	serialWriteString(getString(0));
	serialWriteString(getString(1));

	if (!macLinkIsUp()) {
		serialWriteString(getString(2));
	} else {
		serialWriteString(getString(3));
	}

	wdt_enable(WDTO_2S);

	while(1) {
		wdt_reset();
		i = networkHandler();
		if (i != 255) {
			if (i != 0) {
				serialWriteString(getString(4));
				serialWriteString(timeToString(i));
				serialWrite('\n');
			}
			
			j = networkLastProtocol();
			if (j != ARP) {
				serialWriteString(getString(6));
				if (j == IPV4) {
					serialWriteString(getString(7));
				} else {
					serialWriteString(hexToString(j));
				}
				serialWrite('\n');
			}
		}

		if (serialHasChar()) {
			c = serialGet();
			switch(c) {
				case 'a':
					printArpTable();
					break;
				case 'n':
					i = ntpIssueRequest();
					serialWriteString(getString(8));
					serialWriteString(timeToString(i));
					serialWrite('\n');
					break;
				case 'd':
					i = dhcpIssueRequest();
					serialWriteString(getString(9));
					serialWriteString(timeToString(i));
					serialWrite('\n');
					break;
				case 'h':
					serialWriteString(getString(10));
					break;
				case 'v':
					serialWriteString(getString(0));
					serialWrite('\n');
					break;
				case 'q':
					serialWriteString(getString(11));
					wdt_enable(WDTO_15MS);
					while(1);
				default:
					serialWrite(c);
					break;
			}
		}

		PORTA ^= (1 << PA6);
	}

	return 0;
}

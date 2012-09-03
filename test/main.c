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
#include <util/delay.h>

#include <std.h>
#include <time.h>
#include <serial.h>
#include <scheduler.h>
#include <tasks.h>

#include <net/mac.h>
#include <net/icmp.h>
#include <net/dhcp.h>
#include <net/ntp.h>
#include <net/arp.h>
#include <net/udp.h>
#include <net/controller.h>

char *getString(uint8_t id);
void printArpTable(void);
void heartbeat(void);
void serialHandler(void);

// Thats, the MAC of my WLAN Module, with some bytes swapped...
MacAddress mac = {0x00, 0x1E, 0x99, 0x02, 0xC0, 0x42};
IPv4Address defIp = {192, 168, 0, 42};
IPv4Address defSubnet = {255, 255, 255, 0};
IPv4Address defGateway = {192, 168, 0, 1};
IPv4Address testIp = { 192, 168, 0, 103 };

int main(void) {
	uint8_t i;

	i = MCUSR & 0x1F;
	MCUSR = 0;
	wdt_disable();

	serialInit(BAUD(39400, F_CPU), 8, NONE, 1);

	DDRA |= (1 << PA7) | (1 << PA6);
	PORTA |= (1 << PA7) | (1 << PA6); // LEDs on

	sei(); // Enable Interrupts so we get UART data before entering networkInit

	networkInit(mac, defIp, defSubnet, defGateway);

	serialWriteString(getString(0));
	serialWriteString(getString(1));
	serialWriteString(getString(5));
	if (i == 0x01) {
		serialWriteString(getString(20));
	} else if (i == 0x02) {
		serialWriteString(getString(21));
	} else if (i == 0x04) {
		serialWriteString(getString(22));
	} else if (i == 0x08) {
		serialWriteString(getString(23));
	} else if (i == 0x10) {
		serialWriteString(getString(24));
	} else {
		serialWriteString(hexToString(i));
	}
	serialWrite('\n');

	if (!macLinkIsUp()) {
		serialWriteString(getString(2)); // Link is down
		serialWriteString(getString(17)); // Waiting
		while(!macLinkIsUp());
	}
	serialWriteString(getString(3)); // Link is up

	wdt_enable(WDTO_2S);

	arpGetMacFromIp(testIp); // Request MAC for serial command 't'

	PORTA &= ~((1 << PA7) | (1 << PA6)); // LEDs off

	addTimedTask(heartbeat, 500); // Toggle LED every 500ms
	// Execute Serial Handler if a char was received
	addConditionalTask(serialHandler, serialHasChar);

	while(1) {
		wdt_reset();
		scheduler();
		wdt_reset();
		tasks();
	}

	return 0;
}

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

void heartbeat(void) {
	PORTA ^= (1 << PA6); // Toggle LED
}

void serialHandler(void) {
	uint8_t i;
	uint8_t *p;
	char c = serialGet();
	serialWrite(c - 32); // to uppercase
	serialWriteString(getString(7)); // ": "
	switch(c) {
		case 'm':
			serialWriteString(timeToString(heapBytesAllocated));
			serialWriteString(getString(4)); // " bytes "
			serialWriteString(getString(6)); // "allocated\n"
			break;
		case 't':
			if ((p = arpGetMacFromIp(testIp)) == NULL) {
				serialWriteString(getString(18));
				break;
			}
			serialWriteString(getString(19));
			for (i = 0; i < 6; i++) {
				serialWriteString(hex2ToString(p[i]));
				if (i < 5) {
					serialWrite(':');
				} else {
					serialWrite('\n');
				}
			}
			break;
		case 'l':
			if (macLinkIsUp()) {
				serialWriteString(getString(3));
			} else {
				serialWriteString(getString(2));
			}
			break;
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
			serialWrite('\n');
			break;
	}
}

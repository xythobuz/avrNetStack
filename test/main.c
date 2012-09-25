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

// Thats the MAC of my WLAN Module, with some bytes swapped...
MacAddress mac = {0x02, 0x1E, 0x99, 0x02, 0xC0, 0x42};
IPv4Address defIp = {192, 168, 0, 42};
IPv4Address defSubnet = {255, 255, 255, 0};
IPv4Address defGateway = {192, 168, 0, 1};

IPv4Address testIp = { 192, 168, 0, 103 };
#define TESTPORT 6600

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

	PORTA &= ~((1 << PA7) | (1 << PA6)); // LEDs off

	addTimedTask(heartbeat, 500); // Toggle LED every 500ms
	addConditionalTask(serialHandler, serialHasChar); // Execute Serial Handler if char received

	while(1) {
		// Run the tasks
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

#define IDLE 0
#define PINGED 1
uint8_t pingState = IDLE;
time_t pingTime, responseTime;
IPv4Address pingIpA = { 192, 168, 0, 103 };
IPv4Address pingIpB = { 80, 150, 6, 143 };

void pingInterrupt(Packet *p) {
	responseTime = getSystemTime();
	mfree(p->d, p->dLength);
	mfree(p, sizeof(Packet));
	pingState = IDLE;
	registerEchoReplyHandler(NULL);
	serialWriteString(getString(19)); // "RoundTripTime"
	serialWriteString(getString(7)); // ": "
	serialWriteString(timeToString(diffTime(pingTime, responseTime)));
	serialWriteString(getString(18)); // " ms"
	serialWriteString(getString(15)); // "\n"
}

void pingTool(void) {
	uint8_t c;
	if (pingState == PINGED) {
		// Check if we got a timeout
		if (diffTime(getSystemTime(), pingTime) > 1000) {
			serialWriteString(getString(31)); // "Timed out :(\n"
			pingState = IDLE;
			registerEchoReplyHandler(NULL);
		} else {
			serialWriteString(getString(32)); // "Hasn't timed out yet!\n"
		}
	} else { // IDLE
		// Send an Echo Request to pingIp
		serialWriteString(getString(30)); // "(1)Internal or (2)External?\n"
		registerEchoReplyHandler(pingInterrupt);
		while (!serialHasChar()) { wdt_reset(); }
		c = serialGet();
		if ((c == '1') || (c == 'a')) {
			sendEchoRequest(pingIpA);
		} else {
			sendEchoRequest(pingIpB);
		}
		pingTime = getSystemTime();
		responseTime = 0;
		pingState = PINGED;
	}
}

void serialHandler(void) {
	uint8_t i;
	Packet *p;
	
	char c = serialGet();
	serialWrite(c - 32); // to uppercase
	serialWriteString(getString(28)); // " - "
	serialWriteString(timeToString(getSystemTimeSeconds()));
	serialWriteString(getString(7)); // ": "
	switch(c) {
		case 'p': // Ping Internet
			pingTool();
			break;
		case 's': // Status
			serialWriteString(timeToString(tasksRegistered()));
			serialWriteString(getString(14)); // " Tasks"
			serialWriteString(getString(25)); // ", "
			serialWriteString(timeToString(schedulerRegistered()));
			serialWriteString(getString(16)); // " Scheduler"
			serialWriteString(getString(15)); // "\n"
			break;

		case 'm': // Number of bytes allocated
			serialWriteString(timeToString(heapBytesAllocated));
			serialWriteString(getString(4)); // " bytes "
			serialWriteString(getString(6)); // "allocated\n"
			break;

		case 'u': // Send UDP Packet to testIp
			if ((p = (Packet *)mmalloc(sizeof(Packet))) != NULL) {
				p->dLength = UDPOffset + UDPDataOffset + 12; // "Hello World."
				p->d = (uint8_t *)mmalloc(p->dLength);
				if (p->d == NULL) {
					serialWriteString(getString(26)); // "Not enough memory!\n"
					mfree(p, sizeof(Packet));
					break;
				}
				p->d[UDPOffset + UDPDataOffset + 0] = 'H';
				p->d[UDPOffset + UDPDataOffset + 1] = 'e';
				p->d[UDPOffset + UDPDataOffset + 2] = 'l';
				p->d[UDPOffset + UDPDataOffset + 3] = 'l';
				p->d[UDPOffset + UDPDataOffset + 4] = 'o';
				p->d[UDPOffset + UDPDataOffset + 5] = ' ';
				p->d[UDPOffset + UDPDataOffset + 6] = 'W';
				p->d[UDPOffset + UDPDataOffset + 7] = 'o';
				p->d[UDPOffset + UDPDataOffset + 8] = 'r';
				p->d[UDPOffset + UDPDataOffset + 9] = 'l';
				p->d[UDPOffset + UDPDataOffset + 10] = 'd';
				p->d[UDPOffset + UDPDataOffset + 11] = '.';
				serialWriteString(getString(27)); // "Packet sent"
				serialWriteString(getString(7)); // ": "
				serialWriteString(timeToString(udpSendPacket(p, testIp, TESTPORT, TESTPORT)));
				serialWriteString(getString(15)); // "\n"
			} else {
				serialWriteString(getString(26)); // "Not enough memory!\n"
			}
			break;

		case 'l': // Link Status
			if (macLinkIsUp()) {
				serialWriteString(getString(3));
			} else {
				serialWriteString(getString(2));
			}
			break;

		case 'a': // ARP Table
			printArpTable();
			break;

		case 'n': // Send NTP Request
			i = ntpIssueRequest();
			serialWriteString(getString(8));
			serialWriteString(timeToString(i));
			serialWrite('\n');
			break;

		case 'd': // Send DHCP Request
			i = dhcpIssueRequest();
			serialWriteString(getString(9));
			serialWriteString(timeToString(i));
			serialWrite('\n');
			break;

		case 'h': // Print Help String
			serialWriteString(getString(10));
			break;

		case 'v': // Print Version String
			serialWriteString(getString(0));
			serialWrite('\n');
			break;

		case 'q': // Trigger Watchdog Reset
			serialWriteString(getString(11));
			while(!transmitBufferEmpty()) {
				wdt_reset();
			}
			wdt_enable(WDTO_15MS);
			while(1);

		default:
			serialWriteString(getString(29));
			break;
	}
}

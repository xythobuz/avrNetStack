/*
 * main.c
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
#include <avr/io.h>
#include <stdint.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include <std.h>
#include <time.h>
#include <serial.h>
#include <scheduler.h>
#include <tasks.h>

#include <net/mac.h>
#include <net/ipv4.h>
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

uint8_t mac[6] = {0x00, 0x04, 0xA3, 0x00, 0x00, 0x00};
IPv4Address defIp = {192, 168, 0, 42};
IPv4Address defSubnet = {255, 255, 255, 0};
IPv4Address defGateway = {192, 168, 0, 1};

IPv4Address testIp = {192, 168, 0, 103};
#define TESTPORT 6600

uint8_t pingState = 0, pingMode = 0;
time_t pingTime, responseTime;
IPv4Address pingIpA = {192, 168, 0, 103};
IPv4Address pingIpB = {80, 150, 6, 143};

uint8_t mcusr_mirror __attribute__ ((section(".noinit")));
void mirrorWatchdog(void) __attribute__((naked)) __attribute__((section(".init3")));
void mirrorWatchdog(void) {
    mcusr_mirror = MCUSR;
    MCUSR = 0;
    wdt_disable();
}

int main(void) {
    uint8_t i;

    i = mcusr_mirror & 0x1F;

    serialInit(BAUD(38400, F_CPU));
    initSystemTimer();

    DDRA |= (1 << PA7) | (1 << PA6);
    PORTA |= (1 << PA7) | (1 << PA6); // LEDs on

    sei(); // Enable Interrupts so we get UART data before entering networkInit

    wdt_enable(WDTO_2S);

    networkInit(mac, defIp, defSubnet, defGateway);

    serialWriteString(getString(0)); // avrNetStack-Debug
    serialWriteString(getString(1)); //  initialized!\n
    serialWriteString(getString(5)); // MCUCSR:
    if (i == 0x01) {
        serialWriteString(getString(20)); // Power-On Reset
    } else if (i == 0x02) {
        serialWriteString(getString(21)); // External Reset
    } else if (i == 0x04) {
        serialWriteString(getString(22)); // Brown-Out Reset
    } else if (i == 0x08) {
        serialWriteString(getString(23)); // Watchdog Reset
    } else if (i == 0x10) {
        serialWriteString(getString(24)); // JTAG Reset
    } else {
        serialWriteString(hexToString(i));
    }
    serialWrite('\n');

    PORTA &= ~((1 << PA7) | (1 << PA6)); // LEDs off

    addTimedTask(heartbeat, 500, 1); // Toggle LED every 500ms
    addTask(serialHandler, serialHasChar, "Serial"); // Execute Serial Handler if char received

    while (1)
        networkLoop(); // Runs task manager and scheduler, resets watchdog timer for us

    return 0;
}

void printArpTable(void) {
    uint8_t i;
    ARPTableEntry *p = arpTable;
    serialWriteString(getString(12));
    while (p != NULL) {
        for (i = 0; i < 6; i++) {
            serialWriteString(hex2ToString(p->mac[i]));
            if (i < 5) {
                serialWrite('-');
            }
        }
        serialWriteString(getString(13));
        for (i = 0; i < 4; i++) {
            serialWriteString(timeToString(p->ip[i]));
            if (i < 3) {
                serialWrite('.');
            }
        }
        serialWrite('\n');
        p = p->next;
    }
}

void heartbeat(void) {
    PORTA ^= (1 << PA6); // Toggle LED
}

void pingInterrupt(Packet *p) {
    responseTime = getSystemTime();
    mfree(p->d, p->dLength);
    mfree(p, sizeof(Packet));

    serialWriteString(getString(19)); // "RoundTripTime"
    serialWriteString(getString(7)); // ": "
    serialWriteString(timeToString(diffTime(pingTime, responseTime)));
    serialWriteString(getString(18)); // " ms"
    serialWrite('\n');

    pingState--;
    if (pingState == 0) {
        // Finished pinging
        registerEchoReplyHandler(NULL);
    } else {
        // Ping again
        if (pingMode) {
            sendEchoRequest(pingIpB);
        } else {
            sendEchoRequest(pingIpA);
        }
        pingTime = getSystemTime();
    }
}

void pingTool(void) {
    uint8_t c;
    if (pingState) {
        // Check if we got a timeout
        if (diffTime(getSystemTime(), pingTime) > 2000) {
            serialWriteString(getString(31)); // "Timed out :(\n"
            pingState = 0;
            registerEchoReplyHandler(NULL);
        } else {
            serialWriteString(getString(32)); // "Hasn't timed out yet!\n"
        }
    } else {
        // Send an Echo Request
        serialWriteString(getString(30)); // "(1)Internal or (2)External?\n"
        while (!serialHasChar()) { wdt_reset(); }
        c = serialGet();
        if (c == '1') {
            pingMode = 0;
        } else if (c == '2') {
            pingMode = 1;
        } else {
            serialWriteString(getString(34)); // "Invalid!\n"
            return;
        }
        serialWriteString(getString(33)); // "How many times? (0 - 9)\n"
        while (!serialHasChar()) { wdt_reset(); }
        c = serialGet();
        if ((c >= '0') && (c <= '9')) {
            pingState = c - '0';
            registerEchoReplyHandler(pingInterrupt);
            if (pingMode) {
                sendEchoRequest(pingIpB);
            } else {
                sendEchoRequest(pingIpA);
            }
            pingTime = getSystemTime();
            responseTime = 0;
        } else {
            serialWriteString(getString(34)); // "Invalid!\n"
            return;
        }
    }
}

void serialHandler(void) {
    uint8_t i, j, k, l, m;
    uint16_t n;
    Packet *p;

    char c = serialGet();
    serialWrite(c - 32); // to uppercase
    serialWriteString(getString(7)); // ": "
    switch(c) {
        case 'r':
            macInitialize(mac);
            serialWriteString(getString(40)); // "MAC reinitialized!\n"
            break;
        case 'i':
            serialWriteString(getString(15)); // "Pin is "
            if (!macHasInterrupt()) {
                serialWriteString(getString(41)); // "High"
            } else {
                serialWriteString(getString(42)); // "Low"
            }
            serialWrite('\n');
            break;
        case 't': // Time
            convertTimestamp(getSystemTimeSeconds(), &n, &m, &l, &k, &j, &i);
            serialWriteString(timeToString(l)); // day
            serialWrite('.');
            serialWriteString(timeToString(m)); // month
            serialWrite('.');
            serialWriteString(timeToString(n)); // year
            serialWrite(' ');
            serialWriteString(timeToString(k)); // hour
            serialWrite(':');
            serialWriteString(timeToString(j)); // minutes
            serialWrite(':');
            serialWriteString(timeToString(i)); // seconds
            serialWrite('\n');
            break;
        case 'p': // Ping Internet
            pingTool();
            break;
        case 's': // Status
            serialWriteString(timeToString(tasksRegistered()));
            serialWriteString(getString(14)); // " Tasks"
            serialWriteString(getString(25)); // ", "
            serialWriteString(timeToString(schedulerRegistered()));
            serialWriteString(getString(16)); // " Scheduler"
            serialWrite('\n');
            serialWriteString(timeToString(heapBytesAllocated));
            serialWrite('/');
            serialWriteString(timeToString(HEAPSIZE));
            serialWriteString(getString(4)); // " bytes "
            serialWriteString(getString(6)); // "allocated\n"
            serialWriteString(timeToString(ipv4PacketsInQueue()));
            serialWriteString(getString(36)); // " IPv4 Packets in Queue\n"
            serialWriteString(timeToString(udpRegisteredHandlers));
            serialWrite(' ');
            serialWriteString(getString(38)); // "UDP"
            serialWriteString(getString(39)); // " Handlers registered\n"
            printArpTable();
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
                serialWrite('\n');
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

#ifndef DISABLE_NTP
        case 'n': // Send NTP Request
            i = ntpIssueRequest();
            serialWriteString(getString(8));
            serialWriteString(timeToString(i));
            serialWrite('\n');
            break;
#endif

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
            while(!serialTxBufferEmpty()) {
                wdt_reset();
            }
            wdt_enable(WDTO_15MS);
            while(1);

        default:
            serialWriteString(getString(29));
            break;
    }
}

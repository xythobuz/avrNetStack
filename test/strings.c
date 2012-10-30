/*
 * strings.c
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
#include <stdio.h>
#include <stdlib.h>
#include <avr/pgmspace.h>

#include <serial.h>
#include <net/controller.h>

const char string0[] PROGMEM = "avrNetStack-Debug";
const char string1[] PROGMEM = " initialized!\n";
const char string2[] PROGMEM = "Link is down!\n";
const char string3[] PROGMEM = "Link is up!\n";
const char string4[] PROGMEM = " bytes ";
const char string5[] PROGMEM = "MCUCSR: ";
const char string6[] PROGMEM = "allocated\n";
const char string7[] PROGMEM = ": ";
const char string8[] PROGMEM = "NTP Request: ";
const char string9[] PROGMEM = "DHCP Request: ";
const char string10[] PROGMEM = "Commands: (h)elp, (q)uit, (l)ink,\n  (v)ersion, (s)tatus, (a)rp, (n)tp,\n  (d)hcp, (u)dp, (p)ing, (t)ime\n  (r)eset, (i)nt\n";
const char string11[] PROGMEM = "Good Bye...\n\n";
const char string12[] PROGMEM = "ARP Table:\n";
const char string13[] PROGMEM = " --> ";
const char string14[] PROGMEM = " Tasks";
const char string15[] PROGMEM = "Pin is ";
const char string16[] PROGMEM = " Scheduler";
const char string17[] PROGMEM = "Trying to connect...\n";
const char string18[] PROGMEM = " ms";
const char string19[] PROGMEM = "RoundTripTime";
const char string20[] PROGMEM = "Power-On Reset";
const char string21[] PROGMEM = "External Reset";
const char string22[] PROGMEM = "Brown-Out Reset";
const char string23[] PROGMEM = "Watchdog Reset";
const char string24[] PROGMEM = "JTAG Reset";
const char string25[] PROGMEM = ", ";
const char string26[] PROGMEM = "Not enough memory!\n";
const char string27[] PROGMEM = "Packet sent";
const char string28[] PROGMEM = " - ";
const char string29[] PROGMEM = "Command unknown!\n";
const char string30[] PROGMEM = "(1)Internal or (2)External?\n";
const char string31[] PROGMEM = "Timed out :(\n";
const char string32[] PROGMEM = "Hasn't timed out yet!\n";
const char string33[] PROGMEM = "How many times? (0 - 9)\n";
const char string34[] PROGMEM = "Invalid!\n";
const char string35[] PROGMEM = "Connected!\n";
const char string36[] PROGMEM = " IPv4 Packets in Queue\n";
const char string37[] PROGMEM = "TCP";
const char string38[] PROGMEM = "UDP";
const char string39[] PROGMEM = " Handlers registered\n";
const char string40[] PROGMEM = "MAC reinitialized!\n";
const char string41[] PROGMEM = "High";
const char string42[] PROGMEM = "Low";

// Last index + 1
#define STRINGNUM 43

PGM_P stringTable[STRINGNUM] PROGMEM = {
    string0, string1, string2, string3, string4,
    string5, string6, string7, string8, string9,
    string10, string11, string12, string13, string14,
    string15, string16, string17, string18, string19,
    string20, string21, string22, string23, string24,
    string25, string26, string27, string28, string29,
    string30, string31, string32, string33, string34,
    string35, string36, string37, string38, string39,
    string40, string41, string42
};

const char stringNotFoundError[] PROGMEM = "String not found!\n";

char *getString(uint8_t id) {
    if (id < STRINGNUM) {
        strcpy_P(buff, (PGM_P)pgm_read_word(&(stringTable[id])));
    } else {
        strcpy_P(buff, stringNotFoundError);
    }
    return buff;
}

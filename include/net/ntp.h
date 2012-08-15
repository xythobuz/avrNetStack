/*
 * ntp.h
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
/*
 * A SNTP Implementation that (probably) updates time.h's currentTime
 * sometime after ntpIssueRequest was successfully called (returned 0).
 * Implemented as described in RFC1361 (http://tools.ietf.org/html/rfc1361)
 */
#ifndef _ntp_h
#define _ntp_h

#include <net/ipv4.h>
#include <net/controller.h>

#ifndef DISABLE_DNS
extern char ntpServerDomain[];
#endif

extern IPv4Address ntpServer;

uint8_t ntpHandler(UdpPacket *up);
uint8_t ntpIssueRequest(void);

#endif

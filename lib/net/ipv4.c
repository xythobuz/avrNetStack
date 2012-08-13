/*
 * ipv4.c
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

#include <time.h>
#include <net/mac.h>
#include <net/arp.h>
#include <net/controller.h>
#include <net/ipv4.h>
#include <net/icmp.h>

IPv4Address ownIpAddress;
IPv4Address subnetmask;
IPv4Address defaultGateway;

// ----------------------
// |    Internal API    |
// ----------------------

uint8_t isEqualMem(uint8_t *d1, uint8_t *d2, uint8_t l); // Used in arp.c
IPv4Packet **storedFragments = NULL; // Array of IPv4Packet Pointers
uint16_t fragmentsStored = 0;

IPv4Packet *macPacketToIpPacket(MacPacket *p) {
	uint8_t i, l;
	uint16_t j, realLength;
	IPv4Packet *ip = (IPv4Packet *)malloc(sizeof(IPv4Packet));
	if (ip == NULL) {
		return NULL;
	}
	ip->version = (p->data[0] & 0xF0) >> 4;
	ip->internetHeaderLength = p->data[0] & 0x0F;
	ip->typeOfService = p->data[1];
	ip->totalLength = p->data[3];
	ip->totalLength |= (p->data[2] << 8);
	ip->identification = p->data[5];
	ip->identification |= (p->data[4] << 8);
	ip->flags = (p->data[6] & 0xE0) >> 5;
	ip->fragmentOffset = p->data[7];
	ip->fragmentOffset |= (p->data[6] & 0x1F) << 8;
	ip->timeToLive = p->data[8];
	ip->protocol = p->data[9];
	ip->headerChecksum = p->data[11];
	ip->headerChecksum |= (p->data[10] << 8);
	for (i = 0; i < 4; i++) {
		ip->sourceIp[i] = p->data[12 + i];
		ip->destinationIp[i] = p->data[16 + i];
	}
	l = ip->internetHeaderLength - 5;
	if (l > 5) {
		ip->options = (uint8_t *)malloc(l * 4 * sizeof(uint8_t));
		if (ip->options == NULL) {
			free(ip);
			return NULL;
		}
		for (i = 0; i < l; i++) {
			ip->options[(i * 4) + 0] = p->data[20 + (i * 4)];
			ip->options[(i * 4) + 1] = p->data[21 + (i * 4)];
			ip->options[(i * 4) + 2] = p->data[22 + (i * 4)];
			ip->options[(i * 4) + 3] = p->data[23 + (i * 4)];
		}
	} else {
		ip->options = NULL;
	}
	realLength = ip->totalLength - (ip->internetHeaderLength * 4);
	ip->data = (uint8_t *)malloc(realLength * sizeof(uint8_t));
	if (ip->data == NULL) {
		if (ip->options != NULL) {
			free(ip->options);
		}
		free(ip);
		return NULL;
	}
	for (j = 0; j < realLength; j++) {
		ip->data[j] = p->data[(ip->internetHeaderLength * 4) + j];
	}
	ip->dLength = realLength;
	return ip;
}

#if !defined(DISABLE_IPV4_CHECKSUM) && !defined(DISABLE_ICMP_CHECKSUM)
uint16_t checksum(uint8_t *rawData, uint8_t l) {
	uint32_t a = 0;
	uint8_t i;

	for (i = 0; i < l; i += 2) {
		a += ((rawData[i] << 8) | rawData[i + 1]); // 16bit sum
	}
	a = (a & 0x0000FFFF) + ((a & 0xFFFF0000) >> 16); // 1's complement 16bit sum
	return (uint16_t)~a; // 1's complement of 1's complement 16bit sum
}
#endif

void freeIPv4Packet(IPv4Packet *ip) {
	if (ip != NULL) {
		if (ip->options != NULL) {
			free(ip->options);
		}
		if (ip->data != NULL) {
			free(ip->data);
		}
		free(ip);
	}
}

#ifndef DISABLE_IPV4_FRAGMENT
int16_t findFragment(uint16_t identification, IPv4Address destination) {
	uint16_t i;
	for (i = 0; i < fragmentsStored; i++) {
		if (storedFragments[i] != NULL) {
			if (isEqualMem(destination, storedFragments[i]->destinationIp, 4)) {
				if (identification == storedFragments[i]->identification) {
					return (int16_t)i;
				}
			}
		}
	}
	return -1;
}

uint8_t appendFragment(uint16_t in, IPv4Packet *ip) {
	// Append / Insert fragment into existing buffer
	uint8_t *tmp;
	uint16_t i;
	if (storedFragments[in]->dLength <= (ip->fragmentOffset * 8)) {
		// Buffer is not large enough
		tmp = (uint8_t *)realloc(storedFragments[in]->data, (ip->fragmentOffset * 8) + ip->dLength);
		if (tmp == NULL) {
			freeIPv4Packet(ip);
			return 1;
		}
		storedFragments[in]->data = tmp;
		storedFragments[in]->dLength = (ip->fragmentOffset * 8) + ip->dLength;
	}
	// Insert actual data
	for (i = 0; i < ip->dLength; i++) {
		storedFragments[in]->data[(ip->fragmentOffset * 8) + i] = ip->data[i];
	}
	storedFragments[in]->totalLength -= (ip->internetHeaderLength * 4);
	storedFragments[in]->totalLength += ip->totalLength;
	freeIPv4Packet(ip);
	return 0; // Packet appended
}
#endif

uint8_t ipv4ProcessPacketInternal(IPv4Packet *ip, uint16_t cs) {
#ifndef DISABLE_IPV4_FRAGMENT
	int16_t in;
	uint8_t i, r;
	IPv4Packet **tmp;
#endif

	// Process IPv4 Packet
	if (isEqualMem(ip->destinationIp, ownIpAddress, 4)) {
		// Packet is for us
		if ((cs == 0x0000) && (ip->version == 4)) {
			// Checksum and version fields are valid
			if (!(ip->flags & 0x04)) {
				// Last fragment
				if (ip->fragmentOffset == 0x00) {
					// Packet isn't fragmented
					if (ip->protocol == ICMP) {
						// Internet Control Message Protocol Packet
						return icmpProcessPacket(ip);
					} else if (ip->protocol == IGMP) {
						// Internet Group Management Protocol Packet

					} else if (ip->protocol == TCP) {
						// Transmission Control Protocol Packet

					} else if (ip->protocol == UDP) {
						// User Datagram Protocol Packet

					}
#ifndef DISABLE_IPV4_FRAGMENT
				} else {
					// Packet is last fragment. Are there already some present?
					in = findFragment(ip->identification, ip->destinationIp);
					if (in == -1) {
						// Well, we don't have the beginning of the message...
						freeIPv4Packet(ip);
						return 2;
					} else {
						i = appendFragment(in, ip);
						ip = NULL; // Is already freed in appendFragment
						if (i != 0) {
							return i;
						} else {
							// Finished packet in storedFragmens[in], handle it
							storedFragments[in]->flags = 0x00; // Not a fragment anymore
							storedFragments[in]->fragmentOffset = 0x00;
							r = ipv4ProcessPacketInternal(storedFragments[in], 0x0000);
							storedFragments[in] = NULL;
							if (in < (fragmentsStored - 1)) {
								// Wasn't the last packet, so move the following ones
								for (i = (in + 1); i < fragmentsStored; i++) {
									storedFragments[i - 1] = storedFragments[i];
								}
							}
							fragmentsStored--;
							tmp = (IPv4Packet **)realloc(storedFragments, fragmentsStored * sizeof(IPv4Packet **));
							if (tmp == NULL) {
								// Whoopsie...
								// But we don't really care, it's one rotting IPv4Packet Pointer
								return 1;
							}
							return r;
						}
					}
				}
			} else {
				// More fragments follow. Store this one,
				// if it is the first or we got some fragments already
				in = findFragment(ip->identification, ip->destinationIp);
				if ((in == -1) && (ip->fragmentOffset != 0x00)) {
					// We haven't got the first fragment of this packet...
					freeIPv4Packet(ip);
					return 2;
				} else if (ip->fragmentOffset == 0x00) {
					// First fragment, store it...
					if (in == -1) {
						// Allocate new memory
						fragmentsStored++;
						tmp = (IPv4Packet **)realloc(storedFragments, fragmentsStored * sizeof(IPv4Packet *));
						if (tmp == NULL) {
							// Not enough memory
							fragmentsStored--;
							freeIPv4Packet(ip);
							return 1;
						}
						storedFragments = tmp;
						in = fragmentsStored;
						storedFragments[in] = (IPv4Packet *)malloc(sizeof(IPv4Packet));
						if (storedFragments[in] == NULL) {
							// Delete new space in buffer
							fragmentsStored--;
							tmp = (IPv4Packet **)realloc(storedFragments, fragmentsStored * sizeof(IPv4Packet *));
							if (tmp == NULL) {
								// Now we are really screwed up
								// and there's nothing we could do
								freeIPv4Packet(ip);
								return 1;
							}
							storedFragments = tmp;
							freeIPv4Packet(ip);
							return 1;
						}
					}
					// Copy packet into buffer
					storedFragments[in]->version = ip->version;
					storedFragments[in]->internetHeaderLength = 5; // Discard options
					storedFragments[in]->typeOfService = ip->typeOfService;
					storedFragments[in]->totalLength = ip->totalLength - (ip->internetHeaderLength * 4) + 20;
					storedFragments[in]->identification = ip->identification;
					storedFragments[in]->flags = ip->flags;
					storedFragments[in]->fragmentOffset = 0x00; // First fragment...
					storedFragments[in]->protocol = ip->protocol;
					// We dont care for checksum or TTL...
					for (i = 0; i < 4; i++) {
						storedFragments[in]->sourceIp[i] = ip->sourceIp[i];
						// We don't care for destinationIp, as it is for us...
					}
					storedFragments[in]->options = NULL;
					storedFragments[in]->data = ip->data;
					ip->data = NULL; // Use same data memory
					storedFragments[in]->dLength = ip->dLength;
					freeIPv4Packet(ip);
					return 0; // Fragment stored
				} else if ((in != -1) && (ip->fragmentOffset != 0x00)) {
					return appendFragment(in, ip);
#endif
				}
			}
		} else {
			// Invalid Checksum or version
			freeIPv4Packet(ip);
			return 2;
		}
	} else {
		// Packet is not for us.
		// We are no router!
		freeIPv4Packet(ip);
		return 0;
	}
	freeIPv4Packet(ip);
	return 0;
}

// ----------------------
// |    External API    |
// ----------------------

void ipv4Init(IPv4Address ip, IPv4Address subnet, IPv4Address gateway) {
	uint8_t i;
	for (i = 0; i < 4; i++) {
		ownIpAddress[i] = ip[i];
		subnetmask[i] = subnet[i];
		defaultGateway[i] = gateway[i];
	}
}

uint8_t ipv4ProcessPacket(MacPacket *p) {
	uint16_t cs = 0x0000;
	IPv4Packet *ip = macPacketToIpPacket(p);
#ifndef DISABLE_IPV4_CHECKSUM
	cs = checksum(p->data, 20); // Calculate checksum before freeing raw data
#endif
	free(p->data);
	free(p);
	if (ip == NULL) {
		return 1; // Not enough memory. Can't process packet!
	}
	return ipv4ProcessPacketInternal(ip, cs);
}

// Returns 0 if packet was sent. 1 if destination was unknown.
// Try again later, after ARP response could have arrived...
uint8_t ipv4SendPacket(IPv4Packet *p) {
	return 1;
}

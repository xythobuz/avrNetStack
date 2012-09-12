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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#define DEFPORT 4242
#define BUFFSIZE 128

char buffer[BUFFSIZE];
int s;

void intHandler(int dummy);

int main(int argc, char **argv) {
	int port = DEFPORT;
	socklen_t sl;
	struct sockaddr_in si, si2;

	// Open Socket
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		printf("Could not open socket!\n");
		return 2;
	}

	// Fill si Struct
	memset((char *) &si, 0, sizeof(si)); // Clear to zero
	si.sin_family = AF_INET;
	si.sin_port = htons(port);
	si.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(s, (struct sockaddr *)&si, sizeof(si)) == -1) {
		printf("Could not bind!\n");
		return 2;
	}

	signal(SIGINT, intHandler);
	signal(SIGQUIT, intHandler);
	printf("Waiting for UDP Packets on Port %d\nStop with CTRL+C...\n", port);

	while(1) {
		// recvfrom is blocking...
		if (recvfrom(s, buffer, BUFFSIZE, 0, (struct sockaddr *)&si2, &sl) == -1) {
			printf("Could not receive Packet!\n");
			close(s);
			return 2;
		}
		printf("Packet from %s:%d\nData: %s\n\n", inet_ntoa(si2.sin_addr), ntohs(si2.sin_port), buffer);
	}

	close(s);
	return 0;
}

void intHandler(int dummy) {
	printf("\nExiting...\n");
	close(s);
	exit(0);
}

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

#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#define DEFPORT 6600
#define BUFFSIZE 128

char buffer[BUFFSIZE];
int s;

void usage(char *e);
void processArgs(int argc, char **argv, int *port);
void intHandler(int dummy);

int main(int argc, char **argv) {
	int port = DEFPORT;
	socklen_t sl;
	struct sockaddr_in si, si2;

	processArgs(argc, argv, &port);

	// Open Socket
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		printf("Could not open socket!\n");
		return 2;
	}

	memset((char *) &si, 0, sizeof(si)); // Clear to zero
	si.sin_family = AF_INET;
	si.sin_port = htons(port);
	si.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(s, (struct sockaddr *)&si, sizeof(si)) == -1) {
		printf("Could not bind to port!\n");
		close(s);
		return 2;
	}

	if (fcntl(s, F_SETFL, O_NONBLOCK, 1) == -1) {
		printf("Could not set to nonblock mode!\n");
		close(s);
		return 2;
	}

	// Register Interrupt Handlers to exit in a clean way...
	signal(SIGINT, intHandler);
	signal(SIGQUIT, intHandler);

	printf("Waiting for UDP Packets on Port %d\nStop with CTRL+C...\n", port);

	while(1) {
		if (recvfrom(s, buffer, BUFFSIZE, 0, (struct sockaddr *)&si2, &sl) > 0) {
			printf("Got Packet from %s:%d\nData: %s\n\n", inet_ntoa(si2.sin_addr), ntohs(si2.sin_port), buffer);
		}
	}

	close(s);
	return 0;
}

void usage(char *e) {
	printf("Usage:\n");
	printf("%s [-p 42]\n", e);
	exit(1);
}

void processArgs(int argc, char **argv, int *port) {
	if (argc == 1) {
		// Default values
	} else if (argc == 3) {
		if (strcmp(argv[1], "-p") == 0) {
			*port = atoi(argv[2]);
		} else {
			usage(argv[0]);
		}
	} else {
		usage(argv[0]);
	}
}

void intHandler(int dummy) {
	printf("\nExiting...\n");
	close(s);
	exit(0);
}

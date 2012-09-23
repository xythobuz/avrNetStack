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
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#define PORT 6600
#define TARGET "192.168.0.42"

#define BUFFSIZE 128
char buffer[BUFFSIZE];

int s;

void usage(char *e);
void processArgs(int argc, char **argv, int *port);
void intHandler(int dummy);

int main(int argc, char **argv) {
	socklen_t sl = sizeof(struct sockaddr_in);
	struct sockaddr_in si, si2, si3;
	ssize_t sz;

	// Construct sockaddr_in for our message receiver
	memset((char *) &si, 0, sizeof(si)); // Clear to zero
	si3.sin_family = AF_INET;
	si3.sin_port = htons(PORT);
	if (inet_pton(AF_INET, TARGET, &si3.sin_addr.s_addr) != 1) {
		printf("Target IP not valid!\n");
		return 2;
	}

	// Open Socket
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		printf("Could not open socket!\n");
		return 2;
	}

	// Sockaddr we want to receive on
	memset((char *) &si, 0, sizeof(si)); // Clear to zero
	si.sin_family = AF_INET;
	si.sin_port = htons(PORT);
	si.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(s, (struct sockaddr *)&si, sizeof(si)) == -1) {
		printf("Could not bind to port!\n");
		close(s);
		return 2;
	}

	if (fcntl(s, F_SETFL, O_NONBLOCK, 1) == -1) {
		printf("Could not set Socket to nonblock mode!\n");
		close(s);
		return 2;
	}

	if (fcntl(fileno(stdin), F_SETFL, O_NONBLOCK, 1) == -1) {
		printf("Could not set Terminal to nonblock mode!\n");
		close(s);
		return 2;
	}

	// Register Interrupt Handlers to exit in a clean way...
	signal(SIGINT, intHandler);
	signal(SIGQUIT, intHandler);

	printf("Waiting for UDP Packets on Port %d\nStop with CTRL+C...\n", PORT);

	while(1) {
		if ((sz = recvfrom(s, buffer, BUFFSIZE - 1, 0, (struct sockaddr *)&si2, &sl)) > 0) {
			buffer[sz] = '\0';
			printf("%s:%d  %s\n", inet_ntoa(si2.sin_addr), ntohs(si2.sin_port), buffer);
		}
		if (fgets(buffer, BUFFSIZE, stdin) != NULL) {
			buffer[strlen(buffer) - 1] = '\0'; // Remove trailing new line
			printf("Sending...");
			if ((sz = sendto(s, buffer, strlen(buffer), 0, (struct sockaddr *)&si3, sl)) == -1) {
				printf(" Error (%s)!\n", strerror(errno));
			} else {
				printf(" Done (%d)\n", (int)sz);
			}
		}
	}

	close(s);
	return 0;
}

void intHandler(int dummy) {
	printf(" Exiting...");
	close(s);
	fcntl(fileno(stdin), F_SETFL, O_NONBLOCK, 0);
	exit(0);
}

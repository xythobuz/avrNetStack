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
        // Print received UDP packets
        if ((sz = recvfrom(s, buffer, BUFFSIZE - 1, 0, (struct sockaddr *)&si2, &sl)) > 0) {
            buffer[sz] = '\0';
            printf("%s:%d  %s\n", inet_ntoa(si2.sin_addr), ntohs(si2.sin_port), buffer);
        }

        // Send User Input
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

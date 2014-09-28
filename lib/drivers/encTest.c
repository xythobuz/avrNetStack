#include "encTestCode.h"

#define DEBUG 1

#include <std.h>
#include <net/controller.h>
#include <net/mac.h>

#include <util/delay.h>

#define MAXRECVLEN 400

uint8_t ownMacAddress[6];

uint8_t macInitialize(uint8_t *address) {
    debugPrint("Init ENC...");
    enc28j60Init(address);
    debugPrint(" Done!\n");
    return 0;
}

void macReset(void) {
    // perform system reset
    enc28j60WriteOp(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);
    _delay_ms(20);
}

uint8_t macLinkIsUp(void) {
    return enc28j60linkup();
}

uint8_t macSendPacket(Packet *p) {
    enc28j60PacketSend(p->dLength, p->d);
    return 0;
}

uint8_t macPacketsReceived(void) {
    return enc28j60hasRxPkt();
}

Packet *macGetPacket(void) {
    if (macPacketsReceived()) {
        Packet *p = (Packet *)mmalloc(sizeof(Packet));
        if (p == NULL) {
            return NULL;
        }
        p->d = (uint8_t *)mmalloc(MAXRECVLEN);
        if (p->d == NULL) {
            mfree(p, sizeof(Packet));
            return NULL;
        }
        p->dLength = enc28j60PacketReceive(MAXRECVLEN, p->d);
        if (p->dLength == 0) {
            mfree(p->d, MAXRECVLEN);
            mfree(p, sizeof(Packet));
            return NULL;
        }
        return p;
    } else {
        return NULL;
    }
}

uint8_t macHasInterrupt(void) {
    return macPacketsReceived();
}

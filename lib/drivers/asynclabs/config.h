#ifndef _CONFIG_H
#define _CONFIG_H

#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/interrupt.h>

extern char ssid[];
extern unsigned char ssid_len;

extern char security_passphrase[];
extern unsigned char security_passphrase_len;

extern unsigned char security_type;
extern unsigned char wireless_mode;
extern unsigned char wep_keys[];

// We don't use a real interrupt
extern uint8_t zg2100IsrEnabled;
#define ZG2100_ISR_DISABLE() (zg2100IsrEnabled = 0);
#define ZG2100_ISR_ENABLE()  (zg2100IsrEnabled = 1);

#define ZG2100_CS_BIT  (1 << PA1)
#define ZG2100_CS_DDR  DDRA
#define ZG2100_CS_PORT PORTA

#define ZG2100_CSInit() (ZG2100_CS_DDR  |= ZG2100_CS_BIT)
#define ZG2100_CSon()   (ZG2100_CS_PORT |= ZG2100_CS_BIT)
#define ZG2100_CSoff()  (ZG2100_CS_PORT &= ~ZG2100_CS_BIT)

#define LEDConn_BIT    (1 << PA7)
#define LEDConn_DDR    DDRA
#define LEDConn_PORT   PORTA

#define LEDConn_on()   (LEDConn_PORT |= LEDConn_BIT)
#define LEDConn_off()  (LEDConn_PORT &= ~LEDConn_BIT)

#endif

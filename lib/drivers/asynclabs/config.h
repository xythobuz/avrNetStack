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

extern uint8_t zg2100IsrEnabled;

#define ZG2100_ISR_DISABLE() (zg2100IsrEnabled = 0);
#define ZG2100_ISR_ENABLE()  (zg2100IsrEnabled = 1);

#define SPI0_SS_BIT    (1 << PA1)
#define SPI0_SS_DDR    DDRA
#define SPI0_SS_PORT   PORTA

#define SPI0_SCLK_BIT  (1 << PB7)
#define SPI0_SCLK_DDR  DDRB
#define SPI0_SCLK_PORT PORTB

#define SPI0_MOSI_BIT  (1 << PB5)
#define SPI0_MOSI_DDR  DDRB
#define SPI0_MOSI_PORT PORTB

#define SPI0_MISO_BIT  (1 << PB6)
#define SPI0_MISO_DDR  DDRB
#define SPI0_MISO_PORT PORTB


#define SPI0_WaitForReceive()
#define SPI0_RxData() (SPDR)

#define SPI0_TxData(Data) (SPDR = Data)
#define SPI0_WaitForSend() while((SPSR & 0x80) == 0x00)

#define SPI0_SendByte(Data) SPI0_TxData(Data);SPI0_WaitForSend()
#define SPI0_RecvBute() SPI0_RxData()

#define SPI0_Init() DDRB |= SPI0_SS_BIT|SPI0_SCLK_BIT|SPI0_MOSI_BIT|LEDConn_BIT;\
                    DDRB  &= ~(SPI0_MISO_BIT);\
                    PORTB = SPI0_SS_BIT;\
                    SPCR  = 0x50;\
                    SPSR  = 0x01

#define ZG2100_SpiInit     SPI0_Init
#define ZG2100_SpiSendData SPI0_SendByte
#define ZG2100_SpiRecvData SPI0_RxData

#define ZG2100_CS_BIT   SPI0_SS_BIT
#define ZG2100_CS_DDR   SPI0_SS_DDR
#define ZG2100_CS_PORT  SPI0_SS_PORT

#define ZG2100_CSInit() (ZG2100_CS_DDR  |= ZG2100_CS_BIT)
#define ZG2100_CSon()   (ZG2100_CS_PORT |= ZG2100_CS_BIT)
#define ZG2100_CSoff()  (ZG2100_CS_PORT &= ~ZG2100_CS_BIT)

#define LEDConn_BIT    (1 << PA7)
#define LEDConn_DDR    DDRA
#define LEDConn_PORT   PORTA

#define LEDConn_on()   (LEDConn_PORT |= LEDConn_BIT)
#define LEDConn_off()  (LEDConn_PORT &= ~LEDConn_BIT)

#endif

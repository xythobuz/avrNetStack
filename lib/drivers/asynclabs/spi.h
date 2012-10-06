
/******************************************************************************

  Filename:		spi.h
  Description:	SPI bus configuration for the WiShield 1.0

 ******************************************************************************

  TCP/IP stack and driver for the WiShield 1.0 wireless devices

  Copyright(c) 2009 Async Labs Inc. All rights reserved.

  This program is free software; you can redistribute it and/or modify it
  under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc., 59
  Temple Place - Suite 330, Boston, MA  02111-1307, USA.

  Contact Information:
  <asynclabs@asynclabs.com>

   Author               Date        Comment
  ---------------------------------------------------------------
   AsyncLabs			05/01/2009	Initial version
   AsyncLabs			05/29/2009	Adding support for new library

 *****************************************************************************/

#ifndef SPI2_H_
#define SPI2_H_

#include <spi.h>

// Not using interrupts anyway
#define ZG2100_ISR_DISABLE()
#define ZG2100_ISR_ENABLE()

// Partially uses own spi.h
#define ZG2100_SpiInit() spiInit()
#define ZG2100_SpiSendData(d) (SPDR = d);while(!(SPSR&(1<<SPIF)))
#define ZG2100_SpiRecvData() (SPDR)

#define ZG2100_CS_BIT PA1
#define ZG2100_CS_DDR DDRA
#define ZG2100_CS_PORT PORTA

#define ZG2100_CSInit() (ZG2100_CS_DDR |= (1 << ZG2100_CS_BIT))
#define ZG2100_CSon() (ZG2100_CS_PORT |= (1 << ZG2100_CS_BIT))
#define ZG2100_CSoff() (ZG2100_CS_PORT &= ~(1 << ZG2100_CS_BIT))

// We use the seconds status LED to show connection status
#define LEDConn_BIT PA7
#define LEDConn_DDR DDRA
#define LEDConn_PORT PORTA

#define LEDConn_Init() (LEDConn_DDR |= (1 << LEDConn_BIT))
#define LEDConn_on() (LEDConn_PORT |= (1 << LEDConn_BIT))
#define LEDConn_off() (LEDConn_PORT &= ~(1 << LEDConn_BIT))

#endif /* SPI2_H_ */

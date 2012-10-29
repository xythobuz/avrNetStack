# Copyright (c) 2012, Thomas Buck <xythobuz@me.com>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# - Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
#
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# ------------------------------
IC = enc28j60
#IC = mrf24wb0ma
# ------------------------------

MCU = atmega32
F_CPU = 16000000
RM = rm -rf
OPT = s
EXTRAINCDIR = include
CSTANDARD = gnu99

SRC = lib/drivers/$(IC).c
SRC += lib/std.c
SRC += lib/spi.c
SRC += lib/serial.c
SRC += lib/time.c
SRC += lib/scheduler.c
SRC += lib/tasks.c
SRC += lib/net/controller.c
SRC += lib/net/arp.c
SRC += lib/net/ipv4.c
SRC += lib/net/icmp.c
SRC += lib/net/udp.c
SRC += lib/net/dhcp.c
SRC += lib/net/utils.c
SRC += lib/net/dns.c
SRC += lib/net/ntp.c

ifeq ($(IC),mrf24wb0ma)
SRC += lib/drivers/asynclabs/g2100.c
endif

TESTSRC = test/main.c
TESTSRC += test/strings.c
OBJ = $(SRC:.c=.o)
TESTOBJ = $(TESTSRC:.c=.o)
CARGS = -mmcu=$(MCU)
CARGS += -I$(EXTRAINCDIR)
CARGS += -O$(OPT)
CARGS += -funsigned-char
CARGS += -funsigned-bitfields
CARGS += -fpack-struct
CARGS += -fshort-enums
CARGS += -Wall -Wstrict-prototypes
CARGS += -Iinclude
CARGS += -std=$(CSTANDARD)
CARGS += -DF_CPU=$(F_CPU)

test: test.hex

program: test.hex
	yasab /dev/tty.usbserial-A100QOUE test.hex q

all: libavrNetStack.a

lib: libavrNetStack.a sizelibafter

sizelibafter:
	avr-size --mcu=$(MCU) -C libavrNetStack.a

libavrNetStack.a: $(OBJ)
	avr-ar -c -r -s libavrNetStack.a $(OBJ)

%.o: %.c
	avr-gcc -c $< -o $@ $(CARGS)

test.elf: libavrNetStack.a $(TESTOBJ)
	avr-gcc $(CARGS) $(TESTOBJ) --output test.elf -Wl,-L.,-lm,-lavrNetStack
	avr-size --mcu=$(MCU) -C test.elf

test.hex: test.elf
	avr-objcopy -O ihex test.elf test.hex

clean:
	$(RM) lib/*.o
	$(RM) lib/drivers/*.o
	$(RM) lib/drivers/asynclabs/*.o
	$(RM) lib/net/*.o
	$(RM) test/*.o
	$(RM) *.a
	$(RM) test.hex
	$(RM) test.elf

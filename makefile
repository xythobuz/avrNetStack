# ------------------------------
IC = enc28j60
#IC = uartStub
#IC = mrf24wb0ma
# ------------------------------

MCU = atmega168
F_CPU = 20000000
RM = rm -rf
OPT = s
EXTRAINCDIR = include
CSTANDARD = gnu99

SRC = lib/drivers/$(IC).c
SRC += lib/spi.c
SRC += lib/serial.c
SRC += lib/net/controller.c
SRC += lib/net/arp.c

TESTSRC = test/main.c
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
	$(RM) lib/net/*.o
	$(RM) test/*.o
	$(RM) *.a
	$(RM) test.hex
	$(RM) test.elf

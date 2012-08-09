MCU = atmega168
F_CPU = 16000000
RM = rm -rf

# ------------------------------
SRC = lib/drivers/enc28j60.c
# SRC = lib/drivers/uartStub.c
# SRC = lib/drivers/mrf24wb0ma.c
# ------------------------------
SRC += lib/spi.c

TESTSRC = test/main.c
OBJ = $(SRC:.c=.o)
TESTOBJ = $(TESTSRC:.c=.o)

OPT = s
EXTRAINCDIR = include
CSTANDARD = gnu99

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
	$(RM) $(OBJ)
	$(RM) $(TESTOBJ)
	$(RM) libavrNetStack.a
	$(RM) test.hex
	$(RM) test.elf

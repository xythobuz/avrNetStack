SRC = main.c
TARGET = udpTest
CC = gcc
RM = rm -rf

all: $(TARGET)

run: $(TARGET)
	$(TARGET)

$(TARGET): $(SRC)
	 gcc $(LIB) $(SRC) -o $(TARGET)

clean:
	$(RM) $(TARGET)

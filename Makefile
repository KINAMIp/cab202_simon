MCU = atmega328p
F_CPU = 16000000UL
TARGET = simon

CC = avr-gcc
OBJCOPY = avr-objcopy
CFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -Wall -Wextra -std=gnu11
LDFLAGS = -mmcu=$(MCU)

SRCS = src/main.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET).hex

$(TARGET).elf: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET).elf $(LDFLAGS)

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $(TARGET).elf $(TARGET).hex

clean:
	rm -f $(TARGET).elf $(TARGET).hex $(OBJS)

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
TARGET = simon

SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:.c=.o)

INCLUDES = -Iinclude
LDLIBS = -lm

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDLIBS) -o $@

src/%.o: src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean

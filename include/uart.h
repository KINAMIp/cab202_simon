#ifndef UART_H
#define UART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void uart_init(void);
void uart_write_char(char c);
void uart_write_string(const char *text);
void uart_write_line(const char *text);
void uart_write_uint8(uint8_t value);
bool uart_read_char(char *c);

#endif

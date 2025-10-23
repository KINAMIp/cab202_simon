#ifndef UART_H
#define UART_H

#include <stdbool.h>
#include <stddef.h>

bool uart_read_line(char *buffer, size_t size);
void uart_write(const char *text);
void uart_writeln(const char *text);
void uart_writef(const char *fmt, ...);

#endif

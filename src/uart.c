#include "uart.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

bool uart_read_line(char *buffer, size_t size) {
    if (!buffer || size == 0u) {
        return false;
    }
    if (!fgets(buffer, (int)size, stdin)) {
        return false;
    }
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }
    return true;
}

void uart_write(const char *text) {
    if (text) {
        fputs(text, stdout);
    }
}

void uart_writeln(const char *text) {
    if (text) {
        fputs(text, stdout);
    }
    fputc('\n', stdout);
}

void uart_writef(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

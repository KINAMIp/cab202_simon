#include "uart.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#define UART_BAUD_RATE 9600UL
#define UART_RX_BUFFER_SIZE 64u

#if !defined(F_CPU)
#    error "F_CPU must be defined"
#endif

#define UART_BAUD_VALUE ((uint16_t)((F_CPU * 64UL) / (16UL * UART_BAUD_RATE)))

static volatile uint8_t rx_buffer[UART_RX_BUFFER_SIZE];
static volatile uint8_t rx_head = 0u;
static volatile uint8_t rx_tail = 0u;

static inline uint8_t buffer_next(uint8_t index) {
    return (uint8_t)((index + 1u) % UART_RX_BUFFER_SIZE);
}

ISR(USART0_RXC_vect) {
    uint8_t data = USART0.RXDATAL;
    uint8_t next = buffer_next(rx_head);
    if (next != rx_tail) {
        rx_buffer[rx_head] = data;
        rx_head = next;
    }
}

void uart_init(void) {
    PORTA.DIRSET = PIN1_bm;  // TXD as output
    PORTA.DIRCLR = PIN0_bm;  // RXD as input
    PORTA.PIN0CTRL = PORT_PULLUPEN_bm;

    USART0.BAUD = UART_BAUD_VALUE;
    USART0.CTRLA = USART_RXCIE_bm;
    USART0.CTRLB = USART_RXEN_bm | USART_TXEN_bm;
    USART0.CTRLC = USART_CHSIZE_8BIT_gc;
}

void uart_write_char(char c) {
    while (!(USART0.STATUS & USART_DREIF_bm)) {
        // wait for buffer
    }
    USART0.TXDATAL = (uint8_t)c;
}

void uart_write_string(const char *text) {
    if (!text) {
        return;
    }
    while (*text) {
        uart_write_char(*text++);
    }
}

void uart_write_line(const char *text) {
    uart_write_string(text);
    uart_write_char('\r');
    uart_write_char('\n');
}

void uart_write_uint8(uint8_t value) {
    uint8_t pos = 0u;
    if (value == 0u) {
        uart_write_char('0');
        return;
    }
    char temp[4];
    while (value > 0u && pos < sizeof(temp)) {
        temp[pos++] = (char)('0' + (value % 10u));
        value /= 10u;
    }
    while (pos > 0u) {
        uart_write_char(temp[--pos]);
    }
}

bool uart_read_char(char *c) {
    if (rx_head == rx_tail) {
        return false;
    }
    if (c) {
        *c = (char)rx_buffer[rx_tail];
    }
    rx_tail = buffer_next(rx_tail);
    return true;
}

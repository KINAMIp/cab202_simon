/* src/main.c - minimal test for avr build */
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

int main(void) {
    /* set PB5 (LED on Arduino Uno) as output as a trivial test */
    DDRB |= (1<<5);
    while (1) {
        PORTB ^= (1<<5);
        _delay_ms(500);
    }
    return 0;
}

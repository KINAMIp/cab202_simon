#include "hardware.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#define BUZZER_PIN 0
#define LED_PIN 1

void hardware_init(void)
{
    /* Initialize the hardware */
    DDRB |= (1 << BUZZER_PIN);  // Set buzzer pin as output
    DDRC |= (1 << LED_PIN);      // Set LED pin as output
    PORTB &= ~(1 << BUZZER_PIN); // Ensure buzzer is off initially
    PORTC &= ~(1 << LED_PIN);    // Ensure LED is off initially
}

void hardware_task_display(void)
{
    /* This function would normally handle display updates, but is not used here */
}

void hardware_set_buzzer_tone(uint8_t tone_index)
{
    /* Set the tone based on the index (basic simulation) */
    switch (tone_index) {
    case 0:
        PORTB |= (1 << BUZZER_PIN);  // Turn on buzzer
        break;
    case 1:
        PORTB &= ~(1 << BUZZER_PIN); // Turn off buzzer
        break;
    default:
        break;
    }
}

void hardware_stop_buzzer(void)
{
    PORTB &= ~(1 << BUZZER_PIN); // Turn off buzzer
}

void hardware_set_buzzer_octave_shift(int8_t shift)
{
    /* Not implemented in this case */
}

int8_t hardware_get_buzzer_octave_shift(void)
{
    /* Not implemented in this case */
    return 0;
}

void hardware_display_segments(uint8_t left_digit, uint8_t right_digit)
{
    /* This function would normally display digits on a 7-segment display, but is not used here */
}

void hardware_display_pattern(uint8_t pattern)
{
    /* Display pattern on the hardware (for simplicity, we simulate with LEDs) */
    if (pattern) {
        PORTC |= (1 << LED_PIN);  // Turn on LED
    } else {
        PORTC &= ~(1 << LED_PIN); // Turn off LED
    }
}

void hardware_display_idle_animation(uint8_t frame)
{
    /* Simulate an idle animation with LEDs (for simplicity) */
    if (frame % 2 == 0) {
        PORTC |= (1 << LED_PIN); // Turn on LED
    } else {
        PORTC &= ~(1 << LED_PIN); // Turn off LED
    }
}

uint8_t hardware_read_buttons(void)
{
    /* Return the state of the buttons (this would normally read from the GPIO pins) */
    return PINB & 0x0F; // Assuming buttons are connected to lower 4 bits of PORTB
}

uint16_t hardware_read_pot(void)
{
    /* Read the potentiometer value from an ADC channel */
    uint16_t value = ADC;  // Assuming ADC is already configured
    return value;
}

bool hardware_uart_read(char *value)
{
    if (UCSR0A & (1 << RXC0)) {
        *value = UDR0;  // Read the received data
        return true;
    }
    return false;
}

void hardware_uart_write_char(char value)
{
    while (!(UCSR0A & (1 << UDRE0))) {
        // Wait for the transmit buffer to be empty
    }
    UDR0 = value;  // Write the character to the UART data register
}

void hardware_uart_write_string(const char *text)
{
    while (*text) {
        hardware_uart_write_char(*text++);
    }
}

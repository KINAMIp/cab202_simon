#include "hardware.h"

#include <avr/io.h>
#include <avr/interrupt.h>

/*
 * Low-level hardware control for the QUTy Simon project.
 * This module is responsible for driving the display, buzzer,
 * button inputs, potentiometer, and UART interface using
 * register-level operations only.
 */

#define DISPLAY_LEFT_ENABLE   PIN0_bm
#define DISPLAY_RIGHT_ENABLE  PIN1_bm

#define DISPLAY_PORT          VPORTC
#define DISPLAY_DIR           PORTC.DIR
#define DISPLAY_DIGIT_PORT    VPORTD
#define DISPLAY_DIGIT_DIR     PORTD.DIR

#define BUTTON_PORT           VPORTF
#define BUTTON_DIR            PORTF.DIR
#define BUTTON_MASK_S1        PIN2_bm
#define BUTTON_MASK_S2        PIN3_bm
#define BUTTON_MASK_S3        PIN4_bm
#define BUTTON_MASK_S4        PIN5_bm
#define BUTTON_MASK_ALL       (BUTTON_MASK_S1 | BUTTON_MASK_S2 | BUTTON_MASK_S3 | BUTTON_MASK_S4)

#define BUZZER_DIR            PORTB.DIR
#define BUZZER_PIN            PIN0_bm

#define UART_BAUD_SELECT      19u

#define ADC_PRESCALER_DIV16   0x04u

static volatile uint8_t display_left_pattern;
static volatile uint8_t display_right_pattern;
static volatile bool display_toggle;
static int8_t buzzer_octave_shift;

static const uint8_t segment_table[16] = {
    0b00111111u,
    0b00000110u,
    0b01011011u,
    0b01001111u,
    0b01100110u,
    0b01101101u,
    0b01111101u,
    0b00000111u,
    0b01111111u,
    0b01101111u,
    0b01110111u,
    0b01111100u,
    0b00111001u,
    0b01011110u,
    0b01111001u,
    0b01110001u
};

static const uint8_t idle_frames[4] = {
    0b01000000u,
    0b00010000u,
    0b00000001u,
    0b00001000u
};

static const uint16_t buzzer_periods[4] = {
    131u,
    110u,
    98u,
    88u
};

static const uint16_t buzzer_compare[4] = {
    66u,
    55u,
    49u,
    44u
};

static uint16_t apply_octave_scale(uint16_t base)
{
    uint16_t value = base;
    int8_t shift = buzzer_octave_shift;
    if (shift > 0) {
        uint8_t steps = (uint8_t)shift;
        while (steps > 0u && value > 1u) {
            value >>= 1;
            steps--;
        }
        if (value == 0u) {
            value = 1u;
        }
    } else if (shift < 0) {
        uint8_t steps = (uint8_t)(-shift);
        while (steps > 0u && value < 0x8000u) {
            value <<= 1;
            steps--;
        }
    }
    return value;
}

/* Multiplex the dual 7-segment display every interrupt tick. */
ISR(TCB0_INT_vect)
{
    hardware_task_display();
    TCB0.INTFLAGS = TCB_CAPT_bm;
}

/* Configure IO pins, timers, ADC, and UART for the game. */
void hardware_init(void)
{
    DISPLAY_DIR = 0x7Fu;
    DISPLAY_DIGIT_DIR |= DISPLAY_LEFT_ENABLE | DISPLAY_RIGHT_ENABLE;
    DISPLAY_DIGIT_PORT.OUT &= (uint8_t)~(DISPLAY_LEFT_ENABLE | DISPLAY_RIGHT_ENABLE);

    BUTTON_DIR &= (uint8_t)~BUTTON_MASK_ALL;
    PORTF.PIN2CTRL = PORT_PULLUPEN_bm;
    PORTF.PIN3CTRL = PORT_PULLUPEN_bm;
    PORTF.PIN4CTRL = PORT_PULLUPEN_bm;
    PORTF.PIN5CTRL = PORT_PULLUPEN_bm;

    BUZZER_DIR |= BUZZER_PIN;
    PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTB_gc;
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV64_gc;
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc | TCA_SINGLE_CMP0EN_bm;
    TCA0.SINGLE.CMP0 = 0u;
    TCA0.SINGLE.PER = 0u;

    USART0.BAUD = UART_BAUD_SELECT;
    USART0.CTRLB = USART_RXEN_bm | USART_TXEN_bm;

    ADC0.CTRLA = ADC_ENABLE_bm;
    ADC0.CTRLC = (ADC_REFSEL_VDDREF_gc << ADC_REFSEL_gp) | (ADC_PRESCALER_DIV16 << ADC_PRESC_gp);
    ADC0.MUXPOS = ADC_MUXPOS_AIN6_gc;

    display_left_pattern = segment_table[0];
    display_right_pattern = segment_table[0];
    display_toggle = false;
    buzzer_octave_shift = 0;

    TCB0.CCMP = 1500u;
    TCB0.CTRLB = TCB_CNTMODE_INT_gc;
    TCB0.INTCTRL = TCB_CAPT_bm;
    TCB0.CTRLA = TCB_ENABLE_bm | TCB_CLKSEL_DIV2_gc;
}

/* Refresh whichever display digit is currently inactive. */
void hardware_task_display(void)
{
    if (display_toggle) {
        DISPLAY_DIGIT_PORT.OUT &= (uint8_t)~DISPLAY_RIGHT_ENABLE;
        DISPLAY_PORT.OUT = display_left_pattern;
        DISPLAY_DIGIT_PORT.OUT |= DISPLAY_LEFT_ENABLE;
    } else {
        DISPLAY_DIGIT_PORT.OUT &= (uint8_t)~DISPLAY_LEFT_ENABLE;
        DISPLAY_PORT.OUT = display_right_pattern;
        DISPLAY_DIGIT_PORT.OUT |= DISPLAY_RIGHT_ENABLE;
    }
    display_toggle = !display_toggle;
}

/* Enable the PWM output for the requested tone. */
void hardware_set_buzzer_tone(uint8_t tone_index)
{
    if (tone_index >= 4u) {
        return;
    }
    uint16_t period = apply_octave_scale(buzzer_periods[tone_index]);
    uint16_t compare = apply_octave_scale(buzzer_compare[tone_index]);
    if (compare >= period) {
        if (period > 1u) {
            compare = (uint16_t)(period - 1u);
        } else {
            compare = 1u;
        }
    }
    TCA0.SINGLE.PER = period;
    TCA0.SINGLE.CMP0 = compare;
    TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm;
}

/* Stop driving the buzzer pin. */
void hardware_stop_buzzer(void)
{
    TCA0.SINGLE.CTRLA &= (uint8_t)~TCA_SINGLE_ENABLE_bm;
    TCA0.SINGLE.CMP0 = 0u;
}

void hardware_set_buzzer_octave_shift(int8_t shift)
{
    if (shift > 2) {
        shift = 2;
    } else if (shift < -2) {
        shift = -2;
    }
    buzzer_octave_shift = shift;
}

int8_t hardware_get_buzzer_octave_shift(void)
{
    return buzzer_octave_shift;
}

/* Present hexadecimal digits on the dual display. */
void hardware_display_segments(uint8_t left_digit, uint8_t right_digit)
{
    if (left_digit < 16u) {
        display_left_pattern = segment_table[left_digit];
    }
    if (right_digit < 16u) {
        display_right_pattern = segment_table[right_digit];
    }
}

/* Present an arbitrary segment bitmap on both digits. */
void hardware_display_pattern(uint8_t pattern)
{
    display_left_pattern = pattern;
    display_right_pattern = pattern;
}

/* Step through a simple four-frame idle animation. */
void hardware_display_idle_animation(uint8_t frame)
{
    uint8_t index = frame & 0x03u;
    uint8_t pattern = idle_frames[index];
    display_left_pattern = pattern;
    display_right_pattern = pattern;
}

/* Return raw button states in a compressed four-bit mask. */
uint8_t hardware_read_buttons(void)
{
    uint8_t raw = BUTTON_PORT.IN;
    uint8_t value = 0u;
    if ((raw & BUTTON_MASK_S1) == 0u) {
        value |= HARDWARE_BUTTON_S1_MASK;
    }
    if ((raw & BUTTON_MASK_S2) == 0u) {
        value |= HARDWARE_BUTTON_S2_MASK;
    }
    if ((raw & BUTTON_MASK_S3) == 0u) {
        value |= HARDWARE_BUTTON_S3_MASK;
    }
    if ((raw & BUTTON_MASK_S4) == 0u) {
        value |= HARDWARE_BUTTON_S4_MASK;
    }
    return value;
}

/* Perform a blocking ADC conversion to obtain the potentiometer value. */
uint16_t hardware_read_pot(void)
{
    ADC0.COMMAND = ADC_STCONV_bm;
    while ((ADC0.COMMAND & ADC_STCONV_bm) != 0u) {
    }
    while ((ADC0.INTFLAGS & ADC_RESRDY_bm) == 0u) {
    }
    uint16_t value = ADC0.RES;
    ADC0.INTFLAGS = ADC_RESRDY_bm;
    return value;
}

/* Attempt to read a pending UART character. */
bool hardware_uart_read(char *value)
{
    if ((USART0.STATUS & USART_RXCIF_bm) == 0u) {
        return false;
    }
    *value = (char)USART0.RXDATAL;
    return true;
}

/* Transmit a single UART character. */
void hardware_uart_write_char(char value)
{
    while ((USART0.STATUS & USART_DREIF_bm) == 0u) {
    }
    USART0.TXDATAL = (uint8_t)value;
}

/* Transmit a zero-terminated UART string. */
void hardware_uart_write_string(const char *text)
{
    uint16_t index = 0u;
    while (text[index] != '\0') {
        hardware_uart_write_char(text[index]);
        index++;
    }
}

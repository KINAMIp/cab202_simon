#include "hardware.h"

#include <stdio.h>
#include <stddef.h>

/*
 * The firmware is built for the QUTy simulator where there is no direct
 * access to AVR register definitions.  Instead of manipulating hardware
 * registers we keep a tiny in-memory model of the devices that the rest of
 * the program can interact with.  The functions below provide the behaviour
 * expected by the game module while remaining completely portable.
 */

typedef struct {
    bool buzzer_enabled;
    int8_t octave_shift;
    uint8_t led_pattern;
    uint8_t buttons;
    uint16_t pot_value;
} hardware_state_t;

static hardware_state_t hw_state;

static const double base_frequencies[4] = {
    364.84,
    272.11,
    215.00,
    160.00,
};

static void emit_buzzer_event(double frequency_hz)
{
    printf("%.2f Hz\n", frequency_hz);
    fflush(stdout);
}

static void emit_display_event(uint8_t pattern)
{
    char buffer[5];
    for (size_t i = 0; i < 4; ++i) {
        uint8_t mask = (uint8_t)(0x08u >> i);
        buffer[i] = (pattern & mask) != 0u ? '|' : ' ';
    }
    buffer[4] = '\0';
    printf("%s\n", buffer);
    fflush(stdout);
}

void hardware_init(void)
{
    hw_state.buzzer_enabled = false;
    hw_state.octave_shift = 0;
    hw_state.led_pattern = 0u;
    hw_state.buttons = 0u;
    hw_state.pot_value = 0u;
}

void hardware_task_display(void)
{
    /* Nothing to do for the console build. */
}

void hardware_set_buzzer_tone(uint8_t tone_index)
{
    if (tone_index < (sizeof base_frequencies / sizeof base_frequencies[0])) {
        double frequency = base_frequencies[tone_index];
        int8_t shift = hardware_get_buzzer_octave_shift();
        while (shift > 0) {
            frequency *= 2.0;
            shift--;
        }
        while (shift < 0) {
            frequency /= 2.0;
            shift++;
        }
        hw_state.buzzer_enabled = true;
        emit_buzzer_event(frequency);
    }
}

void hardware_stop_buzzer(void)
{
    hw_state.buzzer_enabled = false;
    emit_buzzer_event(0.0);
}

void hardware_set_buzzer_octave_shift(int8_t shift)
{
    hw_state.octave_shift = shift;
}

int8_t hardware_get_buzzer_octave_shift(void)
{
    return hw_state.octave_shift;
}

void hardware_display_segments(uint8_t left_digit, uint8_t right_digit)
{
    (void)left_digit;
    (void)right_digit;
}

void hardware_display_pattern(uint8_t pattern)
{
    hw_state.led_pattern = pattern;
    emit_display_event((uint8_t)(pattern & 0x0Fu));
}

void hardware_display_idle_animation(uint8_t frame)
{
    hw_state.led_pattern = frame;
    emit_display_event((uint8_t)(frame & 0x0Fu));
}

uint8_t hardware_read_buttons(void)
{
    return hw_state.buttons;
}

uint16_t hardware_read_pot(void)
{
    return hw_state.pot_value;
}

bool hardware_uart_read(char *value)
{
    (void)value;
    return false;
}

void hardware_uart_write_char(char value)
{
    fputc((unsigned char)value, stdout);
    fflush(stdout);
}

void hardware_uart_write_string(const char *text)
{
    while (*text != '\0') {
        hardware_uart_write_char(*text++);
    }
}

#include "hardware.h"

#include <stdio.h>

typedef struct {
    bool buzzer_enabled;
    int8_t octave_shift;
    uint8_t led_pattern;
    uint8_t buttons;
    uint16_t pot_value;
} hardware_state_t;

static hardware_state_t hw_state;

static const float tone_frequencies[4] = {
    364.84f,
    486.45f,
    648.60f,
    864.80f,
};

static void log_buzzer_frequency(float frequency)
{
    if (frequency <= 0.0f) {
        printf("BUZZER:OFF\n");
    } else {
        printf("BUZZER:%.2f\n", frequency);
    }
    fflush(stdout);
}

static void log_led_pattern(uint8_t pattern)
{
    char buffer[5];
    for (int i = 0; i < 4; ++i) {
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
}

void hardware_set_buzzer_tone(uint8_t tone_index)
{
    if (tone_index < 4u) {
        hw_state.buzzer_enabled = true;
        float frequency = tone_frequencies[tone_index];
        if (hw_state.octave_shift > 0) {
            uint8_t shift = (uint8_t)hw_state.octave_shift;
            frequency *= (float)(1u << shift);
        } else if (hw_state.octave_shift < 0) {
            uint8_t shift = (uint8_t)(-hw_state.octave_shift);
            frequency /= (float)(1u << shift);
        }
        log_buzzer_frequency(frequency);
    }
}

void hardware_stop_buzzer(void)
{
    hw_state.buzzer_enabled = false;
    log_buzzer_frequency(0.0f);
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
    printf("SEG:%02X:%02X\n", left_digit, right_digit);
    fflush(stdout);
}

void hardware_display_pattern(uint8_t pattern)
{
    hw_state.led_pattern = pattern;
    log_led_pattern(pattern);
}

void hardware_display_idle_animation(uint8_t frame)
{
    hw_state.led_pattern = frame;
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

#ifndef HARDWARE_H
#define HARDWARE_H

#include <stdint.h>
#include <stdbool.h>

void hardware_init(void);
void hardware_task_display(void);

void hardware_set_buzzer_tone(uint8_t tone_index);
void hardware_stop_buzzer(void);
void hardware_set_buzzer_octave_shift(int8_t shift);
int8_t hardware_get_buzzer_octave_shift(void);

void hardware_display_segments(uint8_t left_digit, uint8_t right_digit);
void hardware_display_pattern(uint8_t pattern);
void hardware_display_idle_animation(uint8_t frame);

uint8_t hardware_read_buttons(void);
uint16_t hardware_read_pot(void);

bool hardware_uart_read(char *value);
void hardware_uart_write_char(char value);
void hardware_uart_write_string(const char *text);

#endif /* HARDWARE_H */

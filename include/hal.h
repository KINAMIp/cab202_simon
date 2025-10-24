#ifndef HAL_H
#define HAL_H

#include <stdint.h>
#include <stdbool.h>

void hal_init(void);
void hal_delay_ms(uint32_t ms);
uint32_t hal_millis(void);

uint8_t hal_get_delay_index(void);
void hal_set_delay_index(uint8_t index);
uint32_t hal_current_delay_ms(void);

int8_t hal_get_octave_shift(void);
void hal_adjust_octave(int8_t delta);
void hal_set_octave(int8_t octave);
void hal_sample_potentiometer(void);

#endif

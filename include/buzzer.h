#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>

void buzzer_init(void);
void buzzer_play(uint8_t button, uint32_t duration_ms, int8_t octave_shift);
void buzzer_silence(void);

#endif

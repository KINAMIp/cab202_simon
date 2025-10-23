#include "buzzer.h"

#include <math.h>
#include <stdio.h>

#include "tone_table.h"

void buzzer_init(void) {
    /* No hardware initialisation required for the host build. */
}

static double apply_octave(double frequency, int8_t octave_shift) {
    if (octave_shift == 0) {
        return frequency;
    }
    double factor = pow(2.0, octave_shift);
    return frequency * factor;
}

void buzzer_play(uint8_t button, uint32_t duration_ms, int8_t octave_shift) {
    size_t count = 0;
    const double *frequencies = tone_table_base_frequencies(&count);
    if (button == 0 || button > count) {
        printf("[Buzzer] Invalid button %u\n", button);
        return;
    }
    double frequency = apply_octave(frequencies[button - 1], octave_shift);
    printf("[Buzzer] Button %u -> %.2f Hz for %u ms\n", button, frequency, duration_ms);
}

void buzzer_silence(void) {
    printf("[Buzzer] Silence\n");
}

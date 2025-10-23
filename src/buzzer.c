#include "buzzer.h"

#include <stdio.h>

#include "tone_table.h"

/*
 * No hardware peripherals are initialised in the host build, but the function is
 * kept to mirror the microcontroller project structure.
 */
void buzzer_init(void) {
    /* Intentional no-op for the simulator environment. */
}

/*
 * Apply an octave shift to the supplied base frequency. Multiplication and
 * division by powers of two are implemented with bit shifts to avoid costly
 * arithmetic on the embedded target.
 */
static uint32_t apply_octave(uint32_t frequency, int8_t octave_shift) {
    if (octave_shift > 0) {
        return frequency << octave_shift;
    }
    if (octave_shift < 0) {
        uint8_t shift = (uint8_t)(-octave_shift);
        return frequency >> shift;
    }
    return frequency;
}

/*
 * Emit a textual representation of the requested tone. The host build prints
 * the frequency for debugging, while the embedded target would trigger the
 * physical buzzer hardware.
 */
void buzzer_play(uint8_t button, uint32_t duration_ms, int8_t octave_shift) {
    size_t count = 0u;
    const uint16_t *frequencies = tone_table_base_frequencies(&count);
    if (button == 0u || button > count) {
        printf("[Buzzer] Invalid button %u\n", button);
        return;
    }
    uint32_t frequency = apply_octave(frequencies[button - 1u], octave_shift);
    printf("[Buzzer] Button %u -> %u Hz for %u ms\n", button, frequency, duration_ms);
}

/*
 * Stop any currently playing tone. In the simulator this is a diagnostic
 * message, while on hardware it would disable the PWM output.
 */
void buzzer_silence(void) {
    printf("[Buzzer] Silence\n");
}

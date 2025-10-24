#include "buzzer.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stddef.h>

#include "tone_table.h"

#define BUZZER_PIN PIN0_bm

static volatile bool g_buzzer_active = false;

static uint32_t apply_octave(uint32_t frequency_centi, int8_t octave_shift) {
    if (octave_shift > 0) {
        return frequency_centi << octave_shift;
    }
    if (octave_shift < 0) {
        uint8_t shift = (uint8_t)(-octave_shift);
        return frequency_centi >> shift;
    }
    return frequency_centi;
}

static uint16_t frequency_to_top(uint32_t frequency_centi) {
    if (frequency_centi == 0u) {
        return 0u;
    }
    uint32_t numerator = F_CPU * 100UL;
    uint32_t denominator = 2UL * frequency_centi;
    if (denominator == 0u) {
        return 0u;
    }
    uint32_t value = (numerator + (denominator / 2u)) / denominator;
    if (value == 0u) {
        return 0u;
    }
    if (value > 0u) {
        value -= 1u;
    }
    if (value > 0xFFFFu) {
        value = 0xFFFFu;
    }
    return (uint16_t)value;
}

ISR(TCB0_INT_vect) {
    TCB0.INTFLAGS = TCB_CAPT_bm;
    if (g_buzzer_active) {
        PORTB.OUTTGL = BUZZER_PIN;
    } else {
        PORTB.OUTCLR = BUZZER_PIN;
    }
}

void buzzer_init(void) {
    PORTB.DIRSET = BUZZER_PIN;
    PORTB.OUTCLR = BUZZER_PIN;

    TCB0.CTRLB = TCB_CNTMODE_INT_gc;
    TCB0.CCMP = 0u;
    TCB0.INTCTRL = TCB_CAPT_bm;
    TCB0.CTRLA = TCB_CLKSEL_DIV1_gc;
}

void buzzer_play(uint8_t button, uint32_t duration_ms, int8_t octave_shift) {
    (void)duration_ms;
    size_t count = 0u;
    const uint32_t *frequencies = tone_table_base_frequencies(&count);
    if (button == 0u || button > count) {
        return;
    }
    uint32_t frequency_centi = apply_octave(frequencies[button - 1u], octave_shift);
    uint16_t top = frequency_to_top(frequency_centi);
    TCB0.CCMP = top;
    g_buzzer_active = true;
    TCB0.CTRLA |= TCB_ENABLE_bm;
}

void buzzer_silence(void) {
    g_buzzer_active = false;
    PORTB.OUTCLR = BUZZER_PIN;
    TCB0.CTRLA &= (uint8_t)~TCB_ENABLE_bm;
}

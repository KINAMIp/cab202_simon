#include "hal.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#include "config.h"

#define POT_ADC_CHANNEL ADC_MUXPOS_AIN6_gc

static volatile uint32_t g_millis = 0u;
static uint8_t g_delay_index = SIMON_DEFAULT_DELAY_INDEX;
static int8_t g_octave_shift = 0;

static uint32_t delay_step_ms(void) {
    const simon_config_t *cfg = config_get();
    if (cfg->delay_steps <= 1u) {
        return cfg->min_delay_ms;
    }
    uint32_t span = cfg->max_delay_ms - cfg->min_delay_ms;
    return span / (cfg->delay_steps - 1u);
}

ISR(TCB1_INT_vect) {
    TCB1.INTFLAGS = TCB_CAPT_bm;
    ++g_millis;
}

void hal_init(void) {
    g_delay_index = config_get()->default_delay_index;
    g_octave_shift = 0;
    g_millis = 0u;

    // Configure system tick using TCB1 at 1 kHz
    TCB1.CTRLB = TCB_CNTMODE_INT_gc;
    TCB1.CCMP = (uint16_t)(F_CPU / 1000UL) - 1u;
    TCB1.INTCTRL = TCB_CAPT_bm;
    TCB1.CTRLA = TCB_CLKSEL_DIV1_gc | TCB_ENABLE_bm;

    // Configure ADC for potentiometer readings
    PORTD.DIRCLR = PIN6_bm;
    PORTD.PIN6CTRL = PORT_ISC_INPUT_DISABLE_gc;
    VREF.ADC0REF = VREF_REFSEL_VDD_gc;
    ADC0.CTRLA = ADC_ENABLE_bm;
    ADC0.CTRLC = ADC_PRESC_DIV32_gc | ADC_REFSEL_VDDREF_gc;
    ADC0.MUXPOS = POT_ADC_CHANNEL;
}

void hal_delay_ms(uint32_t ms) {
    uint32_t start = hal_millis();
    while ((hal_millis() - start) < ms) {
        // busy wait using millisecond tick
    }
}

uint32_t hal_millis(void) {
    uint32_t snapshot;
    cli();
    snapshot = g_millis;
    sei();
    return snapshot;
}

uint8_t hal_get_delay_index(void) {
    return g_delay_index;
}

void hal_set_delay_index(uint8_t index) {
    const simon_config_t *cfg = config_get();
    if (index >= cfg->delay_steps) {
        index = (uint8_t)(cfg->delay_steps - 1u);
    }
    g_delay_index = index;
}

uint32_t hal_current_delay_ms(void) {
    const simon_config_t *cfg = config_get();
    return cfg->min_delay_ms + delay_step_ms() * g_delay_index;
}

int8_t hal_get_octave_shift(void) {
    return g_octave_shift;
}

void hal_adjust_octave(int8_t delta) {
    const simon_config_t *cfg = config_get();
    int8_t octave = g_octave_shift + delta;
    if (octave < cfg->min_octave) {
        octave = cfg->min_octave;
    }
    if (octave > cfg->max_octave) {
        octave = cfg->max_octave;
    }
    g_octave_shift = octave;
}

void hal_set_octave(int8_t octave) {
    const simon_config_t *cfg = config_get();
    if (octave < cfg->min_octave) {
        octave = cfg->min_octave;
    }
    if (octave > cfg->max_octave) {
        octave = cfg->max_octave;
    }
    g_octave_shift = octave;
}

void hal_sample_potentiometer(void) {
    ADC0.COMMAND = ADC_STCONV_bm;
    while (!(ADC0.INTFLAGS & ADC_RESRDY_bm)) {
        // wait for conversion
    }
    uint16_t raw = ADC0.RES;
    ADC0.INTFLAGS = ADC_RESRDY_bm;

    uint8_t index = (uint8_t)((uint32_t)raw * config_get()->delay_steps / 1024u);
    if (index >= config_get()->delay_steps) {
        index = (uint8_t)(config_get()->delay_steps - 1u);
    }
    g_delay_index = index;
}

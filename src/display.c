#include "display.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#define SEG_A PIN0_bm
#define SEG_B PIN1_bm
#define SEG_C PIN2_bm
#define SEG_D PIN3_bm
#define SEG_E PIN4_bm
#define SEG_F PIN5_bm
#define SEG_G PIN6_bm
#define SEG_DP PIN7_bm

#define SEG_MASK (SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G | SEG_DP)

#define DIGIT_LEFT_PIN PIN0_bm
#define DIGIT_RIGHT_PIN PIN1_bm

static volatile uint8_t g_digit_pattern[2] = {0u, 0u};
static volatile uint8_t g_active_digit = 0u;

static const uint8_t digit_lut[] = {
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,          // 0
    SEG_B | SEG_C,                                          // 1
    SEG_A | SEG_B | SEG_D | SEG_E | SEG_G,                  // 2
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_G,                  // 3
    SEG_B | SEG_C | SEG_F | SEG_G,                          // 4
    SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,                  // 5
    SEG_A | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,          // 6
    SEG_A | SEG_B | SEG_C,                                  // 7
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,  // 8
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G           // 9
};

static void apply_pattern(uint8_t pattern) {
    PORTC.OUTCLR = SEG_MASK;
    PORTC.OUTSET = pattern & SEG_MASK;
}

ISR(TCA0_OVF_vect) {
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
    PORTD.OUTCLR = DIGIT_LEFT_PIN | DIGIT_RIGHT_PIN;
    apply_pattern(g_digit_pattern[g_active_digit]);
    if (g_active_digit == 0u) {
        PORTD.OUTSET = DIGIT_LEFT_PIN;
        g_active_digit = 1u;
    } else {
        PORTD.OUTSET = DIGIT_RIGHT_PIN;
        g_active_digit = 0u;
    }
}

static void display_blank(void) {
    g_digit_pattern[0] = 0u;
    g_digit_pattern[1] = 0u;
}

void display_init(void) {
    PORTC.DIRSET = SEG_MASK;
    PORTD.DIRSET = DIGIT_LEFT_PIN | DIGIT_RIGHT_PIN;
    display_blank();

    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV64_gc;
    TCA0.SINGLE.PER = (uint16_t)(((F_CPU / 64UL) / 1000UL) - 1u);
    TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
    TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm;
}

void display_show_level(uint8_t level) {
    if (level == 0u) {
        display_blank();
        return;
    }
    if (level > 99u) {
        g_digit_pattern[0] = digit_lut[8];
        g_digit_pattern[1] = digit_lut[8];
        return;
    }
    uint8_t tens = level / 10u;
    uint8_t ones = level % 10u;
    g_digit_pattern[0] = digit_lut[tens];
    g_digit_pattern[1] = digit_lut[ones];
}

void display_show_sequence_step(uint8_t button) {
    display_blank();
    switch (button) {
        case 1u:
            g_digit_pattern[0] = SEG_E;
            break;
        case 2u:
            g_digit_pattern[0] = SEG_F;
            break;
        case 3u:
            g_digit_pattern[1] = SEG_C;
            break;
        case 4u:
            g_digit_pattern[1] = SEG_D;
            break;
        default:
            break;
    }
}

void display_show_success(void) {
    g_digit_pattern[0] = SEG_MASK;
    g_digit_pattern[1] = SEG_MASK;
}

void display_show_failure(void) {
    g_digit_pattern[0] = SEG_C;
    g_digit_pattern[1] = SEG_C;
}

void display_show_ready(void) {
    display_blank();
}

#include "lfsr.h"

#include "config.h"

#define LFSR_FEEDBACK_MASK 0x80200003u

/*
 * Seed the LFSR. A zero seed is replaced with the default to keep the generator
 * running.
 */
void lfsr_seed(lfsr_t *lfsr, uint32_t seed) {
    if (!lfsr) {
        return;
    }
    if (seed == 0u) {
        seed = SIMON_DEFAULT_SEED;
    }
    lfsr->state = seed;
}

/*
 * Advance the LFSR using a taps configuration that produces a long period. The
 * current state is returned so callers can consume the pseudo-random bits.
 */
uint32_t lfsr_next(lfsr_t *lfsr) {
    if (!lfsr || lfsr->state == 0u) {
        return 0u;
    }
    uint32_t lsb = lfsr->state & 1u;
    lfsr->state >>= 1u;
    if (lsb != 0u) {
        lfsr->state ^= LFSR_FEEDBACK_MASK;
    }
    return lfsr->state;
}

/*
 * Convert the pseudo-random LFSR output into a button index in the range 1-4.
 */
uint8_t lfsr_next_button(lfsr_t *lfsr) {
    uint32_t value = lfsr_next(lfsr);
    return (uint8_t)((value & 0x03u) + 1u);
}

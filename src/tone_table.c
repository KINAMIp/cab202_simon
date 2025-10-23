#include "tone_table.h"

/*
 * Frequencies for the Simon buttons represented in Hertz. Integer values avoid
 * the need for floating-point support on the microcontroller target while still
 * providing musically recognisable tones.
 */
static const uint16_t g_base_frequencies[] = {
    392u,  /* G4 */
    440u,  /* A4 */
    494u,  /* B4 */
    523u   /* C5 */
};

/*
 * Return the tone table along with the element count for caller iteration.
 */
const uint16_t *tone_table_base_frequencies(size_t *count) {
    if (count) {
        *count = sizeof(g_base_frequencies) / sizeof(g_base_frequencies[0]);
    }
    return g_base_frequencies;
}

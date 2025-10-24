#include "tone_table.h"

// Frequencies expressed in centi-Hertz to retain two decimal places
static const uint32_t g_base_frequencies[] = {
    36484u,  // Tone 1: 364.84 Hz
    48645u,  // Tone 2: 486.45 Hz
    64860u,  // Tone 3: 648.60 Hz
    86481u   // Tone 4: 864.81 Hz
};

const uint32_t *tone_table_base_frequencies(size_t *count) {
    if (count) {
        *count = sizeof(g_base_frequencies) / sizeof(g_base_frequencies[0]);
    }
    return g_base_frequencies;
}

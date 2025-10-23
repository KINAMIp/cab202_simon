#include "tone_table.h"

static const double g_base_frequencies[] = {
    392.0,  /* G4 */
    440.0,  /* A4 */
    494.0,  /* B4 */
    523.25  /* C5 */
};

const double *tone_table_base_frequencies(size_t *count) {
    if (count) {
        *count = sizeof(g_base_frequencies) / sizeof(g_base_frequencies[0]);
    }
    return g_base_frequencies;
}

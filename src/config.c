#include "config.h"

/*
 * Immutable configuration values shared across the program. Storing them in a
 * single structure keeps the limits consistent between modules and mirrors the
 * way constants would be kept in flash on the embedded platform.
 */
static const simon_config_t g_config = {
    .min_delay_ms = SIMON_MIN_DELAY_MS,
    .max_delay_ms = SIMON_MAX_DELAY_MS,
    .delay_steps = SIMON_DELAY_STEPS,
    .default_delay_index = SIMON_DEFAULT_DELAY_INDEX,
    .default_seed = SIMON_DEFAULT_SEED,
    .min_octave = SIMON_MIN_OCTAVE_SHIFT,
    .max_octave = SIMON_MAX_OCTAVE_SHIFT,
    .max_sequence_length = SIMON_MAX_SEQUENCE,
    .max_level = SIMON_MAX_LEVEL,
    .max_highscores = SIMON_MAX_HIGHSCORES,
    .max_name_length = SIMON_MAX_NAME_LENGTH,
};

/*
 * Provide read-only access to the shared configuration block.
 */
const simon_config_t *config_get(void) {
    return &g_config;
}

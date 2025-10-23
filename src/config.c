#include "config.h"

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

const simon_config_t *config_get(void) {
    return &g_config;
}

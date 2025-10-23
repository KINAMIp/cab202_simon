#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stddef.h>

#define SIMON_MAX_SEQUENCE        64
#define SIMON_MAX_LEVEL           99
#define SIMON_MIN_DELAY_MS        250u
#define SIMON_MAX_DELAY_MS        1250u
#define SIMON_DELAY_STEPS         10u
#define SIMON_DEFAULT_DELAY_INDEX 4u
#define SIMON_DEFAULT_SEED        0x2DF599ACu
#define SIMON_MAX_OCTAVE_SHIFT    2
#define SIMON_MIN_OCTAVE_SHIFT   -1
#define SIMON_MAX_HIGHSCORES      5
#define SIMON_MAX_NAME_LENGTH     12
#define SIMON_DEFAULT_NAME        "PLAYER"

typedef struct {
    uint32_t min_delay_ms;
    uint32_t max_delay_ms;
    uint8_t delay_steps;
    uint8_t default_delay_index;
    uint32_t default_seed;
    int8_t min_octave;
    int8_t max_octave;
    size_t max_sequence_length;
    uint8_t max_level;
    uint8_t max_highscores;
    uint8_t max_name_length;
} simon_config_t;

const simon_config_t *config_get(void);

#endif

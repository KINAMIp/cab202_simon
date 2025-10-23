#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "config.h"
#include "highscore.h"
#include "input.h"
#include "lfsr.h"

typedef enum {
    GAME_STATE_IDLE = 0,
    GAME_STATE_PLAYBACK,
    GAME_STATE_AWAIT_INPUT,
    GAME_STATE_AWAIT_NAME
} game_state_t;

typedef struct {
    const simon_config_t *config;
    lfsr_t rng;
    uint8_t sequence[SIMON_MAX_SEQUENCE];
    uint8_t level;
    uint8_t step_index;
    game_state_t state;
    highscore_table_t highscores;
    uint32_t pending_seed;
    bool seed_pending;
    uint8_t last_score;
    bool awaiting_name;
} game_t;

void game_init(game_t *game);
void game_reset(game_t *game);
void game_print_status(const game_t *game);
void game_handle_event(game_t *game, const input_event_t *event);

#endif

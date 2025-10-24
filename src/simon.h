#ifndef SIMON_H
#define SIMON_H

#include <stdbool.h>
#include <stdint.h>

#include "board.h"

#define SIMON_MAX_SEQUENCE 32
#define SIMON_HIGHSCORE_ENTRIES 5

typedef enum {
    SIMON_STATE_ATTRACT = 0,
    SIMON_STATE_PLAYBACK,
    SIMON_STATE_WAIT_INPUT,
    SIMON_STATE_LEVEL_COMPLETE,
    SIMON_STATE_FAILURE,
    SIMON_STATE_NAME_ENTRY
} simon_state_t;

typedef struct {
    char name[BOARD_MAX_TEXT];
    uint16_t score;
} simon_highscore_entry_t;

typedef struct {
    simon_highscore_entry_t entries[SIMON_HIGHSCORE_ENTRIES];
} simon_highscore_table_t;

typedef struct {
    simon_state_t state;
    uint8_t sequence[SIMON_MAX_SEQUENCE];
    uint8_t level;
    uint8_t playback_index;
    uint8_t input_index;
    uint16_t playback_timer;
    uint16_t playback_delay_ms;
    uint16_t best_level;
    uint32_t rng_state;
    simon_highscore_table_t highscores;
    bool awaiting_seed;
} simon_game_t;

void simon_game_init(simon_game_t *game);
void simon_game_handle_event(simon_game_t *game, const board_event_t *event);

#endif /* SIMON_H */

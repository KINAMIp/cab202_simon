#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <stdbool.h>

#include "board.h"
#include "hardware.h"

#define SIMON_MAX_SEQUENCE 32
#define SIMON_MAX_NAME_LENGTH 32
#define SIMON_HIGHSCORE_ENTRIES 5

typedef struct {
    char name[SIMON_MAX_NAME_LENGTH];
    uint16_t score;
} simon_highscore_entry_t;

typedef struct {
    simon_highscore_entry_t entries[SIMON_HIGHSCORE_ENTRIES];
} simon_highscore_table_t;

typedef enum {
    SIMON_STATE_ATTRACT = 0,
    SIMON_STATE_PLAYBACK,
    SIMON_STATE_WAIT_INPUT,
    SIMON_STATE_LEVEL_COMPLETE,
    SIMON_STATE_FAILURE,
    SIMON_STATE_NAME_ENTRY
} simon_state_t;

typedef struct {
    uint8_t sequence[SIMON_MAX_SEQUENCE];
    uint8_t level;
    uint8_t playback_step;
    uint8_t input_step;
    uint16_t playback_delay_ms;
    uint16_t best_score;
    uint16_t score;
    simon_state_t state;
    uint32_t rng_state;
    bool pot_update_pending;
    bool playback_tone_active;
    bool pending_success;
    bool pending_highscore;
    int8_t octave_shift;
    uint8_t name_length;
    uint8_t seed_length;
    uint16_t name_timeout;
    uint16_t pot_value;
    bool awaiting_seed;
    uint16_t pending_pot_value;
    uint16_t playback_timer;
    uint16_t state_timer;
    uint16_t pending_score;
    uint8_t idle_frame;
    char name_buffer[SIMON_MAX_NAME_LENGTH];
    char seed_buffer[SIMON_MAX_NAME_LENGTH];
    simon_highscore_table_t highscores;
} simon_game_t;

void game_init(simon_game_t *game);
void game_tick_1ms(simon_game_t *game);
void game_handle_button(simon_game_t *game, uint8_t button_mask);
void game_update_playback_delay(simon_game_t *game, uint16_t pot_value);
void game_handle_uart_char(simon_game_t *game, char value);
void game_handle_event(simon_game_t *game, const board_event_t *event);
void game_reset(simon_game_t *game);
void game_start(simon_game_t *game);
void game_end(simon_game_t *game);

#endif /* GAME_H */

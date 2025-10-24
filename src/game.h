#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stdint.h>

#define SIMON_MAX_LEVEL          32u
#define SIMON_HIGHSCORE_COUNT    5u
#define SIMON_NAME_LENGTH        8u
#define SIMON_OCTAVE_MIN         (-2)
#define SIMON_OCTAVE_MAX         2

typedef enum {
    SIMON_STATE_ATTRACT = 0,
    SIMON_STATE_PLAYBACK,
    SIMON_STATE_INPUT,
    SIMON_STATE_LEVEL_COMPLETE,
    SIMON_STATE_FAILURE,
    SIMON_STATE_NAME_ENTRY
} simon_state_t;

typedef struct {
    char name[SIMON_NAME_LENGTH];
    uint16_t score;
} simon_highscore_t;

typedef struct {
    simon_state_t state;
    simon_highscore_t highscores[SIMON_HIGHSCORE_COUNT];
    uint32_t base_seed;
    uint32_t playback_seed;
    uint32_t input_seed;
    uint16_t playback_delay_ms;
    uint16_t countdown_ms;
    uint16_t idle_ticks;
    uint16_t name_timeout;
    uint16_t last_score;
    uint8_t level;
    uint8_t playback_step;
    uint8_t input_step;
    uint8_t name_length;
    char name_buffer[SIMON_NAME_LENGTH];
    bool awaiting_seed;
    bool playback_tone_active;
    bool pending_success;
    uint8_t current_colour;
    int8_t octave_shift;
} simon_game_t;

void game_init(simon_game_t *game);
void game_tick_1ms(simon_game_t *game);
void game_handle_button(simon_game_t *game, uint8_t button_mask);
void game_handle_uart_char(simon_game_t *game, char value);
void game_update_playback_delay(simon_game_t *game, uint16_t pot_value);

#endif /* GAME_H */

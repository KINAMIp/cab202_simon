#include "game.h"

#include <stdio.h>
#include <string.h>

#include "buzzer.h"
#include "display.h"
#include "hal.h"
#include "uart.h"
#include "util.h"

static void apply_pending_seed(game_t *game) {
    if (game->seed_pending) {
        lfsr_seed(&game->rng, game->pending_seed);
        uart_writef("[Game] RNG seeded with 0x%08X\n", game->pending_seed);
        game->seed_pending = false;
    }
}

static void start_playback(game_t *game) {
    apply_pending_seed(game);
    if (game->level >= game->config->max_sequence_length) {
        uart_writeln("[Game] Maximum sequence length reached. Resetting level.");
        game->level = 0;
    }
    uint8_t next_button = lfsr_next_button(&game->rng);
    game->sequence[game->level] = next_button;
    game->level++;
    game->state = GAME_STATE_PLAYBACK;
    game->step_index = 0;
    uart_writef("[Game] Playing level %u\n", game->level);
    display_show_level(game->level);
    uint32_t delay = hal_current_delay_ms();
    uint32_t tone_duration = delay / 2u;
    if (tone_duration < 100u) {
        tone_duration = 100u;
    }
    for (uint8_t i = 0; i < game->level; ++i) {
        uint8_t button = game->sequence[i];
        display_show_sequence_step(button);
        buzzer_play(button, tone_duration, hal_get_octave_shift());
        hal_delay_ms(tone_duration);
        buzzer_silence();
        hal_delay_ms(delay - tone_duration);
    }
    display_show_ready();
    game->state = GAME_STATE_AWAIT_INPUT;
    game->step_index = 0;
    uart_writeln("[Game] Awaiting player input...");
}

static void handle_correct_input(game_t *game) {
    game->step_index++;
    if (game->step_index >= game->level) {
        display_show_success();
        buzzer_silence();
        uart_writef("[Game] Level %u complete!\n", game->level);
        hal_delay_ms(hal_current_delay_ms());
        start_playback(game);
    }
}

static void handle_failure(game_t *game) {
    display_show_failure();
    buzzer_silence();
    uart_writeln("[Game] Incorrect input. Game over!");
    game->last_score = (game->level > 0u) ? (game->level - 1u) : 0u;
    if (game->last_score > 0u) {
        uart_writef("[Game] You reached level %u. Enter your name using 'name <text>' to record the score.\n", game->last_score);
        game->awaiting_name = true;
        game->state = GAME_STATE_AWAIT_NAME;
    } else {
        uart_writeln("[Game] No score recorded. Use 'start' to play again.");
        game_reset(game);
    }
}

void game_init(game_t *game) {
    if (!game) {
        return;
    }
    memset(game, 0, sizeof(*game));
    game->config = config_get();
    lfsr_seed(&game->rng, game->config->default_seed);
    highscore_init(&game->highscores);
    game->state = GAME_STATE_IDLE;
    game->seed_pending = false;
    game->awaiting_name = false;
    game->last_score = 0;
}

void game_reset(game_t *game) {
    if (!game) {
        return;
    }
    game->level = 0u;
    game->step_index = 0u;
    game->state = GAME_STATE_IDLE;
    game->awaiting_name = false;
    game->last_score = 0u;
    apply_pending_seed(game);
    uart_writeln("[Game] Reset complete. Type 'start' to begin.");
    display_show_level(0);
}

void game_print_status(const game_t *game) {
    if (!game) {
        return;
    }
    const char *state_text = "Idle";
    switch (game->state) {
        case GAME_STATE_IDLE:
            state_text = "Idle";
            break;
        case GAME_STATE_PLAYBACK:
            state_text = "Playback";
            break;
        case GAME_STATE_AWAIT_INPUT:
            state_text = "Awaiting Input";
            break;
        case GAME_STATE_AWAIT_NAME:
            state_text = "Awaiting Name";
            break;
    }
    uart_writef("[Status] Level: %u, State: %s, Delay: %ums, Octave: %+d\n",
                game->level, state_text, hal_current_delay_ms(), hal_get_octave_shift());
}

static void handle_press(game_t *game, uint8_t button) {
    if (game->state != GAME_STATE_AWAIT_INPUT) {
        uart_writeln("[Game] Not ready for button input. Use 'start' to begin.");
        return;
    }
    if (button == 0u || button > 4u) {
        uart_writeln("[Game] Button must be between 1 and 4.");
        return;
    }
    uint8_t expected = game->sequence[game->step_index];
    uart_writef("[Game] Button %u pressed (expected %u)\n", button, expected);
    if (button == expected) {
        display_show_sequence_step(button);
        buzzer_play(button, hal_current_delay_ms() / 2u, hal_get_octave_shift());
        buzzer_silence();
        handle_correct_input(game);
    } else {
        handle_failure(game);
    }
}

void game_handle_event(game_t *game, const input_event_t *event) {
    if (!game || !event) {
        return;
    }
    switch (event->type) {
        case INPUT_EVENT_HELP:
            input_print_help();
            break;
        case INPUT_EVENT_START:
            if (game->awaiting_name) {
                uart_writeln("[Game] Submit your name first using 'name <text>'.");
            } else {
                start_playback(game);
            }
            break;
        case INPUT_EVENT_PRESS:
            handle_press(game, event->data.press.button);
            break;
        case INPUT_EVENT_SET_DELAY:
            hal_set_delay_index(event->data.delay.delay_index);
            break;
        case INPUT_EVENT_OCTAVE_UP:
            hal_adjust_octave(+1);
            break;
        case INPUT_EVENT_OCTAVE_DOWN:
            hal_adjust_octave(-1);
            break;
        case INPUT_EVENT_RESET:
            game_reset(game);
            break;
        case INPUT_EVENT_SEED:
            game->pending_seed = event->data.seed.seed;
            game->seed_pending = true;
            uart_writef("[Game] Seed 0x%08X stored. It will apply on next reset/start.\n", game->pending_seed);
            break;
        case INPUT_EVENT_HIGHSCORES:
            highscore_print(&game->highscores);
            break;
        case INPUT_EVENT_SUBMIT_NAME:
            if (!game->awaiting_name) {
                uart_writeln("[Game] No score pending. Play a round first.");
            } else {
                if (highscore_try_insert(&game->highscores, event->data.name.name, game->last_score)) {
                    uart_writeln("[Game] High score recorded!");
                } else {
                    uart_writeln("[Game] Score did not reach the high-score table.");
                }
                game_reset(game);
            }
            break;
        case INPUT_EVENT_EXIT:
        case INPUT_EVENT_INVALID:
        default:
            break;
    }
}

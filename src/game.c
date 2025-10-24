#include "game.h"

#include <stddef.h>
#include <string.h>

#include "buzzer.h"
#include "display.h"
#include "hal.h"
#include "uart.h"
#include "util.h"

static void apply_pending_seed(game_t *game) {
    if (game->seed_pending) {
        lfsr_seed(&game->rng, game->pending_seed);
        game->seed_pending = false;
    }
}

static void show_success_feedback(game_t *game) {
    display_show_success();
    buzzer_silence();
    uart_write_line("SUCCESS");
    uart_write_uint8(game->level);
    uart_write_char('\r');
    uart_write_char('\n');
}

static void show_failure_feedback(game_t *game) {
    display_show_failure();
    buzzer_silence();
    uart_write_line("GAME OVER");
    uart_write_uint8(game->last_score);
    uart_write_char('\r');
    uart_write_char('\n');
    uart_write_line("Enter name:");
}

static void start_playback(game_t *game) {
    apply_pending_seed(game);
    if (game->level >= game->config->max_sequence_length) {
        game->level = 0u;
    }
    uint8_t next = lfsr_next_button(&game->rng);
    game->sequence[game->level] = next;
    game->level++;
    game->state = GAME_STATE_PLAYBACK;
    game->step_index = 0u;

    display_show_level(game->level);

    uint32_t delay_ms = hal_current_delay_ms();
    uint32_t tone_ms = delay_ms / 2u;
    if (tone_ms < 100u) {
        tone_ms = 100u;
    }

    for (uint8_t i = 0u; i < game->level; ++i) {
        uint8_t button = game->sequence[i];
        display_show_sequence_step(button);
        buzzer_play(button, tone_ms, hal_get_octave_shift());
        hal_delay_ms(tone_ms);
        buzzer_silence();
        hal_delay_ms(delay_ms - tone_ms);
    }

    display_show_ready();
    game->state = GAME_STATE_AWAIT_INPUT;
    game->step_index = 0u;
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
    game->awaiting_name = false;
    game->seed_pending = false;
    display_show_level(0u);
    start_playback(game);
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
    display_show_level(0u);
    start_playback(game);
}

void game_print_status(const game_t *game) {
    (void)game;
}

static void handle_correct_input(game_t *game) {
    game->step_index++;
    if (game->step_index >= game->level) {
        show_success_feedback(game);
        hal_delay_ms(hal_current_delay_ms());
        start_playback(game);
    }
}

static void handle_failure(game_t *game) {
    display_show_failure();
    buzzer_silence();
    game->last_score = (game->level > 0u) ? (game->level - 1u) : 0u;
    if (game->last_score > 0u) {
        game->awaiting_name = true;
        game->state = GAME_STATE_AWAIT_NAME;
        show_failure_feedback(game);
    } else {
        game_reset(game);
    }
}

void game_handle_event(game_t *game, const input_event_t *event) {
    if (!game || !event) {
        return;
    }

    switch (event->type) {
        case INPUT_EVENT_PRESS: {
            if (game->state != GAME_STATE_AWAIT_INPUT) {
                return;
            }
            uint8_t expected = game->sequence[game->step_index];
            if (event->data.press.button == expected) {
                display_show_sequence_step(expected);
                uint32_t tone_ms = hal_current_delay_ms() / 2u;
                if (tone_ms < 100u) {
                    tone_ms = 100u;
                }
                buzzer_play(expected, tone_ms, hal_get_octave_shift());
                hal_delay_ms(tone_ms);
                buzzer_silence();
                handle_correct_input(game);
            } else {
                handle_failure(game);
            }
            break;
        }
        case INPUT_EVENT_RESET:
            game_reset(game);
            break;
        case INPUT_EVENT_OCTAVE_UP:
            hal_adjust_octave(+1);
            break;
        case INPUT_EVENT_OCTAVE_DOWN:
            hal_adjust_octave(-1);
            break;
        case INPUT_EVENT_SET_DELAY:
            hal_set_delay_index(event->data.delay.delay_index);
            break;
        case INPUT_EVENT_SEED:
            game->pending_seed = event->data.seed.seed;
            game->seed_pending = true;
            break;
        case INPUT_EVENT_HIGHSCORES:
            if (game->highscores.count == 0u) {
                uart_write_line("No highscores");
            } else {
                uart_write_line("Highscores:");
                for (size_t i = 0u; i < game->highscores.count; ++i) {
                    uart_write_uint8((uint8_t)(i + 1u));
                    uart_write_char('.');
                    uart_write_char(' ');
                    uart_write_string(game->highscores.entries[i].name);
                    uart_write_char(' ');
                    uart_write_uint8(game->highscores.entries[i].score);
                    uart_write_char('\r');
                    uart_write_char('\n');
                }
            }
            break;
        case INPUT_EVENT_SUBMIT_NAME:
            if (game->awaiting_name) {
                if (highscore_try_insert(&game->highscores, event->data.name.name, game->last_score)) {
                    uart_write_line("Score recorded");
                }
                game_reset(game);
            }
            break;
        case INPUT_EVENT_START:
            if (game->state == GAME_STATE_IDLE) {
                start_playback(game);
            }
            break;
        case INPUT_EVENT_HELP:
        case INPUT_EVENT_EXIT:
        case INPUT_EVENT_INVALID:
        default:
            break;
    }
}

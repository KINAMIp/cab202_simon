#include "simon.h"
#include "game.h"
#include "hardware.h"
#include "board.h"

/* Simon game state machine */
static void simon_handle_button_press(simon_game_t *game, uint8_t button)
{
    if (game->state == SIMON_STATE_WAIT_INPUT) {
        if (game->sequence[game->input_index] == button) {
            // Correct button press
            game->input_index++;
            if (game->input_index == game->level) {
                // Level complete
                game->state = SIMON_STATE_LEVEL_COMPLETE;
                game->input_index = 0;
            }
        } else {
            // Incorrect button press, game over
            game->state = SIMON_STATE_FAILURE;
            game->input_index = 0;
        }
    }
}

void simon_game_init(simon_game_t *game)
{
    game->state = SIMON_STATE_ATTRACT;
    game->level = 1;
    game->playback_index = 0;
    game->input_index = 0;
    game->playback_timer = 0;
    game->playback_delay_ms = 500;
    game->best_level = 1;
    game->rng_state = 0x1234;
    game->awaiting_seed = false;

    // Clear the high scores
    for (int i = 0; i < SIMON_HIGHSCORE_ENTRIES; i++) {
        game->highscores.entries[i].score = 0;
        game->highscores.entries[i].name[0] = '\0';
    }
}

static void simon_game_playback(simon_game_t *game)
{
    if (game->playback_index < game->level) {
        // Display the next color in the sequence
        uint8_t color = game->sequence[game->playback_index];
        hardware_display_pattern(color); // Display the color on the LED

        // Play the tone associated with the color
        hardware_set_buzzer_tone(color);

        // Wait for playback delay
        game->playback_timer = game->playback_delay_ms;
        game->playback_index++;
    } else {
        // Playback is complete, move to waiting for input
        game->state = SIMON_STATE_WAIT_INPUT;
    }
}

static void simon_game_wait_input(simon_game_t *game)
{
    // Handle user input (button press)
    uint8_t button_state = hardware_read_buttons();
    for (int i = 0; i < 4; i++) {
        if (button_state & (1 << i)) {
            simon_handle_button_press(game, i);
        }
    }
}

void simon_game_handle_event(simon_game_t *game, const board_event_t *event)
{
    switch (event->type) {
        case BOARD_EVENT_TICK:
            if (game->state == SIMON_STATE_ATTRACT) {
                // Display attract mode animation or message
                hardware_display_idle_animation(0);
            } else if (game->state == SIMON_STATE_PLAYBACK) {
                simon_game_playback(game);
            } else if (game->state == SIMON_STATE_WAIT_INPUT) {
                simon_game_wait_input(game);
            }
            break;

        case BOARD_EVENT_BUTTON:
            if (game->state == SIMON_STATE_WAIT_INPUT) {
                simon_handle_button_press(game, event->data.button.button);
            }
            break;

        case BOARD_EVENT_COMMAND:
            if (event->data.command.value == 'r') {
                // Reset the game on 'r' command
                game_reset(game);
            }
            break;

        default:
            break;
    }
}

void game_reset(simon_game_t *game)
{
    simon_game_init(game);  // Reset the game state
}


#include "simon.h"

#include <stdio.h>
#include <string.h>

#define DEFAULT_PLAYBACK_DELAY 600
#define MIN_PLAYBACK_DELAY 200
#define MAX_PLAYBACK_DELAY 1200
#define PLAYBACK_STEP_MS 300
#define LEVEL_COMPLETE_DELAY_MS 1500
#define FAILURE_DELAY_MS 1500

static uint32_t rng_next(uint32_t *state)
{
    *state = (*state * 1664525u) + 1013904223u;
    return *state;
}

static uint8_t rng_next_colour(uint32_t *state)
{
    return (uint8_t)(rng_next(state) % 4u);
}

static void highscores_init(simon_highscore_table_t *table)
{
    for (size_t i = 0; i < SIMON_HIGHSCORE_ENTRIES; ++i) {
        table->entries[i].name[0] = '\0';
        table->entries[i].score = 0;
    }
}

static void highscores_insert(simon_highscore_table_t *table, const char *name, uint16_t score)
{
    size_t pos = SIMON_HIGHSCORE_ENTRIES;
    for (size_t i = 0; i < SIMON_HIGHSCORE_ENTRIES; ++i) {
        if (score > table->entries[i].score) {
            pos = i;
            break;
        }
    }

    if (pos == SIMON_HIGHSCORE_ENTRIES) {
        return;
    }

    for (size_t i = SIMON_HIGHSCORE_ENTRIES - 1; i > pos; --i) {
        table->entries[i] = table->entries[i - 1];
    }

    strncpy(table->entries[pos].name, name, BOARD_MAX_TEXT - 1);
    table->entries[pos].name[BOARD_MAX_TEXT - 1] = '\0';
    table->entries[pos].score = score;
}

static void highscores_format(const simon_highscore_table_t *table, char *buffer, size_t buffer_len)
{
    buffer[0] = '\0';
    char line[64];
    for (size_t i = 0; i < SIMON_HIGHSCORE_ENTRIES; ++i) {
        const simon_highscore_entry_t *entry = &table->entries[i];
        if (entry->name[0] == '\0') {
            snprintf(line, sizeof line, "%zu. ---\n", i + 1);
        } else {
            snprintf(line, sizeof line, "%zu. %-10s %u\n", i + 1, entry->name, entry->score);
        }
        strncat(buffer, line, buffer_len - strlen(buffer) - 1);
    }
}

static void reset_game(simon_game_t *game)
{
    game->state = SIMON_STATE_ATTRACT;
    game->level = 0;
    game->playback_index = 0;
    game->input_index = 0;
    game->playback_timer = 0;
    game->playback_delay_ms = DEFAULT_PLAYBACK_DELAY;
    game->best_level = 0;
    game->awaiting_seed = false;
    board_show_idle_animation();
}

void simon_game_init(simon_game_t *game)
{
    game->rng_state = 0x12345678u;
    highscores_init(&game->highscores);
    reset_game(game);
    board_show_message("Type s1-s4 to begin a new game.");
}

static void start_round(simon_game_t *game)
{
    if (game->level < SIMON_MAX_SEQUENCE) {
        game->sequence[game->level] = rng_next_colour(&game->rng_state);
    }
    game->level++;
    if (game->level > game->best_level) {
        game->best_level = game->level;
    }
    game->playback_index = 0;
    game->input_index = 0;
    game->playback_timer = game->playback_delay_ms;
    game->state = SIMON_STATE_PLAYBACK;
    board_show_message("-- Playback starting --");
}

static void handle_tick_playback(simon_game_t *game)
{
    if (game->playback_timer > PLAYBACK_STEP_MS) {
        game->playback_timer -= PLAYBACK_STEP_MS;
        return;
    }

    if (game->playback_index < game->level) {
        board_show_playback_position(game->playback_index, game->level);
        board_show_color(game->sequence[game->playback_index]);
        game->playback_index++;
        game->playback_timer = game->playback_delay_ms;
    } else {
        board_show_message("Your turn! Use s1-s4 to repeat the pattern.");
        game->state = SIMON_STATE_WAIT_INPUT;
        game->input_index = 0;
    }
}

static void handle_tick_level_complete(simon_game_t *game)
{
    if (game->playback_timer > PLAYBACK_STEP_MS) {
        game->playback_timer -= PLAYBACK_STEP_MS;
        return;
    }

    if (game->playback_timer <= PLAYBACK_STEP_MS) {
        board_show_success(game->level);
        start_round(game);
    }
}

static void handle_tick_failure(simon_game_t *game)
{
    if (game->playback_timer > PLAYBACK_STEP_MS) {
        game->playback_timer -= PLAYBACK_STEP_MS;
        return;
    }

    if (game->playback_timer <= PLAYBACK_STEP_MS) {
        game->state = SIMON_STATE_NAME_ENTRY;
        board_show_prompt("Enter name with: name <your name>");
    }
}

static void update_pot(simon_game_t *game, uint16_t value)
{
    uint16_t scaled = MIN_PLAYBACK_DELAY + (uint16_t)(((uint32_t)value * (MAX_PLAYBACK_DELAY - MIN_PLAYBACK_DELAY)) / 1023u);
    game->playback_delay_ms = scaled;
    char msg[64];
    snprintf(msg, sizeof msg, "Playback delay set to %ums", (unsigned)scaled);
    board_show_message(msg);
}

static void adjust_frequency(simon_game_t *game, char command)
{
    switch (command) {
    case 'q':
        if (game->playback_delay_ms + 50 <= MAX_PLAYBACK_DELAY) {
            game->playback_delay_ms += 50;
        }
        break;
    case 'w':
        if (game->playback_delay_ms > MIN_PLAYBACK_DELAY + 50) {
            game->playback_delay_ms -= 50;
        }
        break;
    case 'r':
        reset_game(game);
        return;
    case 's':
        board_show_message("High score table:");
        char buffer[256];
        highscores_format(&game->highscores, buffer, sizeof buffer);
        board_show_high_scores(buffer);
        return;
    case 'p':
        board_show_score(game->best_level);
        return;
    default:
        board_show_message("Unknown command.");
        return;
    }

    char msg[64];
    snprintf(msg, sizeof msg, "Playback delay is now %ums", (unsigned)game->playback_delay_ms);
    board_show_message(msg);
}

static void register_failure(simon_game_t *game)
{
    board_show_failure(game->level > 0 ? game->level - 1 : 0);
    game->playback_timer = FAILURE_DELAY_MS;
    game->state = SIMON_STATE_FAILURE;
}

static void handle_button_press(simon_game_t *game, board_button_t button)
{
    if (game->state == SIMON_STATE_ATTRACT) {
        board_show_message("Game starting! Watch the sequence.");
        start_round(game);
        return;
    }

    if (game->state != SIMON_STATE_WAIT_INPUT) {
        board_show_message("Ignoring button press right now.");
        return;
    }

    uint8_t expected = game->sequence[game->input_index];
    if ((uint8_t)button == expected) {
        game->input_index++;
        char msg[64];
        snprintf(msg, sizeof msg, "Correct! (%u/%u)", (unsigned)game->input_index, (unsigned)game->level);
        board_show_message(msg);
        if (game->input_index == game->level) {
            game->state = SIMON_STATE_LEVEL_COMPLETE;
            game->playback_timer = LEVEL_COMPLETE_DELAY_MS;
            board_show_score(game->level);
        }
    } else {
        register_failure(game);
    }
}

static void handle_name_submission(simon_game_t *game, const char *name)
{
    if (game->state != SIMON_STATE_NAME_ENTRY) {
        board_show_message("Not expecting a name right now.");
        return;
    }

    if (name[0] == '\0') {
        board_show_message("Name cannot be empty.");
        return;
    }

    highscores_insert(&game->highscores, name, game->level - 1);
    char buffer[256];
    highscores_format(&game->highscores, buffer, sizeof buffer);
    board_show_high_scores(buffer);
    reset_game(game);
}

void simon_game_handle_event(simon_game_t *game, const board_event_t *event)
{
    switch (event->type) {
    case BOARD_EVENT_TICK:
        switch (game->state) {
        case SIMON_STATE_PLAYBACK:
            handle_tick_playback(game);
            break;
        case SIMON_STATE_LEVEL_COMPLETE:
            handle_tick_level_complete(game);
            break;
        case SIMON_STATE_FAILURE:
            handle_tick_failure(game);
            break;
        default:
            break;
        }
        break;
    case BOARD_EVENT_BUTTON:
        handle_button_press(game, event->data.button.button);
        break;
    case BOARD_EVENT_COMMAND:
        adjust_frequency(game, event->data.command.value);
        break;
    case BOARD_EVENT_TEXT:
        handle_name_submission(game, event->data.text.text);
        break;
    case BOARD_EVENT_POT:
        update_pot(game, event->data.pot.value);
        break;
    case BOARD_EVENT_QUIT:
        board_show_message("Quit command received.");
        reset_game(game);
        break;
    case BOARD_EVENT_NONE:
    default:
        break;
    }
}

#include "game.h"
#include "hardware.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define PLAYBACK_DELAY_MIN           250u
#define PLAYBACK_DELAY_MAX           2000u
#define PLAYBACK_DELAY_RANGE         (PLAYBACK_DELAY_MAX - PLAYBACK_DELAY_MIN)
#define PLAYBACK_DELAY_SCALE_BITS    10u
#define LEVEL_ADVANCE_PAUSE          800u
#define FAILURE_PAUSE                1200u
#define NAME_TIMEOUT_MS              5000u
#define IDLE_ANIMATION_INTERVAL_MASK 0x1Fu
#define IDLE_ANIMATION_FRAME_SHIFT   5u
#define OCTAVE_SHIFT_MAX             3
#define OCTAVE_SHIFT_MIN            -3

static const uint8_t display_patterns[4] = {
    0x01u,
    0x02u,
    0x04u,
    0x08u,
};

static const uint8_t success_pattern = 0b01111111u;
static const uint8_t failure_pattern = 0b01000000u;

static uint8_t lfsr_next_from_state(uint32_t *state)
{
    uint32_t value = *state;
    if (value == 0u) {
        value = 0xA5A5A5A5u;
    }

    uint32_t lsb = value & 1u;
    value >>= 1u;
    if (lsb != 0u) {
        value ^= 0x80200003u;
    }

    *state = value;
    return (uint8_t)(value & 0x03u);
}

static uint16_t map_pot_to_delay(uint16_t value)
{
    uint32_t scaled = (uint32_t)value * (uint32_t)PLAYBACK_DELAY_RANGE;
    scaled += (1u << (PLAYBACK_DELAY_SCALE_BITS - 1u));
    scaled >>= PLAYBACK_DELAY_SCALE_BITS;
    return (uint16_t)(PLAYBACK_DELAY_MIN + scaled);
}

static void apply_pending_playback_delay(simon_game_t *game)
{
    if (game->pot_update_pending) {
        game->playback_delay_ms = map_pot_to_delay(game->pending_pot_value);
        game->pot_update_pending = false;
    }
}

static uint16_t playback_tone_on_duration(const simon_game_t *game)
{
    uint16_t half = game->playback_delay_ms >> 1u;
    if ((game->playback_delay_ms & 0x01u) != 0u) {
        half++;
    }
    if (half == 0u) {
        half = 1u;
    }
    return half;
}

static uint16_t playback_tone_off_duration(const simon_game_t *game)
{
    uint16_t on = playback_tone_on_duration(game);
    uint16_t off = game->playback_delay_ms - on;
    if (off == 0u) {
        off = 1u;
    }
    return off;
}

static void append_decimal(char *buffer, uint16_t *pos, uint32_t value)
{
    char digits[6];
    uint8_t count = 0u;

    do {
        digits[count++] = (char)('0' + (uint8_t)(value % 10u));
        value /= 10u;
    } while (value > 0u && count < sizeof digits);

    while (count > 0u) {
        buffer[(*pos)++] = digits[--count];
    }
}

static void display_level_value(uint16_t value)
{
    if (value >= 100u) {
        hardware_display_pattern(success_pattern);
        return;
    }

    uint16_t remaining = value;
    uint8_t tens = 0u;
    while (remaining >= 10u) {
        remaining -= 10u;
        tens++;
    }

    if (tens == 0u) {
        hardware_display_segments(0xFFu, (uint8_t)remaining);
    } else {
        hardware_display_segments(tens, (uint8_t)remaining);
    }
}

static void uart_send_delay(const simon_game_t *game)
{
    char buffer[16];
    uint16_t pos = 0u;
    buffer[pos++] = 'D';
    buffer[pos++] = 'E';
    buffer[pos++] = 'L';
    buffer[pos++] = 'A';
    buffer[pos++] = 'Y';
    buffer[pos++] = ' ';
    append_decimal(buffer, &pos, game->playback_delay_ms);
    buffer[pos++] = '\0';
    hardware_uart_write_string(buffer);
    hardware_uart_write_string("\r\n");
}

static void uart_send_score(const char *label, uint16_t value)
{
    char buffer[24];
    uint16_t pos = 0u;

    while (label[pos] != '\0') {
        buffer[pos] = label[pos];
        pos++;
    }

    append_decimal(buffer, &pos, value);
    buffer[pos++] = '\0';
    hardware_uart_write_string(buffer);
    hardware_uart_write_string("\r\n");
}

static void initialise_highscores(simon_highscore_table_t *table)
{
    for (size_t i = 0; i < SIMON_HIGHSCORE_ENTRIES; ++i) {
        table->entries[i].name[0] = '\0';
        table->entries[i].score = 0u;
    }
}

static void format_highscore_table(const simon_highscore_table_t *table, char *buffer, size_t size)
{
    size_t pos = 0u;

    for (size_t i = 0; i < SIMON_HIGHSCORE_ENTRIES && pos + 1u < size; ++i) {
        const simon_highscore_entry_t *entry = &table->entries[i];
        int written = snprintf(
            buffer + pos,
            size - pos,
            "%zu. %-8s %4u\n",
            i + 1u,
            entry->name[0] == '\0' ? "---" : entry->name,
            entry->score);
        if (written < 0) {
            break;
        }
        pos += (size_t)written;
    }

    if (pos >= size) {
        buffer[size - 1u] = '\0';
    }
}

static bool highscore_qualifies(const simon_highscore_table_t *table, uint16_t score)
{
    return score > table->entries[SIMON_HIGHSCORE_ENTRIES - 1u].score;
}

static void insert_highscore(simon_highscore_table_t *table, const char *name, uint16_t score)
{
    size_t insert_index = SIMON_HIGHSCORE_ENTRIES;

    for (size_t i = 0; i < SIMON_HIGHSCORE_ENTRIES; ++i) {
        if (score > table->entries[i].score) {
            insert_index = i;
            break;
        }
    }

    if (insert_index >= SIMON_HIGHSCORE_ENTRIES) {
        return;
    }

    for (size_t i = SIMON_HIGHSCORE_ENTRIES - 1u; i > insert_index; --i) {
        table->entries[i] = table->entries[i - 1u];
    }

    simon_highscore_entry_t *entry = &table->entries[insert_index];
    strncpy(entry->name, name, SIMON_MAX_NAME_LENGTH - 1u);
    entry->name[SIMON_MAX_NAME_LENGTH - 1u] = '\0';
    entry->score = score;
}

static void finalise_pending_highscore(simon_game_t *game)
{
    const char *name = game->name_buffer[0] == '\0' ? "???" : game->name_buffer;
    insert_highscore(&game->highscores, name, game->pending_score);

    char table_buffer[256];
    format_highscore_table(&game->highscores, table_buffer, sizeof table_buffer);
    board_show_high_scores(table_buffer);
    hardware_uart_write_string("HIGHSCORES\r\n");
    hardware_uart_write_string(table_buffer);

    game->pending_score = 0u;
    game->pending_highscore = false;
    game_end(game);
}

static void prompt_for_name(simon_game_t *game)
{
    game->state = SIMON_STATE_NAME_ENTRY;
    game->name_length = 0u;
    game->name_buffer[0] = '\0';
    game->name_timeout = NAME_TIMEOUT_MS;
    board_show_prompt("Enter name: ");
    hardware_uart_write_string("NAME?\r\n");
}

static void reset_round_state(simon_game_t *game)
{
    game->playback_step = 0u;
    game->input_step = 0u;
    game->playback_timer = 0u;
    game->playback_tone_active = false;
    game->pending_success = false;
    game->state_timer = 0u;
}

static void extend_sequence(simon_game_t *game)
{
    if (game->level < SIMON_MAX_SEQUENCE) {
        (void)lfsr_next_from_state(&game->rng_state);
        game->level++;
    }
}

static void begin_playback(simon_game_t *game)
{
    apply_pending_playback_delay(game);
    reset_round_state(game);
    game->state = SIMON_STATE_PLAYBACK;
    game->playback_state = game->sequence_seed;
    hardware_display_pattern(0u);
    board_show_playback_position(0u, game->level);
}

static void update_best_score(simon_game_t *game, uint16_t score)
{
    if (score > game->best_score) {
        game->best_score = score;
        uart_send_score("BEST ", score);
    }
}

static void enter_level_complete_state(simon_game_t *game)
{
    game->state = SIMON_STATE_LEVEL_COMPLETE;
    game->state_timer = LEVEL_ADVANCE_PAUSE;
    game->pending_success = true;
    game->score = game->level;
    board_show_success(game->level);
    hardware_display_pattern(success_pattern);
    display_level_value(game->level);
    update_best_score(game, game->level);
}

static void enter_failure_state(simon_game_t *game, uint16_t final_score)
{
    game->state = SIMON_STATE_FAILURE;
    game->state_timer = FAILURE_PAUSE;
    game->score = final_score;
    hardware_stop_buzzer();
    hardware_display_pattern(failure_pattern);
    board_show_failure(final_score);
    update_best_score(game, final_score);

    if (highscore_qualifies(&game->highscores, final_score)) {
        game->pending_score = final_score;
        game->pending_highscore = true;
    } else {
        game->pending_score = 0u;
        game->pending_highscore = false;
    }
}

static int8_t mask_to_index(uint8_t button_mask)
{
    for (uint8_t i = 0u; i < 4u; ++i) {
        if ((button_mask & (1u << i)) != 0u) {
            return (int8_t)i;
        }
    }
    return -1;
}

static void handle_name_text(simon_game_t *game, const char *text)
{
    strncpy(game->name_buffer, text, SIMON_MAX_NAME_LENGTH - 1u);
    game->name_buffer[SIMON_MAX_NAME_LENGTH - 1u] = '\0';
    game->name_length = (uint8_t)strlen(game->name_buffer);
    finalise_pending_highscore(game);
}

static void apply_seed_from_buffer(simon_game_t *game)
{
    uint32_t seed = 0u;
    for (uint8_t i = 0u; i < game->seed_length; ++i) {
        seed = (seed * 33u) + (uint8_t)game->seed_buffer[i];
    }
    if (seed != 0u) {
        game->rng_state ^= seed;
    }
    game->awaiting_seed = false;
    game->seed_length = 0u;
    game->seed_buffer[0] = '\0';
    hardware_uart_write_string("SEED OK\r\n");
}

static void handle_seed_char(simon_game_t *game, char value)
{
    if (!game->awaiting_seed) {
        return;
    }

    if (value == '\n' || value == '\r') {
        apply_seed_from_buffer(game);
        return;
    }

    if (game->seed_length + 1u < SIMON_MAX_NAME_LENGTH) {
        game->seed_buffer[game->seed_length++] = value;
        game->seed_buffer[game->seed_length] = '\0';
    }
}

static void show_highscores(const simon_game_t *game)
{
    char table_buffer[256];
    format_highscore_table(&game->highscores, table_buffer, sizeof table_buffer);
    board_show_high_scores(table_buffer);
    hardware_uart_write_string(table_buffer);
}

void game_init(simon_game_t *game)
{
    game->level = 0u;
    game->playback_step = 0u;
    game->input_step = 0u;
    game->playback_delay_ms = PLAYBACK_DELAY_MIN;
    game->best_score = 0u;
    game->score = 0u;
    game->state = SIMON_STATE_ATTRACT;
    game->rng_state = 0x1u;
    game->sequence_seed = game->rng_state;
    game->playback_state = game->rng_state;
    game->input_state = game->rng_state;
    game->pot_update_pending = false;
    game->playback_tone_active = false;
    game->pending_success = false;
    game->pending_highscore = false;
    game->octave_shift = 0;
    game->name_length = 0u;
    game->seed_length = 0u;
    game->name_timeout = NAME_TIMEOUT_MS;
    game->pot_value = 0u;
    game->awaiting_seed = false;
    game->pending_pot_value = 0u;
    game->playback_timer = 0u;
    game->state_timer = 0u;
    game->pending_score = 0u;
    game->idle_frame = 0u;
    game->name_buffer[0] = '\0';
    game->seed_buffer[0] = '\0';

    initialise_highscores(&game->highscores);
    board_show_message("Welcome to Simon!");
    hardware_display_pattern(0u);
}

void game_tick_1ms(simon_game_t *game)
{
    apply_pending_playback_delay(game);

    switch (game->state) {
    case SIMON_STATE_ATTRACT:
        game->state_timer++;
        if ((game->state_timer & IDLE_ANIMATION_INTERVAL_MASK) == 0u) {
            game->idle_frame++;
            hardware_display_idle_animation(game->idle_frame);
        }
        break;

    case SIMON_STATE_PLAYBACK:
        if (game->playback_step >= game->level) {
            game->state = SIMON_STATE_WAIT_INPUT;
            game->input_step = 0u;
            game->input_state = game->sequence_seed;
            hardware_stop_buzzer();
            hardware_display_pattern(0u);
            break;
        }

        if (game->playback_timer > 0u) {
            game->playback_timer--;
            break;
        }

        if (!game->playback_tone_active) {
            uint8_t index = lfsr_next_from_state(&game->playback_state);
            hardware_set_buzzer_tone(index);
            hardware_display_pattern(display_patterns[index]);
            board_show_playback_position(game->playback_step + 1u, game->level);
            game->playback_tone_active = true;
            game->playback_timer = playback_tone_on_duration(game);
        } else {
            hardware_stop_buzzer();
            hardware_display_pattern(0u);
            game->playback_tone_active = false;
            game->playback_timer = playback_tone_off_duration(game);
            game->playback_step++;
        }
        break;

    case SIMON_STATE_WAIT_INPUT:
        break;

    case SIMON_STATE_LEVEL_COMPLETE:
        if (game->pending_success) {
            game->pending_success = false;
            hardware_display_pattern(success_pattern);
        }
        if (game->state_timer > 0u) {
            game->state_timer--;
        } else {
            if (game->level < SIMON_MAX_SEQUENCE) {
                extend_sequence(game);
                begin_playback(game);
            } else {
                game_end(game);
            }
        }
        break;

    case SIMON_STATE_FAILURE:
        if (game->state_timer > 0u) {
            game->state_timer--;
        } else {
            if (game->pending_highscore) {
                prompt_for_name(game);
            } else {
                game_end(game);
            }
        }
        break;

    case SIMON_STATE_NAME_ENTRY:
        if (game->name_timeout > 0u) {
            game->name_timeout--;
            if (game->name_timeout == 0u) {
                finalise_pending_highscore(game);
            }
        }
        break;
    }
}

void game_handle_button(simon_game_t *game, uint8_t button_mask)
{
    if (game->state == SIMON_STATE_ATTRACT) {
        game_start(game);
    }

    if (game->state != SIMON_STATE_WAIT_INPUT) {
        return;
    }

    int8_t index = mask_to_index(button_mask);
    if (index < 0) {
        return;
    }

    uint8_t button = (uint8_t)index;
    board_show_color(button);

    if (game->input_step >= game->level) {
        return;
    }

    uint8_t expected = lfsr_next_from_state(&game->input_state);

    if (button == expected) {
        game->input_step++;
        board_show_playback_position(game->input_step, game->level);
        if (game->input_step >= game->level) {
            enter_level_complete_state(game);
        }
    } else {
        uint16_t completed = (game->input_step == 0u) ? 0u : game->input_step;
        enter_failure_state(game, completed);
    }
}

void game_update_playback_delay(simon_game_t *game, uint16_t pot_value)
{
    game->pot_value = pot_value;
    game->pending_pot_value = pot_value;
    game->pot_update_pending = true;
    uart_send_delay(game);
}

void game_handle_uart_char(simon_game_t *game, char value)
{
    if (game->state == SIMON_STATE_NAME_ENTRY) {
        if (value == '\r' || value == '\n') {
            finalise_pending_highscore(game);
        } else if (value == '\b') {
            if (game->name_length > 0u) {
                game->name_length--;
                game->name_buffer[game->name_length] = '\0';
            }
        } else if (game->name_length + 1u < SIMON_MAX_NAME_LENGTH && isprint((unsigned char)value)) {
            game->name_buffer[game->name_length++] = value;
            game->name_buffer[game->name_length] = '\0';
        }
        return;
    }

    switch (value) {
    case 's':
    case 'S':
        game_start(game);
        break;

    case 'r':
    case 'R':
        game_reset(game);
        break;

    case 'd':
    case 'D':
        uart_send_delay(game);
        break;

    case 'b':
    case 'B':
        uart_send_score("BEST ", game->best_score);
        break;

    case 'c':
    case 'C':
        uart_send_score("SCORE ", game->score);
        break;

    case 'h':
    case 'H':
        show_highscores(game);
        break;

    case '+':
        if (game->octave_shift < OCTAVE_SHIFT_MAX) {
            game->octave_shift++;
            hardware_set_buzzer_octave_shift(game->octave_shift);
        }
        break;

    case '-':
        if (game->octave_shift > OCTAVE_SHIFT_MIN) {
            game->octave_shift--;
            hardware_set_buzzer_octave_shift(game->octave_shift);
        }
        break;

    case 'g':
    case 'G':
        game->awaiting_seed = true;
        game->seed_length = 0u;
        game->seed_buffer[0] = '\0';
        hardware_uart_write_string("SEED MODE\r\n");
        break;

    default:
        handle_seed_char(game, value);
        break;
    }
}

void game_handle_event(simon_game_t *game, const board_event_t *event)
{
    switch (event->type) {
    case BOARD_EVENT_TICK:
        break;

    case BOARD_EVENT_BUTTON: {
        uint8_t mask = (uint8_t)(1u << (uint8_t)event->data.button.button);
        game_handle_button(game, mask);
        break;
    }

    case BOARD_EVENT_COMMAND:
        game_handle_uart_char(game, event->data.command.value);
        break;

    case BOARD_EVENT_TEXT:
        if (game->state == SIMON_STATE_NAME_ENTRY) {
            handle_name_text(game, event->data.text.text);
        } else if (game->awaiting_seed) {
            strncpy(game->seed_buffer, event->data.text.text, SIMON_MAX_NAME_LENGTH - 1u);
            game->seed_length = (uint8_t)strlen(game->seed_buffer);
            game->seed_buffer[SIMON_MAX_NAME_LENGTH - 1u] = '\0';
            apply_seed_from_buffer(game);
        }
        break;

    case BOARD_EVENT_POT:
        game_update_playback_delay(game, event->data.pot.value);
        break;

    case BOARD_EVENT_QUIT:
        game_end(game);
        break;

    case BOARD_EVENT_NONE:
    default:
        break;
    }
}

static void reset_for_new_game(simon_game_t *game)
{
    game->level = 0u;
    game->playback_step = 0u;
    game->input_step = 0u;
    game->score = 0u;
    game->state_timer = 0u;
    game->idle_frame = 0u;
    game->pending_success = false;
    game->pending_highscore = false;
    game->pending_score = 0u;
    game->sequence_seed = game->rng_state;
    game->playback_state = game->sequence_seed;
    game->input_state = game->sequence_seed;
    hardware_display_pattern(0u);
}

void game_reset(simon_game_t *game)
{
    reset_for_new_game(game);
    game->state = SIMON_STATE_ATTRACT;
    board_show_message("Game reset.");
}

void game_start(simon_game_t *game)
{
    reset_for_new_game(game);
    if (game->rng_state == 0u) {
        game->rng_state = 0x1u;
    }
    game->rng_state ^= (uint32_t)(game->pot_value + 1u) * 1103515245u;
    game->sequence_seed = game->rng_state;
    game->playback_state = game->sequence_seed;
    game->input_state = game->sequence_seed;
    extend_sequence(game);
    begin_playback(game);
    board_show_message("Starting game...");
}

void game_end(simon_game_t *game)
{
    game->state = SIMON_STATE_ATTRACT;
    game->state_timer = 0u;
    game->idle_frame = 0u;
    hardware_stop_buzzer();
    hardware_display_pattern(0u);
    board_show_message("Press a button to play.");
}

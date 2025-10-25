#include "game.h"

#include "hardware.h"

/*
 * Core Simon game state machine. The logic intentionally avoids
 * floating point arithmetic and integer division to satisfy the
 * CAB202 embedded coding guidelines. All sequencing is produced
 * on demand from an LFSR to meet the "generate on-the-fly"
 * requirement.
 */

#include <string.h>

#define PLAYBACK_DELAY_MIN          250u
#define PLAYBACK_DELAY_MAX          2000u
#define PLAYBACK_DELAY_RANGE        (PLAYBACK_DELAY_MAX - PLAYBACK_DELAY_MIN)
#define PLAYBACK_DELAY_SCALE_BITS   10u
#define LEVEL_ADVANCE_PAUSE         800u
#define FAILURE_PAUSE               1200u
#define NAME_TIMEOUT_MS             5000u
#define IDLE_ANIMATION_INTERVAL_MASK 0x1Fu
#define IDLE_ANIMATION_FRAME_SHIFT   5u

static const uint8_t display_patterns[4] = {
    0x01u,
    0x02u,
    0x04u,
    0x08u
};

static const uint8_t success_pattern = 0b01111111u;
static const uint8_t failure_pattern = 0b01000000u;

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

/* Append an unsigned integer to a buffer without division. */
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
    char buffer[20];
    uint16_t pos = 0u;
    while (label[pos] != '\0') {
        buffer[pos] = label[pos];
        pos++;
    }
    buffer[pos++] = ' ';
    append_decimal(buffer, &pos, value);
    buffer[pos++] = '\0';
    hardware_uart_write_string(buffer);
    hardware_uart_write_string("\r\n");
}

static void uart_send_octave(const simon_game_t *game)
{
    char buffer[16];
    uint16_t pos = 0u;
    buffer[pos++] = 'O';
    buffer[pos++] = 'C';
    buffer[pos++] = 'T';
    buffer[pos++] = 'A';
    buffer[pos++] = 'V';
    buffer[pos++] = 'E';
    buffer[pos++] = ':';
    int8_t shift = game->octave_shift;
    if (shift < 0) {
        buffer[pos++] = '-';
        shift = (int8_t)(-shift);
    } else {
        buffer[pos++] = ' ';
    }
    buffer[pos++] = (char)('0' + (uint8_t)shift);
    buffer[pos++] = '\0';
    hardware_uart_write_string(buffer);
    hardware_uart_write_string("\r\n");
}

/* Step a 32-bit Galois LFSR. */
static uint32_t lfsr_step(uint32_t value)
{
    if (value == 0u) {
        value = 0x1u;
    }
    uint32_t bit = value & 0x1u;
    value >>= 1u;
    if (bit != 0u) {
        value ^= 0xE2024CABu;
    }
    return value;
}

/* Derive the next Simon colour from the LFSR. */
static uint8_t next_colour(uint32_t *state)
{
    *state = lfsr_step(*state);
    return (uint8_t)(*state & 0x03u);
}

/* Convenience helper to push a CRLF-terminated message. */
static void uart_write_line(const char *text)
{
    hardware_uart_write_string(text);
    hardware_uart_write_string("\r\n");
}

/* Translate UART characters, including alternate keys, to button masks. */
static bool map_uart_to_button(char value, uint8_t *mask)
{
    switch (value) {
    case '1':
    case '!':
    case 'q':
    case 'Q':
        *mask = HARDWARE_BUTTON_S1_MASK;
        return true;
    case '2':
    case '@':
    case 'w':
    case 'W':
        *mask = HARDWARE_BUTTON_S2_MASK;
        return true;
    case '3':
    case '#':
    case 'e':
    case 'E':
        *mask = HARDWARE_BUTTON_S3_MASK;
        return true;
    case '4':
    case '$':
    case 'r':
    case 'R':
        *mask = HARDWARE_BUTTON_S4_MASK;
        return true;
    default:
        break;
    }
    return false;
}

/* Clear the high-score table to its default state. */
static void highscores_reset(simon_game_t *game)
{
    uint8_t index = 0u;
    while (index < SIMON_HIGHSCORE_COUNT) {
        uint8_t pos = 0u;
        while (pos < SIMON_NAME_LENGTH) {
            game->highscores[index].name[pos] = '\0';
            pos++;
        }
        game->highscores[index].score = 0u;
        index++;
    }
}

/* Insert a new score into the sorted leaderboard. */
static void highscores_insert(simon_game_t *game, const char *name, uint16_t score)
{
    uint8_t position = SIMON_HIGHSCORE_COUNT;
    uint8_t idx = 0u;
    while (idx < SIMON_HIGHSCORE_COUNT) {
        if (score > game->highscores[idx].score) {
            position = idx;
            break;
        }
        idx++;
    }

    if (position == SIMON_HIGHSCORE_COUNT) {
        return;
    }

    idx = SIMON_HIGHSCORE_COUNT - 1u;
    while (idx > position) {
        game->highscores[idx] = game->highscores[idx - 1u];
        idx--;
    }

    uint8_t copy_index = 0u;
    while (copy_index < (SIMON_NAME_LENGTH - 1u) && name[copy_index] != '\0') {
        game->highscores[position].name[copy_index] = name[copy_index];
        copy_index++;
    }
    game->highscores[position].name[copy_index] = '\0';
    copy_index++;
    while (copy_index < SIMON_NAME_LENGTH) {
        game->highscores[position].name[copy_index] = '\0';
        copy_index++;
    }
    game->highscores[position].score = score;
}

/* Emit the leaderboard over UART. */
static void highscores_print(const simon_game_t *game)
{
    uint8_t index = 0u;
    while (index < SIMON_HIGHSCORE_COUNT) {
        const simon_highscore_t *entry = &game->highscores[index];
        char buffer[SIMON_NAME_LENGTH + 8u];
        uint16_t pos = 0u;
        uint8_t name_index = 0u;
        while (name_index < SIMON_NAME_LENGTH && entry->name[name_index] != '\0') {
            buffer[pos++] = entry->name[name_index];
            name_index++;
        }
        buffer[pos++] = ' ';
        append_decimal(buffer, &pos, entry->score);
        buffer[pos++] = '\0';
        hardware_uart_write_string(buffer);
        hardware_uart_write_string("\r\n");
        index++;
    }
}

/* Return to the attract state and silence all peripherals. */
static void reset_game(simon_game_t *game)
{
    game->state = SIMON_STATE_ATTRACT;
    game->level = 0u;
    game->playback_step = 0u;
    game->input_step = 0u;
    game->countdown_ms = 0u;
    game->idle_ticks = 0u;
    game->name_length = 0u;
    game->name_timeout = 0u;
    game->last_score = 0u;
    game->playback_seed = game->base_seed;
    game->input_seed = game->base_seed;
    game->seed_length = 0u;
    game->awaiting_seed = false;
    apply_pending_playback_delay(game);
    game->playback_tone_active = false;
    game->pending_success = false;
    hardware_set_buzzer_octave_shift(game->octave_shift);
    hardware_stop_buzzer();
    hardware_display_idle_animation(0u);
    uart_write_line("READY");
}

/* Begin playback for the current level. */
static void start_round(simon_game_t *game)
{
    if (game->level < SIMON_MAX_LEVEL) {
        game->level++;
    }
    apply_pending_playback_delay(game);
    game->playback_seed = game->base_seed;
    game->playback_step = 0u;
    game->countdown_ms = playback_tone_off_duration(game);
    game->playback_tone_active = false;
    game->pending_success = false;
    game->state = SIMON_STATE_PLAYBACK;
    game->current_colour = 0u;
    hardware_set_buzzer_octave_shift(game->octave_shift);
    hardware_stop_buzzer();
    hardware_display_pattern(0u);
}

/* Hand control to the player for input comparison. */
static void begin_input_phase(simon_game_t *game)
{
    game->state = SIMON_STATE_INPUT;
    game->input_seed = game->base_seed;
    game->input_step = 0u;
    game->countdown_ms = 0u;
    game->playback_tone_active = false;
    game->pending_success = false;
    hardware_stop_buzzer();
    display_level_value(game->level);
    hardware_uart_write_string("INPUT\r\n");
}

/* Celebrate a successful round and schedule the next. */
static void on_level_complete(simon_game_t *game)
{
    game->pending_success = false;
    game->state = SIMON_STATE_LEVEL_COMPLETE;
    game->countdown_ms = LEVEL_ADVANCE_PAUSE;
    game->last_score = game->level;
    hardware_display_pattern(success_pattern);
    hardware_uart_write_string("SUCCESS\r\n");
}

/* Record a failure and prompt for a name entry. */
static void on_failure(simon_game_t *game)
{
    game->pending_success = false;
    game->state = SIMON_STATE_FAILURE;
    game->countdown_ms = FAILURE_PAUSE;
    if (game->level > 0u) {
        game->last_score = game->level - 1u;
    } else {
        game->last_score = 0u;
    }
    hardware_display_pattern(failure_pattern);
    hardware_stop_buzzer();
    uart_write_line("GAME OVER");
}

/* Request high-score name entry over UART. */
static void publish_name_prompt(void)
{
    hardware_uart_write_string("Enter name:\r\n");
}

/* Change the playback octave within the permitted range. */
static void adjust_octave(simon_game_t *game, int8_t delta)
{
    int8_t target = game->octave_shift;
    if (delta > 0) {
        if (target < SIMON_OCTAVE_MAX) {
            target++;
        }
    } else if (delta < 0) {
        if (target > SIMON_OCTAVE_MIN) {
            target--;
        }
    }
    if (target != game->octave_shift) {
        game->octave_shift = target;
        hardware_set_buzzer_octave_shift(target);
        uart_send_octave(game);
    }
}

void game_init(simon_game_t *game)
{
    memset(game, 0, sizeof *game);
    game->base_seed = 0x12345678u;
    game->playback_delay_ms = map_pot_to_delay(0u);
    game->pending_pot_value = 0u;
    game->pot_update_pending = false;
    game->octave_shift = 0;
    hardware_set_buzzer_octave_shift(0);
    highscores_reset(game);
    reset_game(game);
}

void game_tick_1ms(simon_game_t *game)
{
    switch (game->state) {
    case SIMON_STATE_ATTRACT:
        game->idle_ticks++;
        if ((game->idle_ticks & IDLE_ANIMATION_INTERVAL_MASK) == 0u) {
            uint8_t frame = (uint8_t)((game->idle_ticks >> IDLE_ANIMATION_FRAME_SHIFT) & 0x03u);
            hardware_display_idle_animation(frame);
        }
        break;
    case SIMON_STATE_PLAYBACK:
        if (game->countdown_ms > 0u) {
            game->countdown_ms--;
            if (game->countdown_ms == 0u) {
                if (game->playback_tone_active) {
                    hardware_stop_buzzer();
                    hardware_display_pattern(0u);
                    game->playback_tone_active = false;
                    game->countdown_ms = playback_tone_off_duration(game);
                } else {
                    if (game->playback_step >= game->level) {
                        begin_input_phase(game);
                    } else {
                        game->current_colour = next_colour(&game->playback_seed);
                        hardware_set_buzzer_tone(game->current_colour);
                        hardware_display_pattern(display_patterns[game->current_colour]);
                        game->playback_step++;
                        game->playback_tone_active = true;
                        game->countdown_ms = playback_tone_on_duration(game);
                    }
                }
            }
        }
        break;
    case SIMON_STATE_INPUT:
        if (game->playback_tone_active && game->countdown_ms > 0u) {
            game->countdown_ms--;
            if (game->countdown_ms == 0u) {
                hardware_stop_buzzer();
                hardware_display_pattern(0u);
                game->playback_tone_active = false;
                display_level_value(game->level);
                if (game->pending_success) {
                    game->countdown_ms = playback_tone_off_duration(game);
                }
            }
        } else if (game->pending_success && game->countdown_ms > 0u) {
            game->countdown_ms--;
            if (game->countdown_ms == 0u) {
                game->pending_success = false;
                on_level_complete(game);
            }
        }
        break;
    case SIMON_STATE_LEVEL_COMPLETE:
        if (game->countdown_ms > 0u) {
            game->countdown_ms--;
            if (game->countdown_ms == 0u) {
                start_round(game);
            }
        }
        break;
    case SIMON_STATE_FAILURE:
        if (game->countdown_ms > 0u) {
            game->countdown_ms--;
            if (game->countdown_ms == 0u) {
                game->state = SIMON_STATE_NAME_ENTRY;
                game->name_length = 0u;
                game->name_timeout = NAME_TIMEOUT_MS;
                publish_name_prompt();
            }
        }
        break;
    case SIMON_STATE_NAME_ENTRY:
        if (game->name_timeout > 0u) {
            game->name_timeout--;
            if (game->name_timeout == 0u) {
                reset_game(game);
            }
        }
        break;
    default:
        reset_game(game);
        break;
    }
}

/* Translate a button mask into a colour index. */
static uint8_t mask_to_button_index(uint8_t mask)
{
    if ((mask & HARDWARE_BUTTON_S1_MASK) != 0u) {
        return 0u;
    }
    if ((mask & HARDWARE_BUTTON_S2_MASK) != 0u) {
        return 1u;
    }
    if ((mask & HARDWARE_BUTTON_S3_MASK) != 0u) {
        return 2u;
    }
    if ((mask & HARDWARE_BUTTON_S4_MASK) != 0u) {
        return 3u;
    }
    return 0xFFu;
}

/* Process button presses according to the current state. */
void game_handle_button(simon_game_t *game, uint8_t button_mask)
{
    uint8_t button_index = mask_to_button_index(button_mask);
    if (button_index == 0xFFu) {
        return;
    }

    switch (game->state) {
    case SIMON_STATE_ATTRACT:
        game->base_seed = lfsr_step(game->base_seed ^ ((uint32_t)game->playback_delay_ms << 4u));
        start_round(game);
        break;
    case SIMON_STATE_INPUT: {
        if (game->pending_success) {
            return;
        }
        uint8_t expected = next_colour(&game->input_seed);
        if (button_index == expected) {
            game->input_step++;
            hardware_set_buzzer_tone(button_index);
            hardware_display_pattern(display_patterns[button_index]);
            game->playback_tone_active = true;
            game->countdown_ms = playback_tone_on_duration(game);
            if (game->input_step == game->level) {
                game->pending_success = true;
            }
        } else {
            hardware_stop_buzzer();
            game->playback_tone_active = false;
            game->pending_success = false;
            on_failure(game);
        }
        break;
    }
    default:
        break;
    }
}

/* Decode UART commands into gameplay actions. */
static void apply_command(simon_game_t *game, char value)
{
    switch (value) {
    case 's':
    case 'S':
        highscores_print(game);
        break;
    case 'd':
    case 'D':
        uart_send_delay(game);
        break;
    case 'l':
    case 'L':
    case 'j':
    case 'J':
        adjust_octave(game, (int8_t)-1);
        break;
    case 'k':
    case 'K':
    case 'i':
    case 'I':
        adjust_octave(game, 1);
        break;
    case 'p':
    case 'P':
        uart_send_score("SCORE", game->last_score);
        break;
    case '0':
        game->octave_shift = 0;
        hardware_set_buzzer_octave_shift(0);
        reset_game(game);
        break;
    case 'h':
    case 'H':
        highscores_print(game);
        break;
    default:
        break;
    }
}

/* Handle UART characters for commands or name entry. */
void game_handle_uart_char(simon_game_t *game, char value)
{
    if (game->state == SIMON_STATE_NAME_ENTRY) {
        if (value == '\r' || value == '\n') {
            if (game->name_length > 0u) {
                game->name_buffer[game->name_length] = '\0';
                highscores_insert(game, game->name_buffer, game->last_score);
                highscores_print(game);
            }
            reset_game(game);
            return;
        }
        if ((value == 0x08 || value == 0x7F) && game->name_length > 0u) {
            game->name_length--;
            game->name_buffer[game->name_length] = '\0';
            hardware_uart_write_string("\b \b");
            game->name_timeout = NAME_TIMEOUT_MS;
            return;
        }
        if (game->name_length < (SIMON_NAME_LENGTH - 1u)) {
            game->name_buffer[game->name_length++] = value;
            hardware_uart_write_char(value);
            game->name_timeout = NAME_TIMEOUT_MS;
        }
        return;
    }

    if (game->awaiting_seed) {
        if (value == '\r' || value == '\n') {
            if (game->seed_length == 8u) {
                if ((game->base_seed & 0x1u) == 0u) {
                    game->base_seed |= 0x1u;
                }
                if (game->base_seed == 0u) {
                    game->base_seed = 0x1u;
                }
                reset_game(game);
            }
            game->awaiting_seed = false;
            game->seed_length = 0u;
            return;
        }
        uint8_t digit;
        if (value >= '0' && value <= '9') {
            digit = (uint8_t)(value - '0');
        } else if (value >= 'A' && value <= 'F') {
            digit = (uint8_t)(value - 'A' + 10u);
        } else if (value >= 'a' && value <= 'f') {
            digit = (uint8_t)(value - 'a' + 10u);
        } else {
            game->awaiting_seed = false;
            game->seed_length = 0u;
            return;
        }
        if (game->seed_length < 8u) {
            game->base_seed = (game->base_seed << 4u) | (uint32_t)digit;
            game->seed_length++;
        } else {
            return;
        }
        return;
    }

    if (value == 'o' || value == 'O') {
        game->awaiting_seed = true;
        game->seed_length = 0u;
        game->base_seed = 0u;
        return;
    }

    uint8_t button_mask;
    if (map_uart_to_button(value, &button_mask)) {
        game_handle_button(game, button_mask);
        return;
    }

    apply_command(game, value);
}

/* Map the potentiometer reading into a playback delay. */
void game_update_playback_delay(simon_game_t *game, uint16_t pot_value)
{
    game->pending_pot_value = pot_value;
    switch (game->state) {
    case SIMON_STATE_ATTRACT:
    case SIMON_STATE_FAILURE:
    case SIMON_STATE_NAME_ENTRY:
        game->playback_delay_ms = map_pot_to_delay(pot_value);
        game->pot_update_pending = false;
        break;
    default:
        game->pot_update_pending = true;
        break;
    }
}

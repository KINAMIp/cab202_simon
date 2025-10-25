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

    append_decimal(buffer, &pos, value);
    buffer[pos++] = '\0';
    hardware_uart_write_string(buffer);
    hardware_uart_write_string("\r\n");
}

void game_init(simon_game_t *game)
{
    game->state = SIMON_STATE_ATTRACT;
    game->level = 0;
    game->playback_step = 0;
    game->input_step = 0;
    game->playback_delay_ms = PLAYBACK_DELAY_MIN;
    game->pot_update_pending = false;
    game->playback_tone_active = false;
    game->pending_success = false;
    game->octave_shift = 0;
    game->name_length = 0;
    game->seed_length = 0;
    game->name_timeout = NAME_TIMEOUT_MS;

    // Initialize UART and hardware
    hardware_init();
    hardware_task_display();
}

void game_tick_1ms(simon_game_t *game)
{
    if (game->state == SIMON_STATE_PLAYBACK) {
        // Handle playback of sequence
        uint16_t tone_on = playback_tone_on_duration(game);
        uint16_t tone_off = playback_tone_off_duration(game);
        
        if (game->playback_step < game->level) {
            // Activate buzzer and show the next color
            hardware_set_buzzer_tone(game->sequence[game->playback_step]);
            hardware_display_color(game->sequence[game->playback_step]);
            
            game->playback_step++;
        } else {
            // Completed the playback, transition to input state
            game->state = SIMON_STATE_WAIT_INPUT;
        }
    }
}

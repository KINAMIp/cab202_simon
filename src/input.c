#include "input.h"

#include <avr/io.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

#include "hal.h"
#include "uart.h"
#include "util.h"

#define BUTTON_MASK_S1 PIN4_bm
#define BUTTON_MASK_S2 PIN5_bm
#define BUTTON_MASK_S3 PIN6_bm
#define BUTTON_MASK_S4 PIN7_bm

#define BUTTON_DEBOUNCE_MS 20u
#define NAME_BUFFER_LEN (SIMON_MAX_NAME_LENGTH)
#define SEED_BUFFER_LEN 8u

typedef struct {
    uint8_t mask;
    uint8_t id;
    bool pressed;
    uint32_t last_change;
} button_state_t;

static button_state_t g_buttons[] = {
    {BUTTON_MASK_S1, 1u, false, 0u},
    {BUTTON_MASK_S2, 2u, false, 0u},
    {BUTTON_MASK_S3, 3u, false, 0u},
    {BUTTON_MASK_S4, 4u, false, 0u},
};

static char g_name_buffer[NAME_BUFFER_LEN + 1];
static uint8_t g_name_len = 0u;
static bool g_collecting_name = false;

static char g_seed_buffer[SEED_BUFFER_LEN + 1];
static uint8_t g_seed_len = 0u;

static bool process_button(button_state_t *btn, input_event_t *event) {
    bool current = !(PORTA.IN & btn->mask);
    uint32_t now = hal_millis();
    if (current != btn->pressed) {
        if ((now - btn->last_change) >= BUTTON_DEBOUNCE_MS) {
            btn->pressed = current;
            btn->last_change = now;
            if (current) {
                event->type = INPUT_EVENT_PRESS;
                event->data.press.button = btn->id;
                return true;
            }
        }
    }
    return false;
}

static bool hex_char_value(char c, uint8_t *value) {
    if (c >= '0' && c <= '9') {
        *value = (uint8_t)(c - '0');
        return true;
    }
    if (c >= 'a' && c <= 'f') {
        *value = (uint8_t)(c - 'a' + 10);
        return true;
    }
    if (c >= 'A' && c <= 'F') {
        *value = (uint8_t)(c - 'A' + 10);
        return true;
    }
    return false;
}

static bool emit_seed_event(input_event_t *event) {
    if (g_seed_len == 0u) {
        return false;
    }
    uint32_t seed = 0u;
    for (uint8_t i = 0u; i < g_seed_len; ++i) {
        uint8_t value = 0u;
        if (!hex_char_value(g_seed_buffer[i], &value)) {
            return false;
        }
        seed = (seed << 4) | value;
    }
    event->type = INPUT_EVENT_SEED;
    event->data.seed.seed = seed;
    g_seed_len = 0u;
    return true;
}

static bool emit_name_event(input_event_t *event) {
    if (!g_collecting_name || g_name_len == 0u) {
        return false;
    }
    event->type = INPUT_EVENT_SUBMIT_NAME;
    g_name_buffer[g_name_len] = '\0';
    util_strlcpy(event->data.name.name, g_name_buffer, sizeof(event->data.name.name));
    g_collecting_name = false;
    g_name_len = 0u;
    return true;
}

void input_init(void) {
    uint8_t mask = BUTTON_MASK_S1 | BUTTON_MASK_S2 | BUTTON_MASK_S3 | BUTTON_MASK_S4;
    PORTA.DIRCLR = mask;
    PORTA.PIN4CTRL = PORT_PULLUPEN_bm;
    PORTA.PIN5CTRL = PORT_PULLUPEN_bm;
    PORTA.PIN6CTRL = PORT_PULLUPEN_bm;
    PORTA.PIN7CTRL = PORT_PULLUPEN_bm;

    g_seed_len = 0u;
    g_collecting_name = false;
    g_name_len = 0u;
}

static bool process_uart_char(char ch, input_event_t *event) {
    switch (ch) {
        case '1':
        case 'e':
        case 'E':
            event->type = INPUT_EVENT_PRESS;
            event->data.press.button = 1u;
            return true;
        case '2':
        case 'r':
        case 'R':
            event->type = INPUT_EVENT_PRESS;
            event->data.press.button = 2u;
            return true;
        case '3':
        case 'f':
        case 'F':
            event->type = INPUT_EVENT_PRESS;
            event->data.press.button = 3u;
            return true;
        case '4':
        case 'g':
        case 'G':
            event->type = INPUT_EVENT_PRESS;
            event->data.press.button = 4u;
            return true;
        case 'q':
        case 'Q':
            event->type = INPUT_EVENT_OCTAVE_DOWN;
            return true;
        case 'w':
        case 'W':
            event->type = INPUT_EVENT_OCTAVE_UP;
            return true;
        case 'k':
        case 'K':
            event->type = INPUT_EVENT_RESET;
            return true;
        case 's':
        case 'S':
            return emit_seed_event(event);
        case 'n':
        case 'N':
            g_collecting_name = true;
            g_name_len = 0u;
            return false;
        case 'h':
        case 'H':
            event->type = INPUT_EVENT_HIGHSCORES;
            return true;
        case '\r':
        case '\n':
            if (emit_name_event(event)) {
                return true;
            }
            if (emit_seed_event(event)) {
                return true;
            }
            return false;
        default:
            break;
    }

    if (g_collecting_name) {
        if (g_name_len < NAME_BUFFER_LEN && ch >= 32 && ch < 127) {
            g_name_buffer[g_name_len++] = ch;
        }
        return false;
    }

    uint8_t value = 0u;
    if (hex_char_value(ch, &value)) {
        if (g_seed_len < SEED_BUFFER_LEN) {
            g_seed_buffer[g_seed_len++] = ch;
        }
        return false;
    }

    return false;
}

bool input_poll(input_event_t *event) {
    hal_sample_potentiometer();

    for (size_t i = 0u; i < sizeof(g_buttons) / sizeof(g_buttons[0]); ++i) {
        if (process_button(&g_buttons[i], event)) {
            return true;
        }
    }

    char ch;
    while (uart_read_char(&ch)) {
        if (process_uart_char(ch, event)) {
            return true;
        }
    }

    return false;
}

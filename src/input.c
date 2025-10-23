#include "input.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hal.h"
#include "util.h"

static bool parse_uint8(const char *text, uint8_t *out) {
    char *end = NULL;
    long value = strtol(text, &end, 10);
    if (text == end) {
        return false;
    }
    while (*end) {
        if (!isspace((unsigned char)*end)) {
            return false;
        }
        ++end;
    }
    if (value < 0 || value > 255) {
        return false;
    }
    *out = (uint8_t)value;
    return true;
}

static void set_error(char *buf, size_t len, const char *message) {
    if (!buf || len == 0u) {
        return;
    }
    util_strlcpy(buf, message, len);
}

bool input_parse_line(const char *line, input_event_t *event, char *error_buf, size_t error_buf_len) {
    if (!line || !event) {
        return false;
    }
    event->type = INPUT_EVENT_INVALID;
    char copy[128];
    util_strlcpy(copy, line, sizeof(copy));
    util_trim(copy);
    if (copy[0] == '\0') {
        set_error(error_buf, error_buf_len, "Empty command. Type 'help' for options.");
        return false;
    }
    char *command = strtok(copy, " \t");
    if (!command) {
        return false;
    }
    for (char *p = command; *p; ++p) {
        *p = (char)tolower((unsigned char)*p);
    }
    if (strcmp(command, "help") == 0) {
        event->type = INPUT_EVENT_HELP;
        return true;
    }
    if (strcmp(command, "start") == 0) {
        event->type = INPUT_EVENT_START;
        return true;
    }
    if (strcmp(command, "press") == 0) {
        char *token = strtok(NULL, " \t");
        if (!token) {
            set_error(error_buf, error_buf_len, "Usage: press <1-4>");
            return false;
        }
        uint8_t value = 0u;
        if (!parse_uint8(token, &value) || value == 0u || value > 4u) {
            set_error(error_buf, error_buf_len, "Button must be between 1 and 4.");
            return false;
        }
        event->type = INPUT_EVENT_PRESS;
        event->data.press.button = value;
        return true;
    }
    if (strcmp(command, "delay") == 0) {
        char *token = strtok(NULL, " \t");
        if (!token) {
            set_error(error_buf, error_buf_len, "Usage: delay <0-9>");
            return false;
        }
        uint8_t value = 0u;
        if (!parse_uint8(token, &value)) {
            set_error(error_buf, error_buf_len, "Delay index must be numeric.");
            return false;
        }
        if (value >= config_get()->delay_steps) {
            set_error(error_buf, error_buf_len, "Delay index out of range.");
            return false;
        }
        event->type = INPUT_EVENT_SET_DELAY;
        event->data.delay.delay_index = value;
        return true;
    }
    if (strcmp(command, "octave") == 0) {
        char *token = strtok(NULL, " \t");
        if (!token) {
            set_error(error_buf, error_buf_len, "Usage: octave <up|down|set> [value]");
            return false;
        }
        for (char *p = token; *p; ++p) {
            *p = (char)tolower((unsigned char)*p);
        }
        if (strcmp(token, "up") == 0) {
            event->type = INPUT_EVENT_OCTAVE_UP;
            return true;
        }
        if (strcmp(token, "down") == 0) {
            event->type = INPUT_EVENT_OCTAVE_DOWN;
            return true;
        }
        if (strcmp(token, "set") == 0) {
            char *value_token = strtok(NULL, " \t");
            if (!value_token) {
                set_error(error_buf, error_buf_len, "Usage: octave set <value>");
                return false;
            }
            long value = strtol(value_token, &value_token, 10);
            if (*value_token != '\0') {
                set_error(error_buf, error_buf_len, "Invalid octave value.");
                return false;
            }
            if (value < SIMON_MIN_OCTAVE_SHIFT || value > SIMON_MAX_OCTAVE_SHIFT) {
                set_error(error_buf, error_buf_len, "Octave out of range.");
                return false;
            }
            if (value > hal_get_octave_shift()) {
                event->type = INPUT_EVENT_OCTAVE_UP;
            } else if (value < hal_get_octave_shift()) {
                event->type = INPUT_EVENT_OCTAVE_DOWN;
            } else {
                event->type = INPUT_EVENT_INVALID;
                set_error(error_buf, error_buf_len, "Octave already set to requested value.");
                return false;
            }
            return true;
        }
        set_error(error_buf, error_buf_len, "Unknown octave command. Use 'octave up' or 'octave down'.");
        return false;
    }
    if (strcmp(command, "reset") == 0) {
        event->type = INPUT_EVENT_RESET;
        return true;
    }
    if (strcmp(command, "seed") == 0) {
        char *token = strtok(NULL, " \t");
        if (!token) {
            set_error(error_buf, error_buf_len, "Usage: seed <hex value>");
            return false;
        }
        bool ok = false;
        uint32_t seed = util_parse_hex(token, &ok);
        if (!ok) {
            set_error(error_buf, error_buf_len, "Seed must be a hexadecimal number.");
            return false;
        }
        event->type = INPUT_EVENT_SEED;
        event->data.seed.seed = seed;
        return true;
    }
    if (strcmp(command, "highscores") == 0 || strcmp(command, "scores") == 0) {
        event->type = INPUT_EVENT_HIGHSCORES;
        return true;
    }
    if (strcmp(command, "name") == 0) {
        char *token = strtok(NULL, "");
        if (!token) {
            set_error(error_buf, error_buf_len, "Usage: name <text>");
            return false;
        }
        util_trim(token);
        if (token[0] == '\0') {
            set_error(error_buf, error_buf_len, "Name cannot be empty.");
            return false;
        }
        event->type = INPUT_EVENT_SUBMIT_NAME;
        util_strlcpy(event->data.name.name, token, sizeof(event->data.name.name));
        return true;
    }
    if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) {
        event->type = INPUT_EVENT_EXIT;
        return true;
    }
    /* allow shorthand button commands 1-4 */
    if (strlen(command) == 1 && command[0] >= '1' && command[0] <= '4') {
        event->type = INPUT_EVENT_PRESS;
        event->data.press.button = (uint8_t)(command[0] - '0');
        return true;
    }
    set_error(error_buf, error_buf_len, "Unknown command. Type 'help' for instructions.");
    return false;
}

void input_print_help(void) {
    printf("Commands:\n");
    printf("  help                Show this help text\n");
    printf("  start               Begin a new round\n");
    printf("  press <1-4>         Simulate a button press (shorthand: just type 1-4)\n");
    printf("  delay <0-%u>        Set playback delay index\n", config_get()->delay_steps - 1u);
    printf("  octave up|down      Adjust buzzer octave\n");
    printf("  reset               Reset the game\n");
    printf("  seed <hex>          Seed the random sequence generator\n");
    printf("  highscores          Show the high-score table\n");
    printf("  name <text>         Submit a name after winning\n");
    printf("  exit                Quit the program\n");
}

#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "config.h"

typedef enum {
    INPUT_EVENT_INVALID = 0,
    INPUT_EVENT_HELP,
    INPUT_EVENT_START,
    INPUT_EVENT_PRESS,
    INPUT_EVENT_SET_DELAY,
    INPUT_EVENT_OCTAVE_UP,
    INPUT_EVENT_OCTAVE_DOWN,
    INPUT_EVENT_RESET,
    INPUT_EVENT_SEED,
    INPUT_EVENT_HIGHSCORES,
    INPUT_EVENT_SUBMIT_NAME,
    INPUT_EVENT_EXIT
} input_event_type_t;

typedef struct {
    input_event_type_t type;
    union {
        struct {
            uint8_t button;
        } press;
        struct {
            uint8_t delay_index;
        } delay;
        struct {
            uint32_t seed;
        } seed;
        struct {
            char name[SIMON_MAX_NAME_LENGTH + 1];
        } name;
    } data;
} input_event_t;

bool input_parse_line(const char *line, input_event_t *event, char *error_buf, size_t error_buf_len);
void input_print_help(void);

#endif

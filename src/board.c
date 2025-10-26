#include "board.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROMPT "> "

typedef struct {
    const char *name;
    board_button_t button;
} button_alias_t;

static const button_alias_t button_aliases[] = {
    {"s1", BOARD_BUTTON_S1},
    {"s2", BOARD_BUTTON_S2},
    {"s3", BOARD_BUTTON_S3},
    {"s4", BOARD_BUTTON_S4},
};

static void trim(char *text)
{
    size_t len = strlen(text);
    while (len > 0 && (text[len - 1] == '\n' || text[len - 1] == '\r' || text[len - 1] == ' ' || text[len - 1] == '\t')) {
        text[--len] = '\0';
    }
    size_t start = 0;
    while (text[start] == ' ' || text[start] == '\t') {
        start++;
    }
    if (start > 0) {
        memmove(text, text + start, strlen(text + start) + 1);
    }
}

void board_init(void)
{
    puts("======================================");
    puts(" CAB202 Simon Emulator (Console Mode) ");
    puts("======================================");
    puts("Commands:");
    puts("  tick                -> advance virtual time");
    puts("  s1|s2|s3|s4         -> press a button");
    puts("  cmd <char>          -> send UART command character");
    puts("  name <text>         -> submit player name");
    puts("  pot <0-1023>        -> update potentiometer value");
    puts("  quit                -> exit");
    puts("Press ENTER without typing to emit a tick.");
    fputs(PROMPT, stdout);
    fflush(stdout);
}

void board_shutdown(void)
{
    puts("Exiting emulator.");
}

static board_event_t make_tick_event(void)
{
    board_event_t event = {.type = BOARD_EVENT_TICK};
    return event;
}

static board_event_t parse_line(char *line)
{
    trim(line);
    if (line[0] == '\0') {
        return make_tick_event();
    }

    if (strcmp(line, "tick") == 0) {
        return make_tick_event();
    }

    if (strcmp(line, "quit") == 0) {
        board_event_t event = {.type = BOARD_EVENT_QUIT};
        return event;
    }

    for (size_t i = 0; i < sizeof(button_aliases) / sizeof(button_aliases[0]); ++i) {
        if (strcmp(line, button_aliases[i].name) == 0) {
            board_event_t event = {
                .type = BOARD_EVENT_BUTTON,
                .data.button = {.button = button_aliases[i].button, .long_press = false},
            };
            return event;
        }
    }

    if (strncmp(line, "cmd ", 4) == 0 && line[4] != '\0') {
        board_event_t event = {
            .type = BOARD_EVENT_COMMAND,
            .data.command = {.value = line[4]},
        };
        return event;
    }

    if (strncmp(line, "name ", 5) == 0 && line[5] != '\0') {
        board_event_t event = {.type = BOARD_EVENT_TEXT};
        strncpy(event.data.text.text, line + 5, BOARD_MAX_TEXT - 1);
        event.data.text.text[BOARD_MAX_TEXT - 1] = '\0';
        return event;
    }

    if (strncmp(line, "pot ", 4) == 0 && line[4] != '\0') {
        char *end = NULL;
        long value = strtol(line + 4, &end, 10);
        if (end != line + 4) {
            if (value < 0) {
                value = 0;
            } else {
                if (value > 1023) {
                    value = 1023;
                }
            }
            board_event_t event = {
                .type = BOARD_EVENT_POT,
                .data.pot = {.value = (uint16_t)value},
            };
            return event;
        }
    }

    return (board_event_t){.type = BOARD_EVENT_NONE};
}

board_event_t board_wait_for_event(void)
{
    char line[BOARD_MAX_TEXT];

    fputs(PROMPT, stdout);
    fflush(stdout);

    if (fgets(line, sizeof(line), stdin) != NULL) {
        return parse_line(line);
    }

    return make_tick_event();
}

void board_show_message(const char *message)
{
    printf("%s\n", message);
}

void board_show_prompt(const char *prompt)
{
    printf("%s", prompt);
}

void board_show_color(uint8_t colour_index)
{
    printf("Color: %u\n", colour_index);
}

void board_show_idle_animation(void)
{
    printf("Idle animation running...\n");
}

void board_show_score(uint16_t score)
{
    printf("Score: %u\n", score);
}

void board_show_playback_position(uint8_t step, uint8_t total)
{
    printf("Playback Position: %u/%u\n", step, total);
}

void board_show_failure(uint16_t score)
{
    printf("Failure! Score: %u\n", score);
}

void board_show_success(uint16_t level)
{
    printf("Success! Level: %u\n", level);
}

void board_show_high_scores(const char *table_representation)
{
    printf("High Scores:\n%s", table_representation);
}

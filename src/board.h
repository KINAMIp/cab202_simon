#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>
#include <stdint.h>

#define BOARD_MAX_TEXT 32

typedef enum {
    BOARD_BUTTON_S1 = 0,
    BOARD_BUTTON_S2 = 1,
    BOARD_BUTTON_S3 = 2,
    BOARD_BUTTON_S4 = 3
} board_button_t;

typedef enum {
    BOARD_EVENT_NONE = 0,
    BOARD_EVENT_TICK,
    BOARD_EVENT_BUTTON,
    BOARD_EVENT_COMMAND,
    BOARD_EVENT_TEXT,
    BOARD_EVENT_POT,
    BOARD_EVENT_QUIT
} board_event_type_t;

typedef struct {
    board_event_type_t type;
    union {
        struct {
            board_button_t button;
            bool long_press;
        } button;
        struct {
            char value;
        } command;
        struct {
            char text[BOARD_MAX_TEXT];
        } text;
        struct {
            uint16_t value;
        } pot;
    } data;
} board_event_t;

void board_init(void);
void board_shutdown(void);
board_event_t board_wait_for_event(void);

void board_show_message(const char *message);
void board_show_prompt(const char *prompt);
void board_show_color(uint8_t colour_index);
void board_show_idle_animation(void);
void board_show_score(uint16_t score);
void board_show_playback_position(uint8_t step, uint8_t total);
void board_show_failure(uint16_t score);
void board_show_success(uint16_t level);
void board_show_high_scores(const char *table_representation);

#endif /* BOARD_H */

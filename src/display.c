#include "display.h"

#include <stdio.h>

static const char *button_patterns[] = {
    " ",
    "| ",
    "- ",
    " |",
    " _"
};

void display_init(void) {
    printf("[Display] Ready\n");
}

void display_show_level(uint8_t level) {
    if (level > 99u) {
        printf("[Display] Level 99+ -> 88 flashing\n");
    } else {
        printf("[Display] Level %u\n", level);
    }
}

void display_show_sequence_step(uint8_t button) {
    if (button < sizeof(button_patterns) / sizeof(button_patterns[0])) {
        printf("[Display] Step -> '%s'\n", button_patterns[button]);
    } else {
        printf("[Display] Step -> '?' (button %u)\n", button);
    }
}

void display_show_success(void) {
    printf("[Display] SUCCESS pattern\n");
}

void display_show_failure(void) {
    printf("[Display] FAIL pattern\n");
}

void display_show_ready(void) {
    printf("[Display] Ready for input\n");
}

#include "display.h"

#include <stddef.h>
#include <stdio.h>

/*
 * Character patterns that mimic the segment display for each button. Index 0 is
 * unused to keep the mapping aligned with the 1-based button identifiers.
 */
static const char *button_patterns[] = {
    " ",
    "| ",
    "- ",
    " |",
    " _"
};

/*
 * Prepare the simulated display hardware.
 */
void display_init(void) {
    printf("[Display] Ready\n");
}

/*
 * Show the current level, approximating how the physical display would behave.
 */
void display_show_level(uint8_t level) {
    if (level > 99u) {
        printf("[Display] Level 99+ -> 88 flashing\n");
    } else {
        printf("[Display] Level %u\n", level);
    }
}

/*
 * Visualise the active button using a simple character pattern.
 */
void display_show_sequence_step(uint8_t button) {
    size_t pattern_count = sizeof(button_patterns) / sizeof(button_patterns[0]);
    if (button < pattern_count) {
        printf("[Display] Step -> '%s'\n", button_patterns[button]);
    } else {
        printf("[Display] Step -> '?' (button %u)\n", button);
    }
}

/*
 * Indicate a successful round.
 */
void display_show_success(void) {
    printf("[Display] SUCCESS pattern\n");
}

/*
 * Indicate an incorrect response.
 */
void display_show_failure(void) {
    printf("[Display] FAIL pattern\n");
}

/*
 * Prompt the player to begin entering the sequence.
 */
void display_show_ready(void) {
    printf("[Display] Ready for input\n");
}

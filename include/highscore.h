#ifndef HIGHSCORE_H
#define HIGHSCORE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "config.h"

typedef struct {
    char name[SIMON_MAX_NAME_LENGTH + 1];
    uint8_t score;
} highscore_entry_t;

typedef struct {
    highscore_entry_t entries[SIMON_MAX_HIGHSCORES];
    size_t count;
} highscore_table_t;

void highscore_init(highscore_table_t *table);
void highscore_reset(highscore_table_t *table);
bool highscore_try_insert(highscore_table_t *table, const char *name, uint8_t score);
void highscore_print(const highscore_table_t *table);

#endif

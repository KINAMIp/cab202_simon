#include "highscore.h"

#include <stdio.h>
#include <string.h>

#include "util.h"

static void shift_down(highscore_table_t *table, size_t pos) {
    if (!table || table->count >= SIMON_MAX_HIGHSCORES) {
        return;
    }
    for (size_t i = table->count; i > pos; --i) {
        table->entries[i] = table->entries[i - 1];
    }
}

void highscore_init(highscore_table_t *table) {
    if (!table) {
        return;
    }
    table->count = 0u;
}

void highscore_reset(highscore_table_t *table) {
    if (!table) {
        return;
    }
    table->count = 0u;
}

bool highscore_try_insert(highscore_table_t *table, const char *name, uint8_t score) {
    if (!table || !name) {
        return false;
    }
    size_t insert_pos = 0u;
    while (insert_pos < table->count && table->entries[insert_pos].score >= score) {
        ++insert_pos;
    }
    if (table->count >= SIMON_MAX_HIGHSCORES && insert_pos >= SIMON_MAX_HIGHSCORES) {
        return false;
    }
    if (table->count < SIMON_MAX_HIGHSCORES) {
        shift_down(table, insert_pos);
        ++table->count;
    } else {
        for (size_t i = SIMON_MAX_HIGHSCORES - 1; i > insert_pos; --i) {
            table->entries[i] = table->entries[i - 1];
        }
    }
    util_strlcpy(table->entries[insert_pos].name, name, sizeof(table->entries[insert_pos].name));
    table->entries[insert_pos].score = score;
    return true;
}

void highscore_print(const highscore_table_t *table) {
    if (!table || table->count == 0u) {
        printf("No high scores recorded yet.\n");
        return;
    }
    printf("High Scores:\n");
    for (size_t i = 0; i < table->count; ++i) {
        printf(" %zu. %-*s %u\n", i + 1, SIMON_MAX_NAME_LENGTH, table->entries[i].name, table->entries[i].score);
    }
}

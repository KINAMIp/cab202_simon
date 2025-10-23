#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

void display_init(void);
void display_show_level(uint8_t level);
void display_show_sequence_step(uint8_t button);
void display_show_success(void);
void display_show_failure(void);
void display_show_ready(void);

#endif

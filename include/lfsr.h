#ifndef LFSR_H
#define LFSR_H

#include <stdint.h>

typedef struct {
    uint32_t state;
} lfsr_t;

void lfsr_seed(lfsr_t *lfsr, uint32_t seed);
uint32_t lfsr_next(lfsr_t *lfsr);
uint8_t lfsr_next_button(lfsr_t *lfsr);

#endif

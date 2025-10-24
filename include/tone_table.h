#ifndef TONE_TABLE_H
#define TONE_TABLE_H

#include <stddef.h>
#include <stdint.h>

/*
 * Provides access to the base frequencies (in Hertz) used by the buzzer module.
 * Values are stored as integers to comply with the embedded target restrictions
 * on floating-point usage.
 */
const uint32_t *tone_table_base_frequencies(size_t *count);

#endif

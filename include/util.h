#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

size_t util_strlcpy(char *dst, const char *src, size_t size);
void util_trim(char *str);
uint32_t util_parse_hex(const char *text, bool *ok);
uint32_t util_map_range(uint32_t value, uint32_t in_min, uint32_t in_max,
                        uint32_t out_min, uint32_t out_max);
uint32_t util_clamp_u32(uint32_t value, uint32_t min, uint32_t max);
int util_compare_u32_desc(const void *lhs, const void *rhs);

#endif

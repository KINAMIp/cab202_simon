#include "util.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/*
 * Bounded string copy that mirrors the BSD strlcpy semantics. Always
 * NUL-terminates the destination when size is non-zero and returns the length of
 * the source string.
 */
size_t util_strlcpy(char *dst, const char *src, size_t size) {
    size_t src_len = src ? strlen(src) : 0u;
    if (size != 0u && dst) {
        size_t to_copy = (src_len >= size) ? (size - 1u) : src_len;
        if (src && dst) {
            memcpy(dst, src, to_copy);
        }
        dst[to_copy] = '\0';
    }
    return src_len;
}

/*
 * Trim leading and trailing whitespace from the supplied string in-place.
 */
void util_trim(char *str) {
    if (!str) {
        return;
    }
    char *start = str;
    while (*start && isspace((unsigned char)*start)) {
        ++start;
    }
    char *end = start + strlen(start);
    while (end > start && isspace((unsigned char)*(end - 1))) {
        --end;
    }
    size_t len = (size_t)(end - start);
    if (start != str) {
        memmove(str, start, len);
    }
    str[len] = '\0';
}

/*
 * Parse a hexadecimal string into a 32-bit value. The ok flag is set to true
 * when parsing succeeds and false otherwise.
 */
uint32_t util_parse_hex(const char *text, bool *ok) {
    if (ok) {
        *ok = false;
    }
    if (!text) {
        return 0u;
    }
    while (isspace((unsigned char)*text)) {
        ++text;
    }
    if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        text += 2;
    }
    if (*text == '\0') {
        return 0u;
    }
    char *end = NULL;
    unsigned long value = strtoul(text, &end, 16);
    if (text == end) {
        return 0u;
    }
    while (*end) {
        if (!isspace((unsigned char)*end)) {
            return 0u;
        }
        ++end;
    }
    if (ok) {
        *ok = true;
    }
    return (uint32_t)value;
}

/*
 * Map a value from one numeric range to another using unsigned 64-bit
 * intermediate arithmetic to avoid overflow.
 */
uint32_t util_map_range(uint32_t value, uint32_t in_min, uint32_t in_max,
                        uint32_t out_min, uint32_t out_max) {
    if (in_max == in_min) {
        return out_min;
    }
    if (value <= in_min) {
        return out_min;
    }
    if (value >= in_max) {
        return out_max;
    }
    uint64_t numerator = (uint64_t)(value - in_min) * (out_max - out_min);
    uint64_t denominator = (uint64_t)(in_max - in_min);
    uint32_t result = (uint32_t)(numerator / denominator) + out_min;
    return result;
}

/*
 * Clamp an unsigned 32-bit integer between the supplied bounds.
 */
uint32_t util_clamp_u32(uint32_t value, uint32_t min, uint32_t max) {
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}

/*
 * Comparator used by qsort to order unsigned integers in descending order.
 */
int util_compare_u32_desc(const void *lhs, const void *rhs) {
    const uint32_t a = *(const uint32_t *)lhs;
    const uint32_t b = *(const uint32_t *)rhs;
    if (a < b) {
        return 1;
    }
    if (a > b) {
        return -1;
    }
    return 0;
}

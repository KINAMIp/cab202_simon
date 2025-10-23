#define _POSIX_C_SOURCE 199309L
#include "hal.h"

#include <stdio.h>
#include <time.h>

#include "config.h"
#include "util.h"

static uint8_t g_delay_index = SIMON_DEFAULT_DELAY_INDEX;
static int8_t g_octave_shift = 0;

static uint32_t delay_step_ms(void) {
    const simon_config_t *cfg = config_get();
    if (cfg->delay_steps <= 1u) {
        return cfg->min_delay_ms;
    }
    uint32_t span = cfg->max_delay_ms - cfg->min_delay_ms;
    return span / (cfg->delay_steps - 1u);
}

void hal_init(void) {
    g_delay_index = config_get()->default_delay_index;
    g_octave_shift = 0;
}

void hal_delay_ms(uint32_t ms) {
    struct timespec req;
    req.tv_sec = (time_t)(ms / 1000u);
    req.tv_nsec = (long)((ms % 1000u) * 1000000L);
    nanosleep(&req, NULL);
}

uint8_t hal_get_delay_index(void) {
    return g_delay_index;
}

void hal_set_delay_index(uint8_t index) {
    const simon_config_t *cfg = config_get();
    if (index >= cfg->delay_steps) {
        index = (uint8_t)(cfg->delay_steps - 1u);
    }
    g_delay_index = index;
    printf("[HAL] Playback delay index -> %u (delay %ums)\n", index, hal_current_delay_ms());
}

uint32_t hal_current_delay_ms(void) {
    const simon_config_t *cfg = config_get();
    uint32_t step = delay_step_ms();
    return cfg->min_delay_ms + step * g_delay_index;
}

int8_t hal_get_octave_shift(void) {
    return g_octave_shift;
}

void hal_adjust_octave(int8_t delta) {
    const simon_config_t *cfg = config_get();
    int8_t octave = g_octave_shift + delta;
    if (octave < cfg->min_octave) {
        octave = cfg->min_octave;
    }
    if (octave > cfg->max_octave) {
        octave = cfg->max_octave;
    }
    g_octave_shift = octave;
    printf("[HAL] Octave shift -> %+d\n", g_octave_shift);
}

void hal_set_octave(int8_t octave) {
    const simon_config_t *cfg = config_get();
    if (octave < cfg->min_octave) {
        octave = cfg->min_octave;
    }
    if (octave > cfg->max_octave) {
        octave = cfg->max_octave;
    }
    g_octave_shift = octave;
    printf("[HAL] Octave shift -> %+d\n", g_octave_shift);
}

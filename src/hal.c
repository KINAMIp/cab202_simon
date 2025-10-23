#define _POSIX_C_SOURCE 199309L
#include "hal.h"

#include <stdio.h>
#include <time.h>

#include "config.h"

/*
 * Runtime-configurable delay and octave state. These mimic hardware registers
 * on the embedded target while remaining testable in the host simulator.
 */
static uint8_t g_delay_index = SIMON_DEFAULT_DELAY_INDEX;
static int8_t g_octave_shift = 0;

/*
 * Determine the size of each delay step between the configured minimum and
 * maximum timings. The result is reused to translate the delay index to a
 * concrete millisecond value.
 */
static uint32_t delay_step_ms(void) {
    const simon_config_t *cfg = config_get();
    if (cfg->delay_steps <= 1u) {
        return cfg->min_delay_ms;
    }
    uint32_t span = cfg->max_delay_ms - cfg->min_delay_ms;
    return span / (cfg->delay_steps - 1u);
}

/*
 * Reset runtime state to the defaults defined in the configuration module.
 */
void hal_init(void) {
    g_delay_index = config_get()->default_delay_index;
    g_octave_shift = 0;
}

/*
 * Blocking millisecond delay implemented with nanosleep so the host simulator
 * behaves similarly to the embedded firmware.
 */
void hal_delay_ms(uint32_t ms) {
    struct timespec req;
    req.tv_sec = (time_t)(ms / 1000u);
    req.tv_nsec = (long)((ms % 1000u) * 1000000L);
    nanosleep(&req, NULL);
}

/*
 * Return the current delay index so the UI can reflect the active setting.
 */
uint8_t hal_get_delay_index(void) {
    return g_delay_index;
}

/*
 * Update the current playback delay index while clamping to the configured
 * range.
 */
void hal_set_delay_index(uint8_t index) {
    const simon_config_t *cfg = config_get();
    if (index >= cfg->delay_steps) {
        index = (uint8_t)(cfg->delay_steps - 1u);
    }
    g_delay_index = index;
    printf("[HAL] Playback delay index -> %u (delay %ums)\n", index, hal_current_delay_ms());
}

/*
 * Translate the current delay index into an absolute millisecond value.
 */
uint32_t hal_current_delay_ms(void) {
    const simon_config_t *cfg = config_get();
    uint32_t step = delay_step_ms();
    return cfg->min_delay_ms + step * g_delay_index;
}

/*
 * Expose the currently active octave offset.
 */
int8_t hal_get_octave_shift(void) {
    return g_octave_shift;
}

/*
 * Step the octave shift up or down, constraining the result to the supported
 * range for the hardware implementation.
 */
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

/*
 * Directly set the octave shift, clamping to the configuration limits.
 */
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

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdint.h>

#include "game.h"
#include "hardware.h"

/*
 * Firmware entry point. A 1 ms scheduler tick drives the Simon
 * state machine while the main loop polls for debounced buttons,
 * potentiometer updates, and UART commands.
 */

static volatile bool system_tick_flag;

/* Generate a millisecond tick using TCB1. */
ISR(TCB1_INT_vect)
{
    system_tick_flag = true;
    TCB1.INTFLAGS = TCB_CAPT_bm;
}

static void configure_system_tick(void)
{
    TCB1.CCMP = 3000u;
    TCB1.CTRLB = TCB_CNTMODE_INT_gc;
    TCB1.INTCTRL = TCB_CAPT_bm;
    TCB1.CTRLA = TCB_ENABLE_bm | TCB_CLKSEL_DIV1_gc;
}

/* Apply an 8-sample per-button shift-register debounce. */
static void process_buttons(simon_game_t *game, uint8_t sample)
{
    static uint8_t history[4];
    static uint8_t latched[4];

    uint8_t index = 0u;
    while (index < 4u) {
        history[index] <<= 1u;
        if (((sample >> index) & 0x01u) != 0u) {
            history[index] |= 0x01u;
        }

        if (history[index] == 0xFFu && latched[index] == 0u) {
            latched[index] = 1u;
            switch (index) {
            case 0u:
                game_handle_button(game, HARDWARE_BUTTON_S1_MASK);
                break;
            case 1u:
                game_handle_button(game, HARDWARE_BUTTON_S2_MASK);
                break;
            case 2u:
                game_handle_button(game, HARDWARE_BUTTON_S3_MASK);
                break;
            case 3u:
                game_handle_button(game, HARDWARE_BUTTON_S4_MASK);
                break;
            default:
                break;
            }
        } else if (history[index] == 0x00u) {
            latched[index] = 0u;
        }

        index++;
    }
}

int main(void)
{
    hardware_init();
    configure_system_tick();

    simon_game_t game;
    game_init(&game);

    sei();

    uint16_t pot_timer = 0u;
    for (;;) {
        if (!system_tick_flag) {
            continue;
        }
        system_tick_flag = false;

        game_tick_1ms(&game);

        uint8_t buttons = hardware_read_buttons();
        process_buttons(&game, buttons);

        pot_timer++;
        if (pot_timer >= 32u) {
            pot_timer = 0u;
            uint16_t pot_value = hardware_read_pot();
            game_update_playback_delay(&game, pot_value);
        }

        char uart_value;
        while (hardware_uart_read(&uart_value)) {
            game_handle_uart_char(&game, uart_value);
        }
    }
}

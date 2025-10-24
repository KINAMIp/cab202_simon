#include <avr/interrupt.h>

#include "buzzer.h"
#include "display.h"
#include "game.h"
#include "hal.h"
#include "input.h"
#include "uart.h"

int main(void) {
    hal_init();
    uart_init();
    buzzer_init();
    display_init();
    input_init();

    sei();

    game_t game;
    game_init(&game);

    input_event_t event;
    while (1) {
        if (input_poll(&event)) {
            game_handle_event(&game, &event);
        }
    }
}

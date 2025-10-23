#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "buzzer.h"
#include "display.h"
#include "game.h"
#include "hal.h"
#include "input.h"
#include "uart.h"

/*
 * Entry point for the simulator. The main loop continuously reads commands
 * from the UART abstraction and forwards them to the game state machine until
 * the player issues an exit request or the input stream terminates.
 */
int main(void) {
    hal_init();
    buzzer_init();
    display_init();

    game_t game;
    game_init(&game);
    game_print_status(&game);
    uart_writeln("Type 'help' to view available commands.");

    char line[128];
    input_event_t event;
    char error[128];

    while (true) {
        uart_write("> ");
        if (!uart_read_line(line, sizeof(line))) {
            break;
        }
        error[0] = '\0';
        if (!input_parse_line(line, &event, error, sizeof(error))) {
            if (error[0] != '\0') {
                uart_writef("[Error] %s\n", error);
            }
            continue;
        }
        if (event.type == INPUT_EVENT_EXIT) {
            uart_writeln("[Game] Exiting. Goodbye!");
            break;
        }
        game_handle_event(&game, &event);
    }
    return 0;
}

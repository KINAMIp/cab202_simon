#include "game.h"
#include "board.h"
#include "hardware.h"

int main(void)
{
    // Initialize hardware and board
    hardware_init();
    board_init();

    // Initialize the Simon game
    simon_game_t game;
    game_init(&game);

    // Start the game loop
    while (1) {
        // Wait for an event (button press, tick, etc.)
        board_event_t event = board_wait_for_event();

        // Handle the event
        game_handle_event(&game, &event);

        // Perform any game-related operations on each tick when not already
        // handled through an explicit tick event.
        if (event.type != BOARD_EVENT_TICK) {
            game_tick_1ms(&game);
        }

        // Update hardware if needed (LED, buzzer, etc.)
        hardware_task_display();
    }

    // Shutdown the board and hardware (this part will never be reached in this infinite loop)
    board_shutdown();
    return 0;
}

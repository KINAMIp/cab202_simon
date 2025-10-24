#include <stdbool.h>
#include <stdio.h>

#include "board.h"
#include "simon.h"

int main(void)
{
    board_init();

    simon_game_t game;
    simon_game_init(&game);

    bool running = true;
    while (running) {
        board_event_t event = board_wait_for_event();
        if (event.type == BOARD_EVENT_QUIT) {
            running = false;
        }
        simon_game_handle_event(&game, &event);
    }

    board_shutdown();
    return 0;
}

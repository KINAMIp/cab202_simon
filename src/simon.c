#include "simon.h"

void simon_game_init(simon_game_t *game)
{
    game_init(game);
}

void simon_game_handle_event(simon_game_t *game, const board_event_t *event)
{
    game_handle_event(game, event);
}

void simon_game_tick(simon_game_t *game)
{
    game_tick_1ms(game);
}

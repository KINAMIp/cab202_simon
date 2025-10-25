#ifndef SIMON_H
#define SIMON_H

#include "game.h"

void simon_game_init(simon_game_t *game);
void simon_game_handle_event(simon_game_t *game, const board_event_t *event);
void simon_game_tick(simon_game_t *game);

#endif /* SIMON_H */

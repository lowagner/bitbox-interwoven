#ifndef NAME_H
#define NAME_H
#include <stdint.h>
#include "game.h"

void name_start(game_mode_t next_mode, uint8_t *name, int max_length);
void name_line();
void name_controls();

#endif

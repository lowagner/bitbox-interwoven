#include "bitbox.h"
#include "game.h"
#include "sprite.h"

#define BG_COLOR 0

void debugSprite_reset()
{   LL_RESET(sprite, next_to_draw, previous_to_draw, MAX_SPRITES);   
}

void debugSprite_line()
{   memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
}

void debugSprite_controls()
{   if (GAMEPAD_HOLDING(0, select))
    {   game_message[0] = 0;
        if (GAMEPAD_PRESS(0, up))
            game_switch(ModeNameSong);
        return;
    }
    int movement = 0;
    if (GAMEPAD_PRESSING(0, up))
        ++movement;
    if (GAMEPAD_PRESSING(0, down))
        --movement;
    if (GAMEPAD_PRESSING(0, left))
        --movement;
    if (GAMEPAD_PRESSING(0, right))
        ++movement;
}

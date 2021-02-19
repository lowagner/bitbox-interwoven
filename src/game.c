#include "bitbox.h"
#include "chip.h"
#include "font.h"
#include "game.h"

uint16_t new_gamepad[2] CCM_MEMORY;
uint16_t old_gamepad[2] CCM_MEMORY;
uint8_t gamepad_press_wait CCM_MEMORY;

game_mode_t game_mode CCM_MEMORY; 
game_mode_t previous_game_mode CCM_MEMORY;

// The background color will be as a uint16_t equal to (bg_color8 | (bg_color8<<8)).
// This color can be quickly painted using memset.
uint8_t bg_color8 CCM_MEMORY;

uint8_t game_message[32] CCM_MEMORY;
int game_message_timeout CCM_MEMORY;

#define BSOD_COLOR8 140

void game_init()
{   // Logic run once when setting up the Bitbox; Bitbox will call this, don't do it yourself.
    game_mode = ModeNone;
    previous_game_mode = ModeNone;

    chip_init();
    font_init();

    game_switch(ModeMainMenu);
    game_set_message_with_timeout("oh no!!", 0);
}

void game_frame()
{   // Logic to run every frame; Bitbox will call this, don't do it yourself.
    new_gamepad[0] |= gamepad_buttons[0] & (~old_gamepad[0]);
    new_gamepad[1] |= gamepad_buttons[1] & (~old_gamepad[1]);

    switch (game_mode)
    {   // Run frame logic depending on game mode.
        default:
            break;
    }
    
    old_gamepad[0] = gamepad_buttons[0];
    old_gamepad[1] = gamepad_buttons[1];
    
    if (gamepad_press_wait)
        --gamepad_press_wait;
    
    if (game_message_timeout && --game_message_timeout == 0)
        game_message[0] = 0; 
}

void bsod_line();

void graph_line()
{   // Logic to draw for each line on the VGA display; Bitbox will call this, don't do it yourself.

    // TODO: consider doing some non-drawing work on odd frames:
    if (vga_odd) return;

    switch (game_mode)
    {   // Run drawing logic depending on game mode.
        default:
            bsod_line();
    }

    if (game_message[0])
        font_render_line_doubled(game_message, 36, 200, 65535, bg_color8 | (bg_color8<<8));
}

void game_switch(game_mode_t new_game_mode)
{   // Switches to a new game mode.  Does nothing if already in that mode.
    if (new_game_mode == game_mode) return;

    chip_kill();

    previous_game_mode = game_mode;
    game_mode = new_game_mode;

    // TODO: run the game's start if needed
}

void game_switch_to_previous_or(game_mode_t new_game_mode)
{   // Switches to the previous mode (e.g. to jump back to somewhere you were before).
    if (!previous_game_mode) return game_switch(new_game_mode);

    game_switch(previous_game_mode);
}

void game_set_message_with_timeout(const char *msg, int timeout)
{   // Sets a game message where everyone can see it, with a timeout.
    // use timeout=0 for a permanent message (at least until changed).
    strncpy((char *)game_message, msg, 31);
    game_message[31] = 0;
    game_message_timeout = timeout;
}

void bsod_line()
{   // Draw a line for the blue-screen-of-death.
    int line = vga_line/10;
    int internal_line = vga_line%10;
    if (vga_line/2 == 0 || (internal_line/2 == 4))
    {   // Make the beautiful blue background we deserve:
        memset(draw_buffer, BSOD_COLOR8, 2*SCREEN_W);
        return;
    }
    line -= 4;
    if (line < 0 || line >= 16)
        return;
    uint32_t *dst = (uint32_t *)draw_buffer + 37;
    // TODO: just shift
    uint32_t color_choice[2] = { (BSOD_COLOR8*257)|((BSOD_COLOR8*257)<<16), 65535|(65535<<16) };
    int shift = ((internal_line/2))*4;
    for (int c=0; c<16; ++c)
    {   // Draw all the letters in the font for fun
        uint8_t row = (font[c+line*16] >> shift) & 15;
        for (int j=0; j<4; ++j)
        {   // Draw each pixel in the letter
            *(++dst) = color_choice[row&1];
            row >>= 1;
        }
        *(++dst) = color_choice[0];
    }
}

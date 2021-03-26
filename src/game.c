#include "bitbox.h"
#include "chip.h"
#include "edit-instrument.h"
#include "edit-song.h"
#include "edit-track.h"
#include "font.h"
#include "game.h"
#include "io.h"
#include "name.h"

uint16_t new_gamepad[2] CCM_MEMORY;
uint16_t old_gamepad[2] CCM_MEMORY;
uint8_t gamepad_press_wait CCM_MEMORY;

game_mode_t game_mode CCM_MEMORY; 
game_mode_t previous_game_mode CCM_MEMORY;

// Each sprite has access to 16 colors of the global palette based on a uint8_t offset,
// E.g. palette_offset = 0 means that the sprite uses palette colors from 0 to 15,
//      palette_offset = 230 means the sprite uses colors from 230 to 245.
// The index into the palette should not be a u8, however, since a palette_offset of 255
// should allow you to get all the way to palette index 270.
uint16_t game_palette[255 + 15 + 1] CCM_MEMORY;
uint8_t game_message[32] CCM_MEMORY;
int game_message_timeout CCM_MEMORY;

void game_init()
{   // Logic run once when setting up the Bitbox; Bitbox will call this, don't do it yourself.
    game_mode = ModeNone;
    previous_game_mode = ModeNone;

    chip_init();
    font_init();
    editSong_init();
    editTrack_init();
    editInstrument_init();
    io_error_t error = io_init();
    if (error)
        io_message_from_error(game_message, error, IoEventInit);
    else
    {   error = io_load_recent_song_filename();
        if (error)
            message("couldn't load recent song filename\n");
    }

    game_switch(ModeNameSong);
}

void game_frame()
{   // Logic to run every frame; Bitbox will call this, don't do it yourself.
    new_gamepad[0] |= gamepad_buttons[0] & (~old_gamepad[0]);
    new_gamepad[1] |= gamepad_buttons[1] & (~old_gamepad[1]);

    switch (game_mode)
    {   // Run frame logic depending on game mode.
        case ModeNameSong:
            name_controls();
            break;
        case ModeEditSong:
            editSong_controls();
            break;
        case ModeEditTrack:
            editTrack_controls();
            break;
        case ModeEditInstrument:
            editInstrument_controls();
            break;
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

static void bsod_line();

void graph_line()
{   // Logic to draw for each line on the VGA display; Bitbox will call this, don't do it yourself.

    // TODO: consider doing some non-drawing work on odd frames:
    if (vga_odd)
        return;

    if (game_message[0])
    {   // Drawing the game message takes priority, nothing else can show up where that message goes:
        unsigned int delta_y = vga_line - 220;
        if (delta_y < 10)
        {   memset(draw_buffer, 0, 2*SCREEN_W);
            --delta_y;
            if (delta_y < 8)
            {   font_render_line_doubled(game_message, 36, (int)delta_y, 65535, 0);
            }
            // Don't allow any other drawing in this region...
            return;
        }
    }

    switch (game_mode)
    {   // Run drawing logic depending on game mode.
        case ModeNameSong:
            name_line();
            break;
        case ModeEditSong:
            editSong_line();
            break;
        case ModeEditTrack:
            editTrack_line();
            break;
        case ModeEditInstrument:
            editInstrument_line();
            break;
        default:
            bsod_line();
            break;
    }
}

void game_switch(game_mode_t new_game_mode)
{   // Switches to a new game mode.  Does nothing if already in that mode.
    if (new_game_mode == game_mode)
        return;

    chip_kill();

    previous_game_mode = game_mode;
    game_mode = new_game_mode;

    switch (game_mode)
    {   // Run start-up logic depending on game mode.
        case ModeNameSong:
            name_start(
                ModeEditSong,
                base_song_filename,
                sizeof(base_song_filename)
            );
            break;
        case ModeEditSong:
            editSong_start(previous_game_mode == ModeNameSong);
            break;
        default:
            break;
    }
}

void game_switch_to_previous_or(game_mode_t new_game_mode)
{   // Switches to the previous mode (e.g. to jump back to somewhere you were before).
    if (previous_game_mode)
        return game_switch(previous_game_mode);

    game_switch(new_game_mode);
}

void game_set_message_with_timeout(const char *msg, int timeout)
{   // Sets a game message where everyone can see it, with a timeout.
    // use timeout=0 for a permanent message (at least until changed).
    // You may pass in a msg of NULL if you've already manipulated game_message directly.
    if (msg)
        strncpy((char *)game_message, msg, 31);

    game_message[31] = 0;
    game_message_timeout = timeout;
    message("in-game message: %s (expiring %d)\n", game_message, timeout);
}

#define BSOD_COLOR8 140

static void bsod_line()
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

#include "bitbox.h"
#include "chip.h"
#include "edit-instrument.h"
#include "edit-song.h"
#include "edit-track.h"
#include "font.h"
#include "game.h"
#include "io.h"
#include "name.h"

#include <stdlib.h> // rand

#define BG_COLOR 128
#define BOX_COLOR (RGB(180, 250, 180)|(RGB(180, 250, 180)<<16))
#define MATRIX_WING_COLOR (RGB(30, 90, 30) | (RGB(30, 90, 30)<<16))
#define NUMBER_LINES 20

uint8_t music_editor_in_menu CCM_MEMORY;

const uint8_t hex_character[64] = { 
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', // standard hex
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', // up to 32
    'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', // up to 48
    'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 138, 255, // up to 64
};

const uint8_t note_name[12][2] = {
    { 'C', ' ' }, 
    { 'C', '#' }, 
    { 'D', ' ' }, 
    { 'E', 'b' }, 
    { 'E', ' ' }, 
    { 'F', ' ' }, 
    { 'F', '#' }, 
    { 'G', ' ' }, 
    { 'A', 'b' }, 
    { 'A', ' ' }, 
    { 'B', 'b' }, 
    { 'B', ' ' },
};

uint8_t editSong_pos;
uint8_t editSong_offset;
uint8_t editSong_color[2];
uint8_t editSong_last_painted;

static inline void editSong_save_or_load_all(io_event_t save_or_load);

void editSong_init()
{
    song_speed = 4;
    chip_track_length = 32;
    editSong_color[1] = 0;
    editSong_color[0] = 1;
}

void editSong_start(int load_song)
{   // Set the palette to something reasonable.
    // TODO: maybe have this logic in a palette.c...
    static const uint16_t colors[16] = {
        RGB(0, 0, 0),
        RGB(157, 157, 157),
        (1<<15) - 1,
        RGB(224, 111, 139),
        RGB(190, 38, 51),
        RGB(235, 137, 49),
        RGB(164, 100, 34),
        RGB(73, 60, 43),
        RGB(247, 226, 107),
        RGB(163, 206, 39),
        RGB(68, 137, 26),
        RGB(47, 72, 78),
        RGB(178, 220, 239),
        RGB(49, 162, 242),
        RGB(0, 87, 132),
        RGB(28, 20, 40),
    };
    memcpy(game_palette, colors, sizeof(colors));
    if (load_song)
    {   editSong_save_or_load_all(IoEventLoad);
    }
}

void editSong_load_default()
{
    song_length = 16;
    song_speed = 4;
    chip_track_length = 32;
}

void editSong_render(uint8_t value, int x, int y)
{   // Renders value at the location (x, y) where y is an internal coordinate from 0 to 8.
    // TODO: add/use font_render_character(hex_character[value], x, y, color_choice);
    value &= 15;
    uint32_t *dst = (uint32_t *)draw_buffer + x/2;
    uint8_t row = (font[hex_character[value]] >> (((y/2)*4))) & 15;
    uint32_t color_choice[2];
    color_choice[0] = game_palette[value] | (game_palette[value]<<16);
    color_choice[1] = ~color_choice[0];
    *(++dst) = color_choice[0];
    *(++dst) = color_choice[0];
    for (int k=0; k<4; ++k)
    {
        *(++dst) = color_choice[row&1];
        row >>= 1;
    }
    *(++dst) = color_choice[0];
    *(++dst) = color_choice[0];
}

void editSong_line()
{
    if (vga_line < 16)
    {
        if (vga_line/2 == 0)
        {
            memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
            return;
        }
        return;
    }
    else if (vga_line >= 16 + NUMBER_LINES*10)
    {
        if (vga_line/2 == (16 +NUMBER_LINES*10)/2)
            memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
        return;
    }
    int line = (vga_line-16) / 10;
    int internal_line = (vga_line-16) % 10;
    if (internal_line == 0 || internal_line == 9)
    {
        memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
        if (music_editor_in_menu)
        {
            if (line == 0) 
            {
                uint32_t *dst = (uint32_t *)draw_buffer + 48;
                const uint32_t color = BOX_COLOR;
                *(++dst) = color;
                *(++dst) = color;
                *(++dst) = color;
                *(++dst) = color;
                *(++dst) = color;
                *(++dst) = color;
                *(++dst) = color;
                *(++dst) = color;
            }
        }
        else if (line > 2 && line < 7)
        {
            if (line - 3 == editTrack_player)
            {
                uint32_t *dst = (uint32_t *)draw_buffer + 21 + 8*editSong_pos - 8*editSong_offset;
                (*dst) = BOX_COLOR;
                (*(++dst)) = BOX_COLOR;
                (*(++dst)) = BOX_COLOR;
                (*(++dst)) = BOX_COLOR;
                (*(++dst)) = BOX_COLOR;
                (*(++dst)) = BOX_COLOR;
                (*(++dst)) = BOX_COLOR;
                (*(++dst)) = BOX_COLOR;
            }
            else if (line == 6 && internal_line == 9)
            {
                uint32_t *dst = (uint32_t *)draw_buffer + 21 + 8*editSong_pos - 8*editSong_offset;
                (*dst) = MATRIX_WING_COLOR;
                (*(++dst)) = MATRIX_WING_COLOR;
                (*(++dst)) = MATRIX_WING_COLOR;
                (*(++dst)) = MATRIX_WING_COLOR;
                (*(++dst)) = MATRIX_WING_COLOR;
                (*(++dst)) = MATRIX_WING_COLOR;
                (*(++dst)) = MATRIX_WING_COLOR;
                (*(++dst)) = MATRIX_WING_COLOR;
            }
        }
        return;
    }
    --internal_line;
    uint8_t buffer[24];
    switch (line)
    {
        case 0:
        {
            // edit track
            uint8_t msg[] = { 's', 'o', 'n', 'g',
                ' ', 'X', '0' + editSong_pos/10, '0' + editSong_pos%10, '/',
                '0' + song_length/10, '0' + song_length%10,
                ' ', 's', 'p', 'e', 'e', 'd', ' ', '0'+(16-song_speed)/10, '0'+(16-song_speed)%10,
                ' ', 't', 'k', 'l', 'e', 'n',' ', '0'+chip_track_length/10, '0'+chip_track_length%10,
            0 };
            font_render_line_doubled(msg, 16, internal_line, 65535, BG_COLOR*257);
            break;
        }
        case 2:
            if (chip_playing)
            {
                font_render_line_doubled((const uint8_t *)"P:", 20, internal_line, 65535, BG_COLOR*257);
                uint8_t song_current = song_pos ? song_pos-1 : song_length-1;
                if ((track_pos/4%2 == 0) && song_current >= editSong_offset && song_current < editSong_offset+16)
                    font_render_line_doubled((const uint8_t *)"*", 28+16+16*song_current - 16*editSong_offset, internal_line, BOX_COLOR&65535, BG_COLOR*257);
            }
            else
                font_render_line_doubled((const uint8_t *)"P:tracks", 20, internal_line, 65535, BG_COLOR*257);
            
            break;
        case 3:
        case 4:
        case 5:
        case 6:
        {
            int i = line - 3;
            if (i == editTrack_player)
            {
                uint32_t *dst = (uint32_t *)draw_buffer + 6;
                if ((internal_line+1)/2 == 1)
                {
                    *dst = ~(*dst);
                    ++dst;
                    *dst = ~(*dst);
                }
                else if ((internal_line+1)/2 == 3)
                {
                    *dst = 16843009u*BG_COLOR;
                    ++dst;
                    *dst = 16843009u*BG_COLOR;
                }
            }
            {
                uint8_t msg[] = { hex_character[i], ':', 0 };
                font_render_line_doubled(msg, 20, internal_line, 65535, BG_COLOR*257);
                uint32_t *dst = (uint32_t *)draw_buffer + 20;
                uint8_t y = ((internal_line/2))*4; // how much to shift for font row
                uint16_t *value = &chip_song[editSong_offset]-1;
                for (int j=0; j<8; ++j)
                {
                    uint8_t command = ((*(++value))>>(4*i))&15;
                    uint8_t row = (font[hex_character[command]] >> y) & 15;
                    uint32_t color_choice[2];
                    color_choice[0] = game_palette[command] | (game_palette[command]<<16);
                    color_choice[1] = ~color_choice[0];

                    *(++dst) = color_choice[0];
                    *(++dst) = color_choice[0];
                    for (int k=0; k<4; ++k)
                    {
                        *(++dst) = color_choice[row&1];
                        row >>= 1;
                    }
                    *(++dst) = color_choice[0];
                    *(++dst) = color_choice[0];

                    command = ((*(++value))>>(4*i))&15;
                    row = (font[hex_character[command]] >> y) & 15;
                    color_choice[0] = game_palette[command] | (game_palette[command]<<16);
                    color_choice[1] = ~color_choice[0];
                    *(++dst) = color_choice[0];
                    *(++dst) = color_choice[0];
                    for (int k=0; k<4; ++k)
                    {
                        *(++dst) = color_choice[row&1];
                        row >>= 1;
                    }
                    *(++dst) = color_choice[0];
                    *(++dst) = color_choice[0];
                }
            }
            if (music_editor_in_menu == 0 && i == editTrack_player)
            {
                const uint16_t color = (editSong_offset + 15 == editSong_pos) ? 
                    BOX_COLOR :
                    MATRIX_WING_COLOR;
                uint16_t *dst = draw_buffer + 20*2 + 16*8*2;
                *(++dst) = color;
                ++dst;
                *(++dst) = color;
                ++dst;
                *(++dst) = color;
                ++dst;
                *(++dst) = color;
                ++dst;
                *(++dst) = color;
            }

            break;
        }
        case 8:
            if (music_editor_in_menu)
                font_render_line_doubled((uint8_t *)"dpad:adjust song length", 16, internal_line, 65535, BG_COLOR*257);
            else
                font_render_line_doubled((uint8_t *)"dpad:move cursor", 16, internal_line, 65535, BG_COLOR*257);
            break;
        case 9:
            if (music_editor_in_menu)
                font_render_line_doubled((uint8_t *)"A:save song", 16+3*9, internal_line, 65535, BG_COLOR*257);
            else if (chip_playing)
                font_render_line_doubled((uint8_t *)"A:stop song", 16+3*9, internal_line, 65535, BG_COLOR*257);
            else
                font_render_line_doubled((uint8_t *)"A:play song", 16+3*9, internal_line, 65535, BG_COLOR*257);
            break;
        case 10:
            if (music_editor_in_menu)
                font_render_line_doubled((uint8_t *)"B:load song", 16+3*9, internal_line, 65535, BG_COLOR*257);
            else
                font_render_line_doubled((uint8_t *)"X:edit track under cursor", 16+3*9, internal_line, 65535, BG_COLOR*257);
            break;
        case 11:
            if (music_editor_in_menu)
                font_render_line_doubled((uint8_t *)"X:load song only", 16+3*9, internal_line, 65535, BG_COLOR*257);
            else
            {   // Show what the current color is, and how to change it (with L/R):
                font_render_line_doubled((uint8_t *)"B:put", 16+3*9, internal_line, 65535, BG_COLOR*257);
                editSong_render(editSong_color[1], 16+3*9+6*9, internal_line);
                if (editSong_last_painted)
                {
                    font_render_line_doubled((uint8_t *)"L:", 150, internal_line, 65535, BG_COLOR*257);
                    editSong_render(editSong_color[1]-1, 150+2*9, internal_line);
                    font_render_line_doubled((uint8_t *)"R:", 200, internal_line, 65535, BG_COLOR*257);
                    editSong_render(editSong_color[1]+1, 200+2*9, internal_line);
                }
            }
            break;
        case 12:
            if (music_editor_in_menu)
            {
                strcpy((char *)buffer, "Y:file ");
                strcpy((char *)(buffer+7), (char *)base_song_filename);
                font_render_line_doubled(buffer, 16+3*9, internal_line, 65535, BG_COLOR*257);
            }
            else
            {
                font_render_line_doubled((uint8_t *)"Y:put", 16+3*9, internal_line, 65535, BG_COLOR*257);
                editSong_render(editSong_color[0], 16+3*9+6*9, internal_line);
                if (!editSong_last_painted)
                {
                    font_render_line_doubled((uint8_t *)"L:", 150, internal_line, 65535, BG_COLOR*257);
                    editSong_render(editSong_color[0]-1, 150+2*9, internal_line);
                    font_render_line_doubled((uint8_t *)"R:", 200, internal_line, 65535, BG_COLOR*257);
                    editSong_render(editSong_color[0]+1, 200+2*9, internal_line);
                }
            }
            break;
        case 13:
            if (music_editor_in_menu)
                font_render_line_doubled((uint8_t *)"L/R:adjust global volume", 16+1*9, internal_line, 65535, BG_COLOR*257);
            break;
        case 15:
            if (music_editor_in_menu)
                font_render_line_doubled((uint8_t *)"start:edit song", 16, internal_line, 65535, BG_COLOR*257);
            else
                font_render_line_doubled((uint8_t *)"start:song menu", 16, internal_line, 65535, BG_COLOR*257);
            break;        
        case 16:
            if (music_editor_in_menu)
                font_render_line_doubled((uint8_t *)"select:track menu", 16, internal_line, 65535, BG_COLOR*257);
            else
                font_render_line_doubled((uint8_t *)"select:edit track", 16, internal_line, 65535, BG_COLOR*257);
            break;        
        case 18:
            font_render_line_doubled(game_message, 16, internal_line, 65535, BG_COLOR*257);
            break;        
    }
}

uint8_t editSong_track_under_cursor()
{
    return (chip_song[editSong_pos] >> (editTrack_player*4))&15;
}

void editSong_paint(uint8_t p)
{
    editSong_last_painted = p;

    uint16_t *memory = &chip_song[editSong_pos];
    *memory &= ~(15 << (editTrack_player*4)); // clear out current value there
    *memory |= (editSong_color[p] << (editTrack_player*4)); // add this value
}

void editSong_controls()
{
    if (music_editor_in_menu)
    {
        int movement = 0;
        if (GAMEPAD_PRESSING(0, up))
            ++movement;
        if (GAMEPAD_PRESSING(0, down))
            --movement;
        if (GAMEPAD_PRESSING(0, left))
            --movement;
        if (GAMEPAD_PRESSING(0, right))
            ++movement;
        if (movement)
        {
            game_message[0] = 0;
            song_length += movement;
            if (song_length < 16)
                song_length = 16;
            else if (song_length > MAX_SONG_LENGTH)
                song_length = MAX_SONG_LENGTH;
            gamepad_press_wait = GAMEPAD_PRESS_WAIT;
            return;
        }
        
        io_event_t save_or_load = IoEventNone;
        if (GAMEPAD_PRESS(0, A))
            save_or_load = IoEventSave;
        if (GAMEPAD_PRESS(0, B))
            save_or_load = IoEventLoad;
        if (save_or_load)
        {   editSong_save_or_load_all(save_or_load);
            return;
        }

        if (GAMEPAD_PRESSING(0, L))
        {   if (chip_volume > 4)
                chip_volume -= 4;
            else if (chip_volume)
                --chip_volume;
            --movement;
        }
        if (GAMEPAD_PRESSING(0, R))
        {   if (chip_volume < 252)
                chip_volume += 4;
            else if (chip_volume < 255)
                ++chip_volume;
            ++movement;
        }
        if (movement) 
        {   strcpy((char *)game_message, "global volume to ");
            game_message[17] = hex_character[chip_volume/16];
            game_message[18] = hex_character[chip_volume%16];
            game_message[19] = 0;
            gamepad_press_wait = GAMEPAD_PRESS_WAIT;
            game_message_timeout = MESSAGE_TIMEOUT;
            return;
        }

        if (GAMEPAD_PRESS(0, X))
        {   // load just editSong
            io_message_from_error(game_message, io_load_song(), 2);
            return;
        }

        if (GAMEPAD_PRESS(0, Y))
        {   // switch to choose name and hope to come back
            game_message[0] = 0;
            game_switch(ModeNameSong);
            return;
        }
    }
    else // editing, not menu
    {
        int paint_if_moved = 0; 
        if (GAMEPAD_PRESSING(0, Y))
        {
            game_message[0] = 0;
            editSong_paint(0);
            paint_if_moved = 1;
        }
        if (GAMEPAD_PRESSING(0, B))
        {
            game_message[0] = 0;
            editSong_paint(1);
            paint_if_moved = 2;
        }

        int switched = 0;
        if (GAMEPAD_PRESSING(0, L))
            --switched;
        if (GAMEPAD_PRESSING(0, R))
            ++switched;
        if (switched)
        {
            editSong_color[editSong_last_painted] = (editSong_color[editSong_last_painted]+switched)&15;
            game_message[0] = 0;
        }
        
        int moved = 0;
        if (GAMEPAD_PRESSING(0, down))
        {
            editTrack_player = (editTrack_player+1)&3;
            moved = 1;
        }
        if (GAMEPAD_PRESSING(0, up))
        {
            editTrack_player = (editTrack_player-1)&3;
            moved = 1;
        }
        if (GAMEPAD_PRESSING(0, left))
        {
            if (--editSong_pos >= song_length)
            {
                editSong_pos = song_length - 1;
                editSong_offset = song_length - 16;
            }
            else if (editSong_pos < editSong_offset)
                editSong_offset = editSong_pos;
            moved = 1;
        }
        if (GAMEPAD_PRESSING(0, right))
        {
            if (++editSong_pos >= song_length)
            {
                editSong_pos = 0;
                editSong_offset = 0;
            }
            else if (editSong_pos > editSong_offset+15)
                editSong_offset = editSong_pos - 15;
            moved = 1;
        }
        if (moved)
        {
            gamepad_press_wait = GAMEPAD_PRESS_WAIT;
            if (paint_if_moved)
                editSong_paint(paint_if_moved-1);
        }
        else if (switched || paint_if_moved)
            gamepad_press_wait = GAMEPAD_PRESS_WAIT;

        if (GAMEPAD_PRESS(0, A))
        {   // Toggle playing the song on/off
            game_message[0] = 0;
            if (chip_playing)
                chip_kill();
            else
                chip_play_song(editSong_pos);
        } 
        
        if (GAMEPAD_PRESS(0, X))
        {
            editTrack_track = editSong_track_under_cursor();
            game_switch(ModeEditTrack);
            return;
        }
    }
    
    if (GAMEPAD_PRESS(0, start))
    {
        game_message[0] = 0;
        music_editor_in_menu = 1 - music_editor_in_menu; 
        chip_kill();
        chip_playing = PlayingNone;
        return;
    }

    if (GAMEPAD_PRESS(0, select))
    {   // Switch to editing tracks.
        game_message[0] = 0;
        game_switch(ModeEditTrack);
    } 
}

static inline void editSong_save_or_load_all(io_event_t save_or_load)
{   io_error_t error;
    if (save_or_load == IoEventSave)
    {   // Split up saving into different pieces;
        // The bitbox struggles to save all at once.
        error = io_save_song();
        if (error)
        {   strcpy((char *)game_message, "song ");
            io_message_from_error(game_message+5, error, IoEventSave);
            return;
        }
        error = io_save_track(MAX_TRACKS);

        if (error)
        {   strcpy((char *)game_message, "track ");
            io_message_from_error(game_message+6, error, IoEventSave);
            return;
        }

        error = io_save_instrument(16);
        if (error)
        {   strcpy((char *)game_message, "instr. ");
            io_message_from_error(game_message+7, error, IoEventSave);
        }
        else
            io_message_from_error(game_message, IoNoError, IoEventSave);
    }
    else if (save_or_load == IoEventLoad)
    {   // Load all pieces separately to avoid taxing bitbox too much.
        error = io_load_song();
        if (error)
        {   strcpy((char *)game_message, "song ");
            io_message_from_error(game_message+5, error, IoEventLoad);
            return;
        }

        error = io_load_track(MAX_TRACKS);
        if (error)
        {   strcpy((char *)game_message, "track ");
            io_message_from_error(game_message+6, error, IoEventLoad);
            return;
        }

        error = io_load_instrument(16);
        if (error)
        {   strcpy((char *)game_message, "instr. ");
            io_message_from_error(game_message+7, error, IoEventLoad);
        }
        else
            io_message_from_error(game_message, IoNoError, IoEventLoad);
    }
    else
        message("invalid save_or_load for editSong_save_or_load_all()\n");

    game_set_message_with_timeout(NULL, 1000);
}

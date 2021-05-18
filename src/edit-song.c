#include "bitbox.h"
#include "chip.h"
#include "edit-instrument.h"
#include "edit-song.h"
#include "edit-track.h"
#include "font.h"
#include "game.h"
#include "io.h"
#include "name.h"
#include "sprite.h"

#include <stdlib.h> // rand

#define BG_COLOR 128
// TODO: make unique from edit-track.c:
#define PLAY_COLOR (RGB(200, 100, 0)|(RGB(200, 100, 0)<<16))
#define BOX_COLOR (RGB(180, 250, 180)|(RGB(180, 250, 180)<<16))
#define MATRIX_WING_COLOR (RGB(30, 90, 30) | (RGB(30, 90, 30)<<16))
#define NUMBER_LINES 20

uint8_t music_editor_in_menu CCM_MEMORY;

const uint8_t hex_character[64] =
{   '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', // standard hex
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', // up to 32
    'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', // up to 48
    'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 138, 255, // up to 64
};

const uint8_t note_name[12][2] =
{   { 'C', ' ' }, 
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
uint8_t editSong_bad;
uint8_t editSong_offset;
uint8_t editSong_command_copy;
uint8_t editSong_command_appears_reachable;

static void editSong_save_or_load_all(io_event_t save_or_load);

void editSong_start(int load_song)
{   if (load_song)
    {   editSong_save_or_load_all(IoEventLoad);
    }
    else
    {   chip_song_cmd[0] = SongPlayTracksForCount;
        chip_song_cmd[1] = SongBreak;
    }
    editSong_bad = 0;
    editSong_pos = 0;
    editSong_offset = 0;
    editSong_command_copy = 0;
}

void editSong_render_command(int j, int y)
{   int x = 20;
    uint8_t next_command_will_be_reachable = editSong_command_appears_reachable;
    #ifdef EMULATOR
    if (y < 0 || y >= 8)
    {
        message("got too big a line count for editSong, line %d\n", j);
        return;
    }
    #endif
   
    ASSERT(j >= 0 && j < 256);
    uint8_t cmd = chip_song_cmd[j];
    uint8_t param = cmd>>4;
    cmd &= 15;

    ASSERT((x/2)%2 == 0);
    uint32_t *dst = (uint32_t *)draw_buffer + x/2;
    uint32_t color_choice[2];
    if (j%2)
        color_choice[0] = 16843009u*BG_COLOR;
    else
        color_choice[0] = 16843009u*149;
    
    if (j != editSong_pos)
        color_choice[1] = 65535u*65537u;
    else
    {   color_choice[1] = RGB(190, 245, 255)|(RGB(190, 245, 255)<<16);
        if (!music_editor_in_menu)
        {   // draw a little dot indicating your edit cursor is at this command
            if ((y+1)/2 == 1)
            {   dst -= 4;
                *dst = color_choice[1];
                ++dst;
                *dst = color_choice[1];
                dst += 4 - 1;
            }
            else if ((y+1)/2 == 3)
            {   dst -= 4;
                *dst = 16843009u*BG_COLOR;
                ++dst;
                *dst = 16843009u*BG_COLOR;
                dst += 4 - 1;
            }
        }
    }
    
    switch (cmd)
    {   case SongBreak:
            cmd = 'Q';
            param = hex_character[param];
            if (y == 7)
                next_command_will_be_reachable = 0;
            break;
        case SongVolume:
            cmd = 'V';
            param = hex_character[param];
            break;
        case SongFadeInOrOut:
            if (param < 8)
            {   cmd = '<';
                param = hex_character[param];
            }
            else
            {   cmd = '>';
                param = hex_character[16 - param];
            }
            break;
        case SongChoosePlayers:
            cmd = 'P';
            param = hex_character[param];
            break;
        case SongSetLowTrackForChosenPlayers:
            cmd = 't';
            param = hex_character[param];
            break;
        case SongSetHighTrackForChosenPlayers:
            cmd = 'T';
            param = hex_character[param];
            break;
        case SongRepeatLowTrackForChosenPlayers:
            cmd = 'l';
            param = hex_character[param];
            break;
        case SongRepeatHighTrackForChosenPlayers:
            cmd = 'L';
            param = hex_character[param];
            break;
        case SongPlayTracksForCount:
            cmd = 'W';
            if (param)
                param = hex_character[param];
            else
                param = 'g';
            break;
        case SongSpeed:
            cmd = 'S';
            if (param)
                param = hex_character[param];
            else
                param = 'g';
            break;
        case SongTranspose:
            cmd = 'T';
            param = hex_character[param];
            break;
        case SongSquarify:
            cmd = '_';
            param = hex_character[param];
            break;
        case SongSetVariableA:
            cmd = 'a';
            param = hex_character[param];
            break;
        case SongSpecial:
            switch (param)
            {   case SongIfAEqualsZeroExecuteNextOtherwiseFollowingCommand:
                    cmd = 13; // double exclamation
                    param = 'a';
                    break;
                case SongIfAGreaterThanZeroExecuteNextOtherwiseFollowingCommand:
                    cmd = '!';
                    param = 'a';
                    break;
                case SongIfALessThanBExecuteNextOtherwiseFollowingCommand:
                    cmd = 'a';
                    param = '<';
                    break;
                case SongIfAEqualsBExecuteNextOtherwiseFollowingCommand:
                    cmd = 'a';
                    param = '=';
                    break;
                case SongSetNextCommandParameterToA:
                    cmd = 'a';
                    param = 'N';
                    break;
                case SongSetBEqualToA:
                    cmd = 'b';
                    param = '=';
                    break;
                case SongSwapAAndB:
                    cmd = 'b';
                    param = 'a';
                    break;
                case SongModAByB:
                    cmd = 'a';
                    param = '%';
                    break;
                case SongDivideAByB:
                    cmd = 'a';
                    param = '/';
                    break;
                case SongAddBToA:
                    cmd = 'w';
                    param = '+';
                    break;
                case SongSubtractBFromA:
                    cmd = 'w';
                    param = '-';
                    break;
                case SongHalveA:      
                    cmd = 'a';
                    param = 'H';
                    break;
                case SongIncrementAWithWraparound:
                    cmd = '+';
                    param = '+';
                    break;
                case SongDecrementAWithWraparound:
                    cmd = '-';
                    param = '-';
                    break;
                case SongIncrementANoWrap:
                    cmd = '^';
                    param = 'a';
                    break;
                case SongDecrementANoWrap:
                    cmd = 127; // down arrow
                    param = 'a';
                    break;
            }
            break;
        case SongRandomize:
            cmd = 'R'; 
            param = 224 + param;
            break;
        case SongJump:
            cmd = 'J';
            param = hex_character[param];
            break;
    }
    if (!editSong_command_appears_reachable)
    {   // change the command's colors so it doesn't look easily reachable
        // TODO: do this for instrument and verse 
        uint32_t swapper = ~color_choice[0];
        color_choice[0] = ~color_choice[1];
        color_choice[1] = swapper;
    }
    editSong_command_appears_reachable = next_command_will_be_reachable;
    
    uint8_t shift = ((y/2))*4;
    uint8_t row = (font[hex_character[j/16]] >> shift) & 15;
    *(++dst) = color_choice[0];
    for (int k=0; k<4; ++k)
    {   *(++dst) = color_choice[row&1];
        row >>= 1;
    }
    *(++dst) = color_choice[0];
    row = (font[hex_character[j%16]] >> shift) & 15;
    *(++dst) = color_choice[0];
    for (int k=0; k<4; ++k)
    {   *(++dst) = color_choice[row&1];
        row >>= 1;
    }
    *(++dst) = color_choice[0];
    row = (font[':'] >> shift) & 15;
    for (int k=0; k<4; ++k)
    {   *(++dst) = color_choice[row&1];
        row >>= 1;
    }
    *(++dst) = color_choice[0];
    *(++dst) = color_choice[0];
    row = (font[cmd] >> shift) & 15;
    for (int k=0; k<4; ++k)
    {   *(++dst) = color_choice[row&1];
        row >>= 1;
    }
    *(++dst) = color_choice[0];
    
    row = (font[param] >> shift) & 15;
    for (int k=0; k<4; ++k)
    {   *(++dst) = color_choice[row&1];
        row >>= 1;
    }
    *(++dst) = color_choice[0];
  
    if (chip_playing != PlayingSong)
        return;
    int cmd_index = chip_song_cmd_index;
    if (cmd_index && ((chip_song_cmd[cmd_index-1]&15) == SongPlayTracksForCount))
    {   // the song advances its cmd index even though it's currently waiting based on this command;
        // put the song position indicator on this previous wait command.
        --cmd_index;
    }
    if (j == cmd_index)
    {
        if ((y+1)/2 == 1)
        {
            dst += 4;
            *dst = PLAY_COLOR;
            ++dst;
            *dst = PLAY_COLOR;
        }
        else if ((y+1)/2 == 3)
        {
            dst += 4;
            *dst = 16843009u*BG_COLOR;
            ++dst;
            *dst = 16843009u*BG_COLOR;
        }
    }
}

int _editSong_check();

void editSong_check()
{   // check if that parameter broke something
    if (_editSong_check())
    {   editSong_bad = 1; 
        game_set_message_with_timeout("bad jump, need wait in loop.", MESSAGE_TIMEOUT);
    }
    else
    {   editSong_bad = 0; 
        game_message[0] = 0;
    }
}

int _editSong_check()
{   // check for a JUMP which loops back on itself without waiting at least a little bit.
    // return 1 if so, 0 if not.
    int j=0; // current command index
    int j_last_jump = -1;
    int found_wait = 0;
    for (int k=0; k<257; ++k)
    {   if (j >= 256) // got to the end
            return !(found_wait);
        uint8_t command = chip_song_cmd[j];
        if (j_last_jump >= 0)
        {   if (j == j_last_jump) // we found our loop-back point
                return !(found_wait); // did we find a wait?
            int j_next_jump = -1;
            switch (command & 15)
            {   case SongJump:
                    j_next_jump = 16*(command >> 4);
                    if (j_next_jump == j_last_jump) // jumping forward to the original jump
                    {   message("jumped to the old jump\n");
                        return !(found_wait);
                    }
                    else if (j_next_jump > j_last_jump) // jumping past original jump
                    {   message("This probably shouldn't happen.\n");
                        j_last_jump = -1; // can't look for loops...
                    }
                    else
                    {   message("jumped backwards again?\n");
                        j_last_jump = j;
                        found_wait = 0;
                    }
                    j = j_next_jump;
                    break;
                case SongPlayTracksForCount:
                    message("saw wait at j=%d\n", j);
                    found_wait = 1;
                    ++j;
                    break;
                case SongBreak:
                    // check for a randomizer behind
                    if (j > 0 && (chip_song_cmd[j-1]&15) == SongRandomize)
                    {}
                    else if (command>>4)
                        return 0;
                    j = 0;
                    break;
                default:
                    ++j;
            }
        }
        else
        switch (command & 15)
        {   case SongJump:
                j_last_jump = j;
                j = 16*(command >> 4);
                if (j > j_last_jump)
                {   message("This probably shouldn't happen??\n");
                    j_last_jump = -1; // don't care, we moved ahead
                }
                else if (j == j_last_jump)
                {   message("jumped to itself\n");
                    return 1; // this is bad
                }
                else
                    found_wait = 0;
                break;
            case SongBreak:
                // check for a randomizer behind
                if (j > 0 && (chip_song_cmd[j-1] & 15) == SongRandomize)
                {}
                else if (command >> 4)
                    return 0;
                j = 0;
                break;
            default:
                ++j;
        }
    }
    message("couldn't finish after 257 iterations. congratulations.\nprobably looping back on self, but with waits.");
    return 0;
}

void editSong_short_command_message(uint8_t *buffer, uint8_t cmd)
{   switch (cmd&15)
    {   case SongBreak:
            strcpy((char *)buffer, "end");
            break;
        case SongVolume:
            strcpy((char *)buffer, "volume");
            break;
        case SongFadeInOrOut:
            strcpy((char *)buffer, "fade");
            break;
        case SongChoosePlayers:
            strcpy((char *)buffer, "choose p");
            break;
        case SongSetLowTrackForChosenPlayers:
            strcpy((char *)buffer, "set low trk");
            break;
        case SongSetHighTrackForChosenPlayers:
            strcpy((char *)buffer, "set hi TRK");
            break;
        case SongRepeatLowTrackForChosenPlayers:
            strcpy((char *)buffer, "loop low trk");
            break;
        case SongRepeatHighTrackForChosenPlayers:
            strcpy((char *)buffer, "loop hi TRK");
            break;
        case SongPlayTracksForCount:
            strcpy((char *)buffer, "wait/play");
            break;
        case SongSpeed:
            strcpy((char *)buffer, "speed");
            break;
        case SongTranspose:
            strcpy((char *)buffer, "transpose");
            break;
        case SongSquarify:
            strcpy((char *)buffer, "squarify");
            break;
        case SongSetVariableA:
            strcpy((char *)buffer, "set var a");
            break;
        case SongSpecial:
            strcpy((char *)buffer, "special");
            break;
        case SongRandomize:
            strcpy((char *)buffer, "randomize");
            break;
        case SongJump:
            strcpy((char *)buffer, "jump");
            break;
    }
}

void editSong_line()
{   if (vga_line < 16)
    {   if (vga_line/2 == 0)
        {   memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
            return;
        }
        return;
    }
    else if (vga_line >= 16 + NUMBER_LINES*10)
    {   if (vga_line/2 == (16 +NUMBER_LINES*10)/2)
            memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
        return;
    }
    int line = (vga_line-16) / 10;
    int internal_line = (vga_line-16) % 10;
    if (internal_line == 0 || internal_line == 9)
    {   memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
        return;
    }
    --internal_line;
    uint8_t buffer[32];
    switch (line)
    {   case 0:
        {   uint8_t msg[] =
            {   (chip_playing && (track_pos/4 % 2==0)) ? '*' : ' ',
                's', 'o', 'n', 'g', ' ', 
                ' ', 't', 'r', 'a', 'n', 's', 
                ' ', '0' + song_transpose/10, '0' + song_transpose%10,
                ' ', 's', 'p', 'e', 'e', 'd', 
                ' ', '0' + song_speed/10, '0' + song_speed%10,
                ' ', 't', 'T', 'i', 'm', 'e', 
                ' ', '0' + chip_track_playtime/10, '0' + chip_track_playtime%10,
                0
            };
            font_render_line_doubled(msg, 16, internal_line, 65535, BG_COLOR*257);
            break;
        }
        case 1:
            break;
        case 2:
        {   editSong_command_appears_reachable = 1; 
            editSong_render_command(editSong_offset+line-2, internal_line);
            // command
            uint8_t msg[] =
            {   'c', 'o', 'm', 'm', 'a', 'n', 'd', ' ',
                hex_character[editSong_pos/16], hex_character[editSong_pos%16], ':',
                0
            };
            font_render_line_doubled(msg, 96, internal_line, 65535, BG_COLOR*257);
            break;
        }
        case 3:
        {   const char *msg = "";
            switch (chip_song_cmd[editSong_pos]&15)
            {   case SongBreak:
                    msg = "end song";
                    break;
                case SongVolume:
                    msg = "volume";
                    break;
                case SongFadeInOrOut:
                    msg = "volume fade";
                    break;
                case SongChoosePlayers:
                    msg = "choose players";
                    break;
                case SongSetLowTrackForChosenPlayers:
                    msg = "select low track";
                    break;
                case SongSetHighTrackForChosenPlayers:
                    msg = "select high TRACK";
                    break;
                case SongRepeatLowTrackForChosenPlayers:
                    msg = "loop low track";
                    break;
                case SongRepeatHighTrackForChosenPlayers:
                    msg = "loop high TRACK";
                    break;
                case SongPlayTracksForCount:
                    msg = "play for duration";
                    break;
                case SongSpeed:
                    msg = "song speed";
                    break;
                case SongTranspose:
                    msg = "song transpose";
                    break;
                case SongSquarify:
                    msg = "song squarify";
                    break;
                case SongSetVariableA:
                    msg = "set variable 'a'";
                    break;
                case SongSpecial:
                    msg = "special";
                    break;
                case SongRandomize:
                    msg = "randomize next cmd";
                    break;
                case SongJump:
                    msg = "jump to cmd index";
                    break;
            }
            font_render_line_doubled((const uint8_t *)msg, 102, internal_line, 65535, BG_COLOR*257);
            goto draw_song_command;
        }
        case 4:
        {   uint8_t command = chip_song_cmd[editSong_pos];
            uint8_t param = command >> 4;
            command &= 15;
            switch (command)
            {   case SongChoosePlayers:
                {   if (param == 0)
                        param = 15;
                    uint8_t *msg_writable = buffer;
                    *msg_writable = 'p';
                    *++msg_writable = 'l';
                    *++msg_writable = 'a';
                    *++msg_writable = 'y';
                    *++msg_writable = 'e';
                    *++msg_writable = 'r';
                    *++msg_writable = 's';
                    *++msg_writable = ' ';
                    if (param & 1)
                    {   *++msg_writable = '0';
                        *++msg_writable = ',';
                        *++msg_writable = ' ';
                    }
                    if (param & 2)
                    {   *++msg_writable = '1';
                        *++msg_writable = ',';
                        *++msg_writable = ' ';
                    }
                    if (param & 4)
                    {   *++msg_writable = '2';
                        *++msg_writable = ',';
                        *++msg_writable = ' ';
                    }
                    if (param & 8)
                    {   *++msg_writable = '3';
                        msg_writable += 2;
                    }
                    *--msg_writable = 0;
                    font_render_line_doubled
                    (   buffer, 120, internal_line, 65535, BG_COLOR*257
                    );
                    break;
                }
                case SongSpecial:
                {   const char *msg = "";
                    switch (param)
                    {   case SongIfAEqualsZeroExecuteNextOtherwiseFollowingCommand:
                            msg = "exe next cmd if a==0";
                            break;
                        case SongIfAGreaterThanZeroExecuteNextOtherwiseFollowingCommand:
                            msg = "exe next cmd if a>0";
                            break;
                        case SongIfALessThanBExecuteNextOtherwiseFollowingCommand:
                            msg = "exe next cmd if a<b";
                            break;
                        case SongIfAEqualsBExecuteNextOtherwiseFollowingCommand:
                            msg = "exe next cmd if a==b";
                            break;
                        case SongSetNextCommandParameterToA:
                            msg = "set next param to a";
                            break;
                        case SongSetBEqualToA:
                            msg = "set b equal to a";
                            break;
                        case SongSwapAAndB:
                            msg = "swap b and a";
                            break;
                        case SongModAByB:
                            msg = "set a = a \% b";
                            break;
                        case SongDivideAByB:
                            msg = "set a = a / b";
                            break;
                        case SongAddBToA:
                            msg = "a += b with wrap & 15";
                            break;
                        case SongSubtractBFromA:
                            msg = "a -= b with wrap & 15";
                            break;
                        case SongHalveA:      
                            msg = "set a /= 2";
                            break;
                        case SongIncrementAWithWraparound:
                            msg = "a += 1 with wrap & 15";
                            break;
                        case SongDecrementAWithWraparound:
                            msg = "a -= 1 with wrap & 15";
                            break;
                        case SongIncrementANoWrap:
                            msg = "++a if a < 15";
                            break;
                        case SongDecrementANoWrap:
                            msg = "--a if a > 0";
                            break;
                    }
                    font_render_line_doubled((const uint8_t *)msg, 120, internal_line, 65535, BG_COLOR*257);
                    break;
                }
            }
            goto draw_song_command;
        }
        case 5:
            font_render_line_doubled((uint8_t *)"switch to:", 102 - 6*music_editor_in_menu, internal_line, 65535, BG_COLOR*257); 
            goto draw_song_command;
        case 6:
            if (music_editor_in_menu)
            {   font_render_line_doubled((uint8_t *)"L:prev track", 112, internal_line, 65535, BG_COLOR*257);
            }
            else
            {   buffer[0] = 'L'; buffer[1] = ':';
                editSong_short_command_message(buffer+2, chip_song_cmd[editSong_pos]-1);
                font_render_line_doubled(buffer, 112, internal_line, 65535, BG_COLOR*257);
            }
            goto draw_song_command;
        case 7:
            if (music_editor_in_menu)
            {
                font_render_line_doubled((uint8_t *)"R:next track", 112, internal_line, 65535, BG_COLOR*257);
            }
            else
            {
                buffer[0] = 'R'; buffer[1] = ':';
                editSong_short_command_message(buffer+2, chip_song_cmd[editSong_pos]+1);
                font_render_line_doubled(buffer, 112, internal_line, 65535, BG_COLOR*257);
            }
            goto draw_song_command;
        case 8:
            if (!music_editor_in_menu)
                font_render_line_doubled((uint8_t *)"dpad:", 102 - 6*music_editor_in_menu, internal_line, 65535, BG_COLOR*257);
            goto draw_song_command;
        case 9:
            if (!music_editor_in_menu)
            {   font_render_line_doubled
                (   (const uint8_t *)"adjust parameters", 112, internal_line, 65535, BG_COLOR*257
                );
            }
            goto draw_song_command;
        case 11:
        {   const char *msg = "";
            if (music_editor_in_menu)
            {   msg = "A:save to file";
            }
            else if (chip_playing)
            {   msg = "A:stop song";
            }
            else
            {   msg = "A:play song from start";
            }
            font_render_line_doubled((const uint8_t *)msg, 96, internal_line, 65535, BG_COLOR*257);
            goto draw_song_command;
        }
        case 12:
        {   const char *msg = "";
            if (music_editor_in_menu)
            {   msg = "B:load from file";
            }
            else
            {   msg = "B:play song from here";
            }
            font_render_line_doubled((const uint8_t *)msg, 96, internal_line, 65535, BG_COLOR*257);
            goto draw_song_command;
        }
        case 13:
            if (!music_editor_in_menu)
            {   font_render_line_doubled((uint8_t *)"X:cut cmd", 96, internal_line, 65535, BG_COLOR*257);
            }
            goto draw_song_command;
        case 14:
            if (!music_editor_in_menu)
                font_render_line_doubled((uint8_t *)"Y:insert cmd", 96, internal_line, 65535, BG_COLOR*257);
            goto draw_song_command;
        case 16:
            if (music_editor_in_menu)
                font_render_line_doubled((uint8_t *)"start:edit song", 96, internal_line, 65535, BG_COLOR*257);
            else
                font_render_line_doubled((uint8_t *)"start:song menu", 96, internal_line, 65535, BG_COLOR*257);
            goto draw_song_command;
        case 17:
            font_render_line_doubled((uint8_t *)"select:special", 96, internal_line, 65535, BG_COLOR*257);
            goto draw_song_command;
        case 18:
            if (GAMEPAD_HOLDING(0, select))
                font_render_line_doubled((uint8_t *)"> track < inst ^ up", 100, internal_line, 65535, BG_COLOR*257);
            break;
        case 19:
            break;
        default:
          draw_song_command:
            editSong_render_command(editSong_offset+line-2, internal_line);
            break; 
    }
}

void editSong_controls()
{   if (GAMEPAD_HOLDING(0, select))
    {   if (editSong_bad)
        {   strcpy((char *)game_message, "fix jump first");
            return;
        }
        game_message[0] = 0;
        if (GAMEPAD_PRESS(0, up))
            game_switch(ModeNameSong);
        else if (GAMEPAD_PRESS(0, left))
            game_switch(ModeEditInstrument);
        else if (GAMEPAD_PRESS(0, right))
            game_switch(ModeEditTrack);
        return;
    }

    if (music_editor_in_menu)
    {   io_event_t save_or_load = IoEventNone;
        if (GAMEPAD_PRESS(0, A))
            save_or_load = IoEventSave;
        if (GAMEPAD_PRESS(0, B))
            save_or_load = IoEventLoad;
        if (save_or_load)
        {   editSong_save_or_load_all(save_or_load);
            return;
        }

        int movement = 0;
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
            gamepad_press_wait[0] = GAMEPAD_PRESS_WAIT;
            game_message_timeout = MESSAGE_TIMEOUT;
            return;
        }

        if (GAMEPAD_PRESS(0, X))
        {   // load just editSong
            io_message_from_error(game_message, io_load_song(), IoEventLoad);
            return;
        }
    }  
    else
    {   // editing, not menu
        if (GAMEPAD_PRESSING(0, down))
        {
            if (editSong_pos < 255)
            {   ++editSong_pos;
                if (editSong_pos > editSong_offset + 15)
                    editSong_offset = editSong_pos - 15;
            }
            else
            {   editSong_pos = 0;
                editSong_offset = 0;
            }
            gamepad_press_wait[0] = GAMEPAD_PRESS_WAIT;
            return;
        }
        if (GAMEPAD_PRESSING(0, up))
        {
            if (editSong_pos)
            {   --editSong_pos;
                if (editSong_pos < editSong_offset)
                    editSong_offset = editSong_pos;
            }
            else
            {   editSong_pos = 255;
                editSong_offset = 240;
            }
            gamepad_press_wait[0] = GAMEPAD_PRESS_WAIT;
            return;
        }
        int movement = 0;
        if (GAMEPAD_PRESSING(0, L))
            --movement;
        if (GAMEPAD_PRESSING(0, R))
            ++movement;
        if (movement)
        {   uint8_t *memory = &chip_song_cmd[editSong_pos];
            *memory = ((*memory+movement)&15)|((*memory)&240);
            editSong_check();
            return;
        }
        if (GAMEPAD_PRESSING(0, left))
            --movement;
        if (GAMEPAD_PRESSING(0, right))
            ++movement;
        if (movement)
        {   gamepad_press_wait[0] = GAMEPAD_PRESS_WAIT;
            uint8_t *memory = &chip_song_cmd[editSong_pos];
            *memory = ((*memory)&15)|((*memory + 16*movement)&240);
            editSong_check();
            return;
        }

        if (GAMEPAD_PRESS(0, X))
        {   // delete / cut
            editSong_command_copy = chip_song_cmd[editSong_pos];
            for (int j=editSong_pos; j<255; ++j)
            {   chip_song_cmd[j] = chip_song_cmd[j+1];
            }
            editSong_check();
            return;
        }

        if (GAMEPAD_PRESS(0, Y))
        {   // insert
            for (int j=255; j>editSong_pos; --j)
            {   chip_song_cmd[j] = chip_song_cmd[j-1];
            }
            chip_song_cmd[editSong_pos] = editSong_command_copy;
            editSong_check();
            return;
        }
        
        if (editSong_bad) // can't do anything else until you fix this
            return;

        if (GAMEPAD_PRESS(0, A))
        {   // Toggle playing the song on/off from beginning
            game_message[0] = 0;
            if (chip_playing)
                chip_kill();
            else
                chip_play_song(0);
            return;
        } 
        
        if (GAMEPAD_PRESS(0, B))
        {   // Toggle playing the song on/off
            game_message[0] = 0;
            if (chip_playing)
                chip_kill();
            else
                chip_play_song(editSong_pos);
            return;
        } 
    }
    
    if (GAMEPAD_PRESS(0, start))
    {
        game_message[0] = 0;
        music_editor_in_menu = 1 - music_editor_in_menu; 
        chip_kill();
        return;
    }
}

static void editSong_save_or_load_all(io_event_t save_or_load)
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

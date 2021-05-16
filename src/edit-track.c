#include "bitbox.h"
#include "chip.h"
#include "edit-instrument.h"
#include "edit-song.h"
#include "edit-track.h"
#include "font.h"
#include "game.h"
#include "name.h"
#include "io.h"

#include <stdlib.h> // rand

#define BG_COLOR 132
#define PLAY_COLOR (RGB(200, 100, 0)|(RGB(200, 100, 0)<<16))
#define BOX_COLOR (RGB(200, 200, 230)|(RGB(200, 200, 230)<<16))
#define MATRIX_WING_COLOR (RGB(30, 90, 90) | (RGB(30, 90, 90)<<16))
#define NUMBER_LINES 20

uint8_t editTrack_track_playtime;
uint8_t editTrack_track;
uint8_t editTrack_pos;
uint8_t editTrack_offset;
uint8_t editTrack_copying;
// keeps track of whether to continue showing track commands.  set to 1 at top
// and set to zero if you encounter a break.
uint8_t editTrack_show_track;
uint8_t editTrack_player;
uint8_t editTrack_command_copy;
uint8_t editTrack_bad;

enum
{   EditTrackMenuTrackLoHi = 0,
    EditTrackMenuTrackIndex,
    EditTrackMenuPlayer,
    EditTrackMenuTrackLength,
    EditTrackMenuMax,
};
uint8_t editTrack_menu_index;

void editTrack_init()
{   editTrack_track_playtime = 32;
    editTrack_track = 0;
    editTrack_pos = 0;
    editTrack_offset = 0;
    editTrack_command_copy = rand()%16;
    editTrack_bad = 0;
}

void editTrack_load_defaults()
{
}

void editTrack_short_command_message(uint8_t *buffer, uint8_t cmd)
{
    switch (cmd&15)
    {
        case TrackBreak:
            strcpy((char *)buffer, "break");
            break;
        case TrackOctave:
            strcpy((char *)buffer, "octave");
            break;
        case TrackInstrument:
            strcpy((char *)buffer, "instrument");
            break;
        case TrackVolume:
            strcpy((char *)buffer, "volume");
            break;
        case TrackFadeInOrOut:
            strcpy((char *)buffer, "fade in/out");
            break;
        case TrackNote:
            strcpy((char *)buffer, "note");
            break;
        case TrackWait:
            strcpy((char *)buffer, "wait");
            break;
        case TrackArpNote:
            strcpy((char *)buffer, "arp. note");
            break;
        case TrackArpScale:
            strcpy((char *)buffer, "arp. scale");
            break;
        case TrackArpWait:
            strcpy((char *)buffer, "arp. wait");
            break;
        case TrackInertia:
            strcpy((char *)buffer, "inertia");
            break;
        case TrackVibrato:
            strcpy((char *)buffer, "vibrato");
            break;
        case TrackBend:
            strcpy((char *)buffer, "bend");
            break;
        case TrackStatic:
            strcpy((char *)buffer, "static");
            break;
        case TrackRandomize:
            strcpy((char *)buffer, "randomize");
            break;
        case TrackJump:
            strcpy((char *)buffer, "jump");
            break;
    }
}

void editTrack_render_command(int j, int y)
{
    int x = 32;
    #ifdef EMULATOR
    if (y < 0 || y >= 8)
    {
        message("got too big a line count for editTrack %d, line %d\n", (int)editTrack_track, j);
        return;
    }
    #endif
    
    uint8_t cmd = chip_track[editTrack_track][editTrack_player][j];
    uint8_t param = cmd>>4;
    cmd &= 15;

    uint32_t *dst = (uint32_t *)draw_buffer + x/2;
    uint32_t color_choice[2];
    if (j%2)
        color_choice[0] = 16843009u*BG_COLOR;
    else
        color_choice[0] = 16843009u*149;
    
    if (j != editTrack_pos)
        color_choice[1] = 65535u*65537u;
    else
    {
        color_choice[1] = RGB(190, 245, 255)|(RGB(190, 245, 255)<<16);
        if (!music_editor_in_menu)
        {
            if ((y+1)/2 == 1)
            {
                dst -= 4;
                *dst = color_choice[1];
                ++dst;
                *dst = color_choice[1];
                dst += 4 - 1;
            }
            else if ((y+1)/2 == 3)
            {
                dst -= 4;
                *dst = 16843009u*BG_COLOR;
                ++dst;
                *dst = 16843009u*BG_COLOR;
                dst += 4 - 1;
            }
        }
    }
    
    switch (cmd)
    {   case TrackBreak:
            if (param == 0)
            {   if
                (   y == 7 && 
                    (   j == 0 ||
                        (   chip_track[editTrack_track][editTrack_player][j-1]&15
                        )   != TrackRandomize
                    )
                )
                {   editTrack_show_track = 0;
                }
                cmd = '0';
                param = '0';
            }
            else
            {
                cmd = '0' + (4*param)/10;
                param = '0' + (4*param)%10; 
            }
            break;
        case TrackOctave:
            if (param < 7)
            {
                cmd = 'O';
                param += '0';
            }
            else if (param == 7)
            {
                cmd = '=';
                param = '=';
            }
            else if (param < 12)
            {
                cmd = (param%2) ? '+' : '/';
                param = '0' + (param - 6)/2;
            }
            else
            {
                cmd = (param%2) ? '\\' : '-';
                param = '0' + (17-param)/2;
            }
            break;
        case TrackInstrument:
            cmd = 'I';
            param = hex_character[param];
            break;
        case TrackVolume:
            cmd = 'V';
            param = hex_character[param];
            break;
        case TrackFadeInOrOut:
            if (param > 7)
            {   // Fade out
                cmd = '>'; 
                param = hex_character[16 - param];
            }
            else
            {   // Fade in
                cmd = '<'; 
                param = hex_character[param];
            }
            break;
        case TrackNote:
            if (param >= 12)
                color_choice[1] = RGB(150,150,255)|(65535<<16);
            param %= 12;
            cmd = note_name[param][0];
            param = note_name[param][1];
            break;
        case TrackWait:
            cmd = 'W';
            if (param)
                param = hex_character[param];
            else
                param = 'g';
            break;
        case TrackArpNote:
            color_choice[1] = RGB(150,255,150)|(65535<<16);
            if (param >= 12)
            {   switch (param)
                {   case ArpPlayLowNote:
                        cmd = 9; // down staircase
                        param = '_';
                        break;
                    case ArpPlayHighNote:
                        cmd = 10; // up staircase
                        param = 248; // upper bar
                        break;
                    case ArpPlayNextNoteDown:
                        cmd = 9; // down staircase
                        param = '-';
                        break;
                    case ArpPlayNextNoteUp:
                        cmd = 10; // up staircase
                        param = '+';
                        break;
                }
            }
            else
            {   cmd = note_name[param][0];
                param = note_name[param][1];
            }
            break;
        case TrackArpScale:
            cmd = 10; // staircase
            // TODO:
            param = hex_character[param];
            break;
        case TrackArpWait:
            cmd = 'a';
            param = hex_character[1 + param];
            break;
        case TrackInertia:
            cmd = 'i';
            param = hex_character[param];
            break;
        case TrackVibrato:
            cmd = '~';
            param = hex_character[param];
            break;
        case TrackBend:
            if (param < 8)
            {   cmd = 11; // bend up
                param = hex_character[param];
            }
            else
            {   cmd = 12; // bend down
                param = hex_character[16-param];
            }
            break;
        case TrackStatic:
            cmd = 'S'; 
            param = hex_character[param];
            break;
        case TrackRandomize:
            cmd = 'R';
            param = 224 + param;
            break;
        case TrackJump:
            cmd = 'J';
            param = hex_character[2*param];
            break;
    }
    
    uint8_t shift = ((y/2))*4;
    uint8_t row = (font[hex_character[j]] >> shift) & 15;
    *(++dst) = color_choice[0];
    for (int k=0; k<4; ++k)
    {
        *(++dst) = color_choice[row&1];
        row >>= 1;
    }
    *(++dst) = color_choice[0];
    row = (font[':'] >> shift) & 15;
    for (int k=0; k<4; ++k)
    {
        *(++dst) = color_choice[row&1];
        row >>= 1;
    }
    *(++dst) = color_choice[0];
    *(++dst) = color_choice[0];
    row = (font[cmd] >> shift) & 15;
    for (int k=0; k<4; ++k)
    {
        *(++dst) = color_choice[row&1];
        row >>= 1;
    }
    *(++dst) = color_choice[0];
    
    row = (font[param] >> shift) & 15;
    for (int k=0; k<4; ++k)
    {
        *(++dst) = color_choice[row&1];
        row >>= 1;
    }
    *(++dst) = color_choice[0];
  
    if (chip_playing != PlayingTrack)
        return;
    int cmd_index = chip_player[editTrack_player].track_cmd_index;
    if (cmd_index)
    switch (chip_track[editTrack_track][editTrack_player][cmd_index-1]&15)
    {
        case TrackBreak:
        case TrackWait:
        case TrackArpNote:
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

int _check_editTrack();

void check_editTrack()
{
    // check if that parameter broke something
    if (_check_editTrack())
    {
        editTrack_bad = 1; 
        game_set_message_with_timeout("bad jump, need wait in loop.", MESSAGE_TIMEOUT);
    }
    else
    {
        editTrack_bad = 0; 
        game_message[0] = 0;
    }
    chip_kill();
}

void editTrack_adjust_parameter(int direction)
{
    if (!direction)
        return;
    uint8_t cmd = chip_track[editTrack_track][editTrack_player][editTrack_pos];
    uint8_t param = cmd>>4;
    cmd &= 15;
    param = (param + direction)&15;
    chip_track[editTrack_track][editTrack_player][editTrack_pos] = cmd | (param<<4);

    check_editTrack();
}

int _check_editTrack()
{
    // check for a JUMP which loops back on itself without waiting at least a little bit.
    // return 1 if so, 0 if not.
    int j=0; // current command index
    int j_last_jump = -1;
    int found_wait = 0;
    for (int k=0; k<64; ++k)
    {
        if (j >= MAX_TRACK_LENGTH) // got to the end
            return 0;
        if (j_last_jump >= 0)
        {
            if (j == j_last_jump) // we found our loop-back point
                return !(found_wait); // did we find a wait?
            int j_next_jump = -1;
            switch (chip_track[editTrack_track][editTrack_player][j]&15)
            {
                case TrackJump:
                    j_next_jump = 2*(chip_track[editTrack_track][editTrack_player][j]>>4);
                    if (j_next_jump == j_last_jump) // jumping forward to the original jump
                    {
                        message("jumped to the old jump\n");
                        return !(found_wait);
                    }
                    else if (j_next_jump > j_last_jump) // jumping past original jump
                    {
                        message("This probably shouldn't happen.\n");
                        j_last_jump = -1; // can't look for loops...
                    }
                    else
                    {
                        message("jumped backwards again?\n");
                        j_last_jump = j;
                        found_wait = 0;
                    }
                    j = j_next_jump;
                    break;
                case TrackWait:
                case TrackArpNote:
                    message("saw wait at j=%d\n", j);
                    found_wait = 1;
                    ++j;
                    break;
                case TrackBreak:
                    // check for a randomizer behind
                    if (j > 0 && (chip_track[editTrack_track][editTrack_player][j-1]&15) == TrackRandomize)
                    {}
                    else if ((chip_track[editTrack_track][editTrack_player][j]>>4) == 0)
                        return 0;
                    // fall through to ++j
                default:
                    ++j;
            }
        }
        else switch (chip_track[editTrack_track][editTrack_player][j]&15)
        {
            case TrackJump:
                j_last_jump = j;
                j = 2*(chip_track[editTrack_track][editTrack_player][j]>>4);
                if (j > j_last_jump)
                {
                    message("This probably shouldn't happen??\n");
                    j_last_jump = -1; // don't care, we moved ahead
                }
                else if (j == j_last_jump)
                {
                    message("jumped to itself\n");
                    return 1; // this is bad
                }
                else
                    found_wait = 0;
                break;
            case TrackBreak:
                // check for a randomizer behind
                if (j > 0 && (chip_track[editTrack_track][editTrack_player][j-1]&15) == TrackRandomize)
                {}
                else if ((chip_track[editTrack_track][editTrack_player][j]>>4) == 0)
                    return 0;
                // fall through to ++j
            default:
                ++j;
        }
    }
    message("couldn't finish after 32 iterations. congratulations.\nprobably looping back on self, but with waits.");
    return 0;
}

// TODO: make sure to show all commands, even if they are past a break.
// we can jump to those locations.  maybe make them gray.
void editTrack_line()
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
        if (music_editor_in_menu && line == 0)
        {   uint32_t *dst = (uint32_t *)draw_buffer + 39;
            const uint32_t color = BOX_COLOR;
            switch (editTrack_menu_index)
            {   case EditTrackMenuTrackIndex:
                    dst += 4;
                    break;
                case EditTrackMenuPlayer:
                    dst += 18;
                    break;
                case EditTrackMenuTrackLength:
                    dst += 70;
                    break;
            }
            *(++dst) = color;
            *(++dst) = color;
            *(++dst) = color;
            *(++dst) = color;
        }
        return;
    }
    --internal_line;
    uint8_t buffer[24];
    switch (line)
    {
        case 0:
        {   // track Lx Pp tklen 64
            uint8_t msg[] = {
                (chip_playing && (track_pos/4 % 2==0)) ? '*' : ' ',
                't', 'r', 'a', 'c', 'k', ' ', 
                editTrack_track < 16
                ?   't' : 'T',
                hex_character[editTrack_track & 15], 
                ' ', 'P', hex_character[editTrack_player], 
                ' ', 'I', hex_character[chip_player[editTrack_player].instrument],
                ' ', 't', 'T', 'i', 'm', 'e', 
                ' ', '0' + editTrack_track_playtime/10, '0' + editTrack_track_playtime%10,
            0 };
            font_render_line_doubled(msg, 16, internal_line, 65535, BG_COLOR*257);
            break;
        }
        case 1:
            break;
        case 2:
        {
            editTrack_show_track = 1; 
            editTrack_render_command(editTrack_offset+line-2, internal_line);
            // command
            uint8_t msg[] = { 'c', 'o', 'm', 'm', 'a', 'n', 'd', ' ', hex_character[editTrack_pos], ':', 0 };
            font_render_line_doubled(msg, 96, internal_line, 65535, BG_COLOR*257);
            break;
        }
        case 3:
            switch (chip_track[editTrack_track][editTrack_player][editTrack_pos]&15)
            {
                case TrackBreak:
                    strcpy((char *)buffer, "break until time");
                    break;
                case TrackOctave:
                    strcpy((char *)buffer, "octave");
                    break;
                case TrackInstrument:
                    strcpy((char *)buffer, "instrument");
                    break;
                case TrackVolume:
                    strcpy((char *)buffer, "volume");
                    break;
                case TrackFadeInOrOut:
                    strcpy((char *)buffer, "fade in or out");
                    break;
                case TrackNote:
                    strcpy((char *)buffer, "relative note from C");
                    break;
                case TrackWait:
                    strcpy((char *)buffer, "wait");
                    break;
                case TrackArpNote:
                    strcpy((char *)buffer, "arpeggio note");
                    break;
                case TrackArpScale:
                    strcpy((char *)buffer, "arp. scale \\ # notes");
                    break;
                case TrackArpWait:
                    strcpy((char *)buffer, "arpeggio note length");
                    break;
                case TrackInertia:
                    strcpy((char *)buffer, "note inertia");
                    break;
                case TrackVibrato:
                    strcpy((char *)buffer, "vibrato rate, depth");
                    break;
                case TrackBend:
                    strcpy((char *)buffer, "track pitch bend");
                    break;
                case TrackStatic:
                    strcpy((char *)buffer, "track static");
                    break;
                case TrackRandomize:
                    strcpy((char *)buffer, "randomize next cmd");
                    break;
                case TrackJump:
                    strcpy((char *)buffer, "jump to cmd index");
                    break;
            }
            font_render_line_doubled(buffer, 102, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_track;
        case 4:
        {   uint8_t command = chip_track[editTrack_track][editTrack_player][editTrack_pos];
            uint8_t param = command >> 4;
            command &= 15;
            switch (command)
            {   case TrackOctave:
                    if (param == 7)
                    {   font_render_line_doubled
                        (   (const uint8_t *)"set from instrument",
                            120, internal_line, 65535, BG_COLOR*257
                        );
                    }
                    break;
                case TrackArpNote:
                {   const char *msg;
                    switch (param)
                    {   case 12:
                            msg = "drop to bass note";
                            break;
                        case 13:
                            msg = "hit top note";
                            break;
                        case 14:
                            msg = "hit higher note";
                            break;
                        case 15:
                            msg = "hit lower note";
                            break;
                        default:
                            msg = "set bass note";
                            break;
                    }
                    if (msg[0])
                        font_render_line_doubled((const uint8_t *)msg, 120, internal_line, 65535, BG_COLOR*257);
                    break;
                }
                case TrackArpScale:
                {   const char *msg;
                    switch (param)
                    {   case ScaleMajorTriad:
                            msg = "Major \\ 3";
                            break;
                        case ScaleMinorTriad:
                            msg = "minor \\ 3";
                            break;
                        case ScaleMajor7:
                            msg = "Maj7 \\ 4";
                            break;
                        case ScaleMinor7:
                            msg = "min7 \\ 4";
                            break;
                        case ScaleSuspended4Triad:
                            msg = "sus4 \\ 3";
                            break;
                        case ScaleSuspended2Triad:
                            msg = "sus2 \\ 3";
                            break;
                        case ScaleMajor6:
                            msg = "Maj6 \\ 4";
                            break;
                        case ScaleMinor6:
                            msg = "min6 \\ 4";
                            break;
                        case ScaleAugmentedTriad:
                            msg = "aug \\ 3";
                            break;
                        case ScaleDiminishedTriad:
                            msg = "dim \\ 3";
                            break;
                        case ScaleAugmented7:
                            msg = "aug7 \\ 4";
                            break;
                        case ScaleDiminished7:
                            msg = "dim7 \\ 4";
                            break;
                        case ScaleFifths:
                            msg = "fifths \\ 2";
                            break;
                        case ScaleOctaves:
                            msg = "Octaves \\ 1";
                            break;
                        case ScaleDominant7:
                            msg = "Dom7 \\ 4";
                            break;
                        case ScaleChromatic:
                            msg = "chromatic \\ 12";
                            break;
                        default:
                            msg = "??";
                    }
                    font_render_line_doubled((const uint8_t *)msg, 120, internal_line, 65535, BG_COLOR*257);
                    break;
                }
                case TrackVibrato:
                {   uint8_t msg[16];
                    switch (param/4) // rate
                    {
                        case 0:
                            strcpy((char *)msg, "  slow, ");
                            break;
                        case 1:
                            strcpy((char *)msg, "medium, ");
                            break;
                        case 2:
                            strcpy((char *)msg, "  fast, ");
                            break;
                        case 3:
                            strcpy((char *)msg, " gamma, ");
                            break;
                    }
                    msg[8] = hex_character[param % 4];
                    msg[9] = 0;
                    font_render_line_doubled(msg, 120, internal_line, 65535, BG_COLOR*257);
                    break;
                }
            }
            goto maybe_show_track;
        }
        case 5:
            font_render_line_doubled((uint8_t *)"switch to:", 102 - 6*music_editor_in_menu, internal_line, 65535, BG_COLOR*257); 
            goto maybe_show_track;
        case 6:
            if (music_editor_in_menu)
            {
                font_render_line_doubled((uint8_t *)"L:prev track", 112, internal_line, 65535, BG_COLOR*257);
            }
            else
            {
                buffer[0] = 'L'; buffer[1] = ':';
                editTrack_short_command_message(buffer+2, chip_track[editTrack_track][editTrack_player][editTrack_pos]-1);
                font_render_line_doubled(buffer, 112, internal_line, 65535, BG_COLOR*257);
            }
            goto maybe_show_track;
        case 7:
            if (music_editor_in_menu)
            {
                font_render_line_doubled((uint8_t *)"R:next track", 112, internal_line, 65535, BG_COLOR*257);
            }
            else
            {
                buffer[0] = 'R'; buffer[1] = ':';
                editTrack_short_command_message(buffer+2, chip_track[editTrack_track][editTrack_player][editTrack_pos]+1);
                font_render_line_doubled(buffer, 112, internal_line, 65535, BG_COLOR*257);
            }
            goto maybe_show_track;
        case 8:
            font_render_line_doubled((uint8_t *)"dpad:", 102 - 6*music_editor_in_menu, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_track;
        case 9:
        {   const char *msg = "";
            if (music_editor_in_menu)
            switch (editTrack_menu_index)
            {   case EditTrackMenuTrackLoHi:
                    msg = "change lo/hi (t/T)";
                    break;
                case EditTrackMenuTrackIndex:
                    msg = "change track";
                    break;
                case EditTrackMenuPlayer:
                    msg = "switch player";
                    break;
                case EditTrackMenuTrackLength:
                    msg = "change track length";
                    break;
            }
            else
                msg = "adjust parameters";
            font_render_line_doubled((const uint8_t *)msg, 112, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_track;
        }
        case 11:
            if (music_editor_in_menu)
            {
                if (editTrack_copying < CHIP_PLAYERS * MAX_TRACKS)
                    font_render_line_doubled((uint8_t *)"A:cancel copy", 96, internal_line, 65535, BG_COLOR*257);
                else
                    font_render_line_doubled((uint8_t *)"A:save to file", 96, internal_line, 65535, BG_COLOR*257);
            }
            else if (chip_playing)
                font_render_line_doubled((uint8_t *)"A:stop track", 96, internal_line, 65535, BG_COLOR*257);
            else
                font_render_line_doubled((uint8_t *)"A:play track", 96, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_track;
        case 12:
            if (music_editor_in_menu)
            {
                if (editTrack_copying < CHIP_PLAYERS * MAX_TRACKS)
                    font_render_line_doubled((uint8_t *)"B/X:\"     \"", 96, internal_line, 65535, BG_COLOR*257);

                else
                    font_render_line_doubled((uint8_t *)"B:load from file", 96, internal_line, 65535, BG_COLOR*257);
            }
            else
                font_render_line_doubled((uint8_t *)"B:edit instrument", 96, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_track;
        case 13:
            if (music_editor_in_menu)
            {
                if (editTrack_copying < CHIP_PLAYERS * MAX_TRACKS)
                    font_render_line_doubled((uint8_t *)"Y:paste", 96, internal_line, 65535, BG_COLOR*257);

                else
                    font_render_line_doubled((uint8_t *)"X:copy", 96, internal_line, 65535, BG_COLOR*257);
            }
            else
            {
                font_render_line_doubled((uint8_t *)"X:cut cmd", 96, internal_line, 65535, BG_COLOR*257);
            }
            goto maybe_show_track;
        case 14:
            if (music_editor_in_menu)
            {
                if (editTrack_copying < CHIP_PLAYERS * MAX_TRACKS)
                    goto maybe_show_track;
                strcpy((char *)buffer, "Y:file ");
                strcpy((char *)(buffer+7), (char *)base_song_filename);
                font_render_line_doubled(buffer, 96, internal_line, 65535, BG_COLOR*257);
            }
            else
                font_render_line_doubled((uint8_t *)"Y:insert cmd", 96, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_track;
        case 16:
            if (music_editor_in_menu)
                font_render_line_doubled((uint8_t *)"start:edit track", 96, internal_line, 65535, BG_COLOR*257);
            else
                font_render_line_doubled((uint8_t *)"start:track menu", 96, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_track;
        case 17:
            font_render_line_doubled((uint8_t *)"select:special", 96, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_track;
        case 18:
            if (GAMEPAD_HOLDING(0, select))
                font_render_line_doubled((uint8_t *)"> inst < song ^ up", 100, internal_line, 65535, BG_COLOR*257);
            break;
        case 19:
            break;
        default:
          maybe_show_track:
            if (editTrack_show_track)
                editTrack_render_command(editTrack_offset+line-2, internal_line);
            break; 
    }
}

void editTrack_controls()
{   if (GAMEPAD_HOLDING(0, select))
    {   if (editTrack_bad)
        {   strcpy((char *)game_message, "fix jump first");
            return;
        }
        game_message[0] = 0;
        if (GAMEPAD_PRESS(0, up))
            game_switch(ModeNameSong);
        else if (GAMEPAD_PRESS(0, left))
            game_switch(ModeEditSong);
        else if (GAMEPAD_PRESS(0, right))
            game_switch(ModeEditInstrument);
        return;
    }

    if (music_editor_in_menu)
    {   int switched = 0;
        if (GAMEPAD_PRESSING(0, left))
            --switched;
        if (GAMEPAD_PRESSING(0, right))
            ++switched;
        if (switched)
        {   game_message[0] = 0;
            editTrack_menu_index =
            (   editTrack_menu_index + switched + EditTrackMenuMax
            ) % EditTrackMenuMax;
            gamepad_press_wait[0] = GAMEPAD_PRESS_WAIT;
            return;
        }
        if (GAMEPAD_PRESSING(0, down))
            --switched;
        if (GAMEPAD_PRESSING(0, up))
            ++switched;
        if (switched)
        {   game_message[0] = 0;
            editTrack_pos = 0;
            editTrack_offset = 0;
            switch (editTrack_menu_index)
            {   case EditTrackMenuTrackLoHi:
                    editTrack_track ^= 16;
                    break;
                case EditTrackMenuTrackIndex:
                    editTrack_track = (editTrack_track & 16) |
                        ((editTrack_track + switched) & 15);
                    break;
                case EditTrackMenuPlayer:
                    editTrack_player = (editTrack_player+switched)&3;
                    break;
                case EditTrackMenuTrackLength:
                    editTrack_track_playtime += switched * 4;
                    if (editTrack_track_playtime > 64)
                        editTrack_track_playtime = 64;
                    else if (editTrack_track_playtime < 16)
                        editTrack_track_playtime = 16;
                    break;
            }
            gamepad_press_wait[0] = GAMEPAD_PRESS_WAIT;
            return;
        }
        
        if (GAMEPAD_PRESSING(0, L))
            --switched;
        if (GAMEPAD_PRESSING(0, R))
            ++switched;
        if (switched)
        {
            game_message[0] = 0;
            // Need a power of two for MAX_TRACKS:
            ASSERT(((MAX_TRACKS - 1)&MAX_TRACKS) == 0);
            editTrack_track = (editTrack_track+switched)&(MAX_TRACKS - 1);
            editTrack_pos = 0;
            editTrack_offset = 0;
            gamepad_press_wait[0] = GAMEPAD_PRESS_WAIT;
            return;
        }

        if (GAMEPAD_PRESS(0, X)) // copy
        {   ASSERT(CHIP_PLAYERS * MAX_TRACKS < 256);
            if (editTrack_copying < CHIP_PLAYERS * MAX_TRACKS)
            {
                editTrack_copying = CHIP_PLAYERS * MAX_TRACKS;
                game_message[0] = 0;
            }
            else
            {
                editTrack_copying = CHIP_PLAYERS*editTrack_track + editTrack_player;
                game_set_message_with_timeout("copied.", MESSAGE_TIMEOUT);
            }
        }

        if (GAMEPAD_PRESS(0, Y)) // paste
        {
            if (editTrack_copying < CHIP_PLAYERS * MAX_TRACKS)
            {
                // paste
                uint8_t *src, *dst;
                src = &chip_track[editTrack_copying/4][editTrack_copying%CHIP_PLAYERS][0];
                dst = &chip_track[editTrack_track][editTrack_player][0];
                if (src == dst)
                {
                    editTrack_copying = CHIP_PLAYERS * MAX_TRACKS;
                    game_set_message_with_timeout("pasting to same thing", MESSAGE_TIMEOUT);
                    return;
                }
                memcpy(dst, src, sizeof(chip_track[0][0]));
                game_set_message_with_timeout("pasted.", MESSAGE_TIMEOUT);
                editTrack_copying = CHIP_PLAYERS * MAX_TRACKS;
            }
            else
            {   // TODO: remove and use select + up
                // switch to choose name and hope to come back
                game_message[0] = 0;
                game_switch(ModeNameSong);
            }
            return;
        }
        
        int save_or_load = IoEventNone;
        if (GAMEPAD_PRESS(0, A))
            save_or_load = IoEventSave;
        if (GAMEPAD_PRESS(0, B))
            save_or_load = IoEventLoad;
        if (save_or_load)
        {
            if (editTrack_copying < CHIP_PLAYERS * MAX_TRACKS)
            {
                // cancel a copy 
                editTrack_copying = CHIP_PLAYERS * MAX_TRACKS;
                game_message[0] = 0;
                return;
            }

            io_error_t error;
            if (save_or_load == IoEventSave)
                error = io_save_track(editTrack_track);
            else
                error = io_load_track(editTrack_track);
            io_message_from_error(game_message, error, save_or_load);
            return;
        }
    }
    else
    {   // editing, not menu
        int movement = 0;
        if (GAMEPAD_PRESSING(0, L))
            --movement;
        if (GAMEPAD_PRESSING(0, R))
            ++movement;
        if (movement)
        {
            uint8_t *memory = &chip_track[editTrack_track][editTrack_player][editTrack_pos];
            *memory = ((*memory+movement)&15)|((*memory)&240);
            check_editTrack();
        }
        if (GAMEPAD_PRESSING(0, down))
        {
            if (editTrack_pos < MAX_TRACK_LENGTH-1 &&
                chip_track[editTrack_track][editTrack_player][editTrack_pos])
            {
                ++editTrack_pos;
                if (editTrack_pos > editTrack_offset + 15)
                    editTrack_offset = editTrack_pos - 15;
            }
            else
            {
                editTrack_pos = 0;
                editTrack_offset = 0;
            }
            movement = 1;
        }
        if (GAMEPAD_PRESSING(0, up))
        {
            if (editTrack_pos)
            {
                --editTrack_pos;
                if (editTrack_pos < editTrack_offset)
                    editTrack_offset = editTrack_pos;
            }
            else
            {
                editTrack_pos = 0;
                while (editTrack_pos < MAX_TRACK_LENGTH-1 && 
                    chip_track[editTrack_track][editTrack_player][editTrack_pos] != TrackBreak)
                {
                    ++editTrack_pos;
                }
                if (editTrack_pos > editTrack_offset + 15)
                    editTrack_offset = editTrack_pos - 15;
            }
            movement = 1;
        }
        if (GAMEPAD_PRESSING(0, left))
        {
            editTrack_adjust_parameter(-1);
            movement = 1;
        }
        if (GAMEPAD_PRESSING(0, right))
        {
            editTrack_adjust_parameter(+1);
            movement = 1;
        }
        if (movement)
        {
            gamepad_press_wait[0] = GAMEPAD_PRESS_WAIT;
            return;
        }

        if (GAMEPAD_PRESS(0, X))
        { 
            // delete / cut
            editTrack_command_copy = chip_track[editTrack_track][editTrack_player][editTrack_pos];
            for (int j=editTrack_pos; j<MAX_TRACK_LENGTH-1; ++j)
            {
                if ((chip_track[editTrack_track][editTrack_player][j] = chip_track[editTrack_track][editTrack_player][j+1]) == 0)
                    break;
            }
            chip_track[editTrack_track][editTrack_player][MAX_TRACK_LENGTH-1] = TrackBreak;
            check_editTrack();
            return;
        }

        if (GAMEPAD_PRESS(0, Y))
        {
            // insert
            if ((chip_track[editTrack_track][editTrack_player][MAX_TRACK_LENGTH-1]&15) != TrackBreak)
                return game_set_message_with_timeout("list full, can't insert.", MESSAGE_TIMEOUT);
            for (int j=MAX_TRACK_LENGTH-1; j>editTrack_pos; --j)
            {
                chip_track[editTrack_track][editTrack_player][j] = chip_track[editTrack_track][editTrack_player][j-1];
            }
            chip_track[editTrack_track][editTrack_player][editTrack_pos] = editTrack_command_copy;
            check_editTrack();
            return;
        }
        
        if (editTrack_bad) // can't do anything else until you fix this
            return;

        if (GAMEPAD_PRESS(0, A))
        {
            // play all instruments (or stop them all)
            track_pos = 0;
            if (chip_playing)
            {
                message("stop play\n");
                chip_kill();
            }
            else
            {
                // play this instrument track.
                // after the repeat, all tracks will sound.
                chip_play_track(editTrack_track, editTrack_track_playtime);
                // avoid playing other instruments for now:
                for (int i=0; i<editTrack_player; ++i)
                    chip_player[i].track_cmd_index = MAX_TRACK_LENGTH;
                for (int i=editTrack_player+1; i<4; ++i)
                    chip_player[i].track_cmd_index = MAX_TRACK_LENGTH;
            }
            return;
        }
        
        if (GAMEPAD_PRESS(0, B))
        {
            editInstrument_instrument = chip_player[editTrack_player].instrument;
            game_switch(ModeEditInstrument);
            return;
        }
    }
    
    if (GAMEPAD_PRESS(0, start))
    {
        game_message[0] = 0;
        music_editor_in_menu = 1 - music_editor_in_menu; 
        editTrack_copying = CHIP_PLAYERS * MAX_TRACKS;
        chip_kill();
        return;
    }
}

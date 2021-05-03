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

void editSong_start(int load_song)
{   if (load_song)
    {   editSong_save_or_load_all(IoEventLoad);
    }
}

// TODO: make sure to show all commands, even if they are past a break.
// we can jump to those locations.  maybe make them gray.
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
        if (music_editor_in_menu && line == 0)
        {   uint32_t *dst = (uint32_t *)draw_buffer + 39;
            const uint32_t color = BOX_COLOR;
            switch (editSong_menu_index)
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
                editSong_track < 16
                ?   'L' : 'H',
                hex_character[editSong_track & 15], 
                ' ', 'P', hex_character[editSong_player], 
                ' ', 'I', hex_character[chip_player[editSong_player].instrument],
                ' ', 't', 'T', 'i', 'm', 'e', 
                ' ', '0' + editSong_track_playtime/10, '0' + editSong_track_playtime%10,
            0 };
            font_render_line_doubled(msg, 16, internal_line, 65535, BG_COLOR*257);
            break;
        }
        case 1:
            break;
        case 2:
        {
            editSong_show_track = 1; 
            editSong_render_command(editSong_offset+line-2, internal_line);
            // command
            uint8_t msg[] = { 'c', 'o', 'm', 'm', 'a', 'n', 'd', ' ', hex_character[editSong_pos], ':', 0 };
            font_render_line_doubled(msg, 96, internal_line, 65535, BG_COLOR*257);
            break;
        }
        case 3:
            switch (chip_track[editSong_track][editSong_player][editSong_pos]&15)
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
            goto maybe_show_song_command;
        case 4:
        {   uint8_t command = chip_track[editSong_track][editSong_player][editSong_pos];
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
            goto maybe_show_song_command;
        }
        case 5:
            font_render_line_doubled((uint8_t *)"switch to:", 102 - 6*music_editor_in_menu, internal_line, 65535, BG_COLOR*257); 
            goto maybe_show_song_command;
        case 6:
            if (music_editor_in_menu)
            {
                font_render_line_doubled((uint8_t *)"L:prev track", 112, internal_line, 65535, BG_COLOR*257);
            }
            else
            {
                buffer[0] = 'L'; buffer[1] = ':';
                editSong_short_command_message(buffer+2, chip_track[editSong_track][editSong_player][editSong_pos]-1);
                font_render_line_doubled(buffer, 112, internal_line, 65535, BG_COLOR*257);
            }
            goto maybe_show_song_command;
        case 7:
            if (music_editor_in_menu)
            {
                font_render_line_doubled((uint8_t *)"R:next track", 112, internal_line, 65535, BG_COLOR*257);
            }
            else
            {
                buffer[0] = 'R'; buffer[1] = ':';
                editSong_short_command_message(buffer+2, chip_track[editSong_track][editSong_player][editSong_pos]+1);
                font_render_line_doubled(buffer, 112, internal_line, 65535, BG_COLOR*257);
            }
            goto maybe_show_song_command;
        case 8:
            font_render_line_doubled((uint8_t *)"dpad:", 102 - 6*music_editor_in_menu, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_song_command;
        case 9:
        {   const char *msg = "";
            if (music_editor_in_menu)
            switch (editSong_menu_index)
            {   case EditTrackMenuTrackLoHi:
                    msg = "change lo/hi";
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
            goto maybe_show_song_command;
        }
        case 11:
            if (music_editor_in_menu)
            {
                if (editSong_copying < CHIP_PLAYERS * MAX_TRACKS)
                    font_render_line_doubled((uint8_t *)"A:cancel copy", 96, internal_line, 65535, BG_COLOR*257);
                else
                    font_render_line_doubled((uint8_t *)"A:save to file", 96, internal_line, 65535, BG_COLOR*257);
            }
            else if (chip_playing)
                font_render_line_doubled((uint8_t *)"A:stop track", 96, internal_line, 65535, BG_COLOR*257);
            else
                font_render_line_doubled((uint8_t *)"A:play track", 96, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_song_command;
        case 12:
            if (music_editor_in_menu)
            {
                if (editSong_copying < CHIP_PLAYERS * MAX_TRACKS)
                    font_render_line_doubled((uint8_t *)"B/X:\"     \"", 96, internal_line, 65535, BG_COLOR*257);

                else
                    font_render_line_doubled((uint8_t *)"B:load from file", 96, internal_line, 65535, BG_COLOR*257);
            }
            else
                font_render_line_doubled((uint8_t *)"B:edit instrument", 96, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_song_command;
        case 13:
            if (music_editor_in_menu)
            {
                if (editSong_copying < CHIP_PLAYERS * MAX_TRACKS)
                    font_render_line_doubled((uint8_t *)"Y:paste", 96, internal_line, 65535, BG_COLOR*257);

                else
                    font_render_line_doubled((uint8_t *)"X:copy", 96, internal_line, 65535, BG_COLOR*257);
            }
            else
            {
                font_render_line_doubled((uint8_t *)"X:cut cmd", 96, internal_line, 65535, BG_COLOR*257);
            }
            goto maybe_show_song_command;
        case 14:
            if (music_editor_in_menu)
            {
                if (editSong_copying < CHIP_PLAYERS * MAX_TRACKS)
                    goto maybe_show_song_command;
                strcpy((char *)buffer, "Y:file ");
                strcpy((char *)(buffer+7), (char *)base_song_filename);
                font_render_line_doubled(buffer, 96, internal_line, 65535, BG_COLOR*257);
            }
            else
                font_render_line_doubled((uint8_t *)"Y:insert cmd", 96, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_song_command;
        case 16:
            if (music_editor_in_menu)
                font_render_line_doubled((uint8_t *)"start:edit track", 96, internal_line, 65535, BG_COLOR*257);
            else
                font_render_line_doubled((uint8_t *)"start:track menu", 96, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_song_command;
        case 17:
            font_render_line_doubled((uint8_t *)"select:special", 96, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_song_command;
        case 18:
            if (GAMEPAD_HOLDING(0, select))
                font_render_line_doubled((uint8_t *)"> inst < song ^ up", 100, internal_line, 65535, BG_COLOR*257);
            break;
        case 19:
            break;
        default:
          maybe_show_song_command:
            if (editSong_show_track)
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
        
        if (GAMEPAD_PRESSING(0, B))
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
            gamepad_press_wait[0] = GAMEPAD_PRESS_WAIT;
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
            gamepad_press_wait[0] = GAMEPAD_PRESS_WAIT;
            game_message_timeout = MESSAGE_TIMEOUT;
            return;
        }

        if (GAMEPAD_PRESS(0, X))
        {   // load just editSong
            io_message_from_error(game_message, io_load_song(), 2);
            return;
        }

        // TODO: remove and use select + up
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
            gamepad_press_wait[0] = GAMEPAD_PRESS_WAIT;
            if (paint_if_moved)
                editSong_paint(paint_if_moved-1);
        }
        else if (switched || paint_if_moved)
            gamepad_press_wait[0] = GAMEPAD_PRESS_WAIT;

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

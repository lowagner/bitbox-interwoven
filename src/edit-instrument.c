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
#include <stdint.h> // rand

#define BG_COLOR 5
#define BOX_COLOR RGB(30, 20, 0)
#define PLAY_COLOR (RGB(220, 50, 0)|(RGB(220, 50, 0)<<16))
#define NUMBER_LINES 20

uint8_t editInstrument_note;
uint8_t editInstrument_instrument;
uint8_t editInstrument_cmd_index;
// TODO: WHAT DOES THIS DO:
uint8_t show_instrument;
uint8_t editInstrument_bad;
uint8_t editInstrument_copying;
uint8_t editInstrument_cursor;
uint8_t editInstrument_command_copy;

void editInstrument_init()
{   // Initialize the instrument editor
    editInstrument_instrument = 0;
    editInstrument_cmd_index = 0;
    editInstrument_bad = 0;
    editInstrument_copying = 16;
    editInstrument_cursor = 0;
    editInstrument_command_copy = rand()%16;
}

void editInstrument_load_defaults()
{
    int i=0; // instrument 
    int ci = 0; // command index
    instrument[i].octave = 2;
    instrument[i].cmd[ci] = InstrumentPan | (8<<4); 
    instrument[i].cmd[++ci] = InstrumentVolume | (15<<4); 
    instrument[i].cmd[++ci] = InstrumentWaveform | (WfSaw<<4); 
    instrument[i].cmd[++ci] = InstrumentNote | (0<<4); 
    instrument[i].cmd[++ci] = InstrumentWait | (15<<4); 
    instrument[i].cmd[++ci] = InstrumentPan | (1<<4); 
    instrument[i].cmd[++ci] = InstrumentNote | (4<<4); 
    instrument[i].cmd[++ci] = InstrumentWait | (4<<4); 
    instrument[i].cmd[++ci] = InstrumentPan | (15<<4); 
    instrument[i].cmd[++ci] = InstrumentWait | (4<<4); 
    instrument[i].cmd[++ci] = InstrumentNote | (7<<4); 
    instrument[i].cmd[++ci] = InstrumentWait | (4<<4); 
    instrument[i].cmd[++ci] = InstrumentFadeMagnitude | (15<<4); 
    
    i = 1;
    ci = 0;
    instrument[i].octave = 3;
    instrument[i].cmd[ci] = InstrumentPan | (8<<4); 
    instrument[i].cmd[++ci] = InstrumentInertia | (15<<4); 
    instrument[i].cmd[++ci] = InstrumentVolume | (15<<4); 
    instrument[i].cmd[++ci] = InstrumentWaveform | (WfSine<<4); 
    instrument[i].cmd[++ci] = InstrumentNote | (0<<4); 
    instrument[i].cmd[++ci] = InstrumentWait | (15<<4); 
    instrument[i].cmd[++ci] = InstrumentVibrato | (0xc<<4); 
    instrument[i].cmd[++ci] = InstrumentWait | (15<<4); 
    instrument[i].cmd[++ci] = InstrumentWait | (15<<4); 
    instrument[i].cmd[++ci] = InstrumentFadeMagnitude | (1<<4); 
    
    i = 2;
    ci = 0;
    instrument[i].octave = 4;
    instrument[i].cmd[ci] = InstrumentPan | (8<<4); 
    instrument[i].cmd[++ci] = InstrumentVolume | (15<<4); 
    instrument[i].cmd[++ci] = InstrumentWaveform | (WfNoise<<4); 
    instrument[i].cmd[++ci] = InstrumentWait | (3<<4); 
    instrument[i].cmd[++ci] = InstrumentWaveform | (WfPulse<<4); 
    instrument[i].cmd[++ci] = InstrumentDutyDelta | (6<<4); 
    instrument[i].cmd[++ci] = InstrumentFadeMagnitude | (1<<4); 
    instrument[i].cmd[++ci] = InstrumentNote | (12<<4); 
    instrument[i].cmd[++ci] = InstrumentWait | (3<<4); 
    instrument[i].cmd[++ci] = InstrumentNote | (7<<4); 
    instrument[i].cmd[++ci] = InstrumentWait | (3<<4); 
    instrument[i].cmd[++ci] = InstrumentNote | (3<<4); 
    instrument[i].cmd[++ci] = InstrumentWait | (3<<4); 
    instrument[i].cmd[++ci] = InstrumentNote | (0<<4); 
    instrument[i].cmd[++ci] = InstrumentWait | (3<<4); 
    instrument[i].cmd[++ci] = InstrumentJump | (7<<4); 
    
    i = 3;
    ci = 0; 
    instrument[i].octave = 3;
    instrument[i].is_drum = 1; // drums get MAX_INSTRUMENT_LENGTH/4 commands for each sub-instrument
    instrument[i].cmd[ci] = InstrumentWait | (5<<4); 
    instrument[i].cmd[++ci] = InstrumentWaveform | (WfSine<<4); 
    instrument[i].cmd[++ci] = InstrumentNote | (0 << 4); 
    instrument[i].cmd[++ci] = InstrumentWait | (6<<4); 
    instrument[i].cmd[++ci] = InstrumentPan | (1 << 4);  
    instrument[i].cmd[++ci] = InstrumentWait | (6<<4); 
    instrument[i].cmd[++ci] = InstrumentNote | (2 << 4); 
    instrument[i].cmd[++ci] = InstrumentFadeMagnitude | (13<<4); // the first sub-instrument is long (8 commands) 
    instrument[i].cmd[++ci] = InstrumentPan | (1<<4); 
    instrument[i].cmd[++ci] = InstrumentWait | (15<<4); 
    instrument[i].cmd[++ci] = InstrumentPan | (15<<4); 
    instrument[i].cmd[++ci] = InstrumentFadeMagnitude | (10<<4);  // that was the second sub-instrument
    
    instrument[i].cmd[++ci] = InstrumentWait | (3<<4); 
    instrument[i].cmd[++ci] = InstrumentFadeMagnitude | (15<<4); 
    instrument[i].cmd[++ci] = 0; 
    instrument[i].cmd[++ci] = 0;  // that was the third (last) sub-instrument
}

static void editInstrument_short_command_message(uint8_t *buffer, uint8_t cmd)
{
    switch (cmd&15)
    {
        case InstrumentBreak:
            strcpy((char *)buffer, "break");
            break;
        case InstrumentPan:
            strcpy((char *)buffer, "pan");
            break;
        case InstrumentWaveform:
            strcpy((char *)buffer, "waveform");
            break;
        case InstrumentVolume:
            strcpy((char *)buffer, "volume");
            break;
        case InstrumentNote:
            strcpy((char *)buffer, "note");
            break;
        case InstrumentWait:
            strcpy((char *)buffer, "wait");
            break;
        case InstrumentFadeMagnitude:
            strcpy((char *)buffer, "fade amt");
            break;
        case InstrumentFadeBehavior:
            strcpy((char *)buffer, "fade type");
            break;
        case InstrumentInertia:
            strcpy((char *)buffer, "inertia");
            break;
        case InstrumentVibrato:
            strcpy((char *)buffer, "vibrato");
            break;
        case InstrumentBend:
            strcpy((char *)buffer, "bend");
            break;
        case InstrumentSpecial:
            strcpy((char *)buffer, "???");
            break;
        case InstrumentDuty:
            strcpy((char *)buffer, "duty");
            break;
        case InstrumentDutyDelta:
            strcpy((char *)buffer, "delta duty");
            break;
        case InstrumentRandomize:
            strcpy((char *)buffer, "randomize");
            break;
        case InstrumentJump:
            strcpy((char *)buffer, "jump");
            break;
    }
}

void editInstrument_render_command(int j, int y)
{
    int x = 32;
    #ifdef EMULATOR
    if (y < 0 || y >= 8)
    {
        message("got too big a line count for instrument %d, %d\n", (int)editInstrument_instrument, y);
        return;
    }
    #endif
    
    uint8_t cmd = instrument[editInstrument_instrument].cmd[j];
    uint8_t param = cmd>>4;
    cmd &= 15;
    int smash_together = 0;

    uint32_t *dst = (uint32_t *)draw_buffer + x/2;
    uint32_t color_choice[2];
    if (!instrument[editInstrument_instrument].is_drum || j < 2*DRUM_SECTION_LENGTH)
    {
        if (j % 2)
            color_choice[0] = 16843009u*BG_COLOR;
        else
            color_choice[0] = 16843009u*14;
    }
    else if (j < 3*DRUM_SECTION_LENGTH)
    {
        if (j % 2)
            color_choice[0] = 16843009u*41;
        else
            color_choice[0] = 16843009u*45;
    }
    else
    {
        if (j % 2)
            color_choice[0] = 16843009u*BG_COLOR;
        else
            color_choice[0] = 16843009u*9;
    }

    if (j != editInstrument_cmd_index)
    {
        color_choice[1] = 65535u*65537u;
    }
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
    
    if (cmd == InstrumentBreak)
    {
        if (param == 0)
        {
            if (y == 7)
            {
                if (j == 0 || (instrument[editInstrument_instrument].cmd[j-1]&15) != InstrumentRandomize)
                    show_instrument = 0;
            }
            cmd = '6';
            param = '4';
        }
        else
        {
            cmd = '0' + (4*param)/10;
            param = '0' + (4*param)%10; 
        }
    }
    else 
    switch (cmd)
    {
        case InstrumentPan:
        {    
            uint8_t L, R=param-1;
            L = 14-R;
            if (param % 8 == 0)
            {
                cmd = 'L';
                param = 'R';
            }
            else if (param < 8)
            {
                cmd = 'L';
                param = 240 + R;
            }
            else
            {
                cmd = 240 + L;
                param = 'R';
            }
            break;
        }
        case InstrumentWaveform:
            switch (param)
            {
                case WfSine:
                    cmd = 1;
                    param = 2;
                    break;
                case WfTriangle:
                    cmd = '/';
                    param = '\\';
                    break;
                case WfHalfUpSaw:
                    cmd = '/';
                    param = '_';
                    break;
                case WfHalfDownSaw:
                    cmd = '\\';
                    param = 248; // upper bar
                    break;
                case WfSaw:
                    cmd = 3;
                    param = 4;
                    break;
                case WfPulse:
                    cmd = 5;
                    param = 6;
                    break;
                case WfInvertedSine:
                    cmd = 217;
                    param = 216;
                    break;
                case WfHalfUpSine:
                    cmd = 1;
                    param = '_';
                    break;
                case WfHalfDownSine:
                    cmd = 2;
                    param = 248; // upper bar
                    break;
                case WfNoise:
                    cmd = 7;
                    param = 8;
                    break;
                case WfRed:
                    cmd = 7;
                    param = 9;
                    break;
                case WfViolet:
                    cmd = 7;
                    param = 10;
                    break;
                default:
                    cmd = ' ';
                    param = ' ';
                    break;
            }
            smash_together = 1;
            break;
        case InstrumentVolume:
            cmd = 'V';
            param = hex_character[param];
            break;
        case InstrumentNote:
            if (param >= 12)
                color_choice[1] = RGB(150,150,255)|(65535<<16);
            param %= 12;
            cmd = note_name[param][0];
            param = note_name[param][1];
            break;
        case InstrumentWait:
            cmd = 'W';
            if (param)
                param = hex_character[param];
            else
                param = 'g';
            break;
        case InstrumentFadeMagnitude:
            cmd = '>';
            param = hex_character[param];
            break;
        case InstrumentFadeBehavior:
            switch (param)
            {   case InstrumentFadeClampWithNegativeVolumeD:
                    cmd = '\\';
                    param = 241; // lower bar
                    break;
                case InstrumentFadeClampWithPositiveVolumeD:
                    cmd = '/';
                    param = 248; // upper bar
                    break;
                case InstrumentFadeReverseClampWithNegativeVolumeD:
                    cmd = '\\';
                    param = 248; // upper bar
                    break;
                case InstrumentFadeReverseClampWithPositiveVolumeD:
                    cmd = '/';
                    param = 241; // lower bar
                    break;
                case InstrumentFadePingPongStartingWithNegativeVolumeD:
                    cmd = '\\';
                    param = '/';
                    break;
                case InstrumentFadePingPongStartingWithPositiveVolumeD:
                    cmd = '/';
                    param = '\\';
                    break;
                case InstrumentFadePingPongLoweringVolumeStartingWithNegativeVolumeD:
                    cmd = '\\';
                    param = 220; // lowering mountain
                    break;
                case InstrumentFadePingPongLoweringVolumeStartingWithPositiveVolumeD:
                    cmd = '/';
                    param = 223; // lowering valley
                    break;
                case InstrumentFadePingPongRaisingVolumeStartingWithNegativeVolumeD:
                    cmd = '\\';
                    param = 222; // rising mountain
                    break;
                case InstrumentFadePingPongRaisingVolumeStartingWithPositiveVolumeD:
                    cmd = '/';
                    param = 221; // rising valley
                    break;
                case InstrumentFadeWrapWithNegativeVolumeD:
                    cmd = '\\';
                    param = '\\';
                    break;
                case InstrumentFadeWrapWithPositiveVolumeD:
                    cmd = '/';
                    param = '/';
                    break;
                case InstrumentFadeWrapLoweringVolumeWithNegativeVolumeD:
                    cmd = '\\';
                    param = 223; // lowering valley
                    break;
                case InstrumentFadeWrapLoweringVolumeWithPositiveVolumeD:
                    cmd = '/';
                    param = 219; // ramp
                    break;
                case InstrumentFadeWrapRaisingVolumeWithNegativeVolumeD:
                    cmd = '\\';
                    param = 218; // drop
                    break;
                case InstrumentFadeWrapRaisingVolumeWithPositiveVolumeD:
                    cmd = '/';
                    param = 222; // rising mountain
                    break;
            }
            smash_together = 1;
            break;
        case InstrumentInertia:
            cmd = 'i';
            param = hex_character[param];
            break;
        case InstrumentVibrato:
            cmd = '~';
            param = hex_character[param];
            break;
        case InstrumentBend:
            if (param < 8)
            {
                cmd = 11; // bend up
                param = hex_character[param];
            }
            else
            {
                cmd = 12; // bend down
                param = hex_character[16-param];
            }
            break;
        case InstrumentSpecial:
            cmd = 9;
            param = hex_character[param];
            break;
        case InstrumentDuty:
            cmd = 129; // Gamma
            param = hex_character[param];
            break;
        case InstrumentDutyDelta:
            cmd = 130; // Delta
            param = hex_character[param];
            break;
        case InstrumentRandomize:
            cmd = 'R';
            param = 224 + param;
            break;
        case InstrumentJump:
            cmd = 'J';
            param = hex_character[param];
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
    if (smash_together)
    {
        *(++dst) = color_choice[0];
        for (int k=0; k<4; ++k)
        {
            *(++dst) = color_choice[row&1];
            row >>= 1;
        }
        row = (font[param] >> shift) & 15;
        for (int k=0; k<4; ++k)
        {
            *(++dst) = color_choice[row&1];
            row >>= 1;
        }
    }
    else
    {
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
    }
    *(++dst) = color_choice[0];
    
    if (!chip_player[editTrack_player].track_volume)
        return;
    int cmd_index = chip_player[editTrack_player].cmd_index;
    if (cmd_index)
    switch (instrument[editInstrument_instrument].cmd[cmd_index-1]&15)
    {
        case InstrumentWait:
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

int _check_instrument();

void check_instrument()
{
    // check if that parameter broke something
    if (_check_instrument())
    {
        editInstrument_bad = 1; 
        game_set_message_with_timeout("bad jump, need wait in loop.", MESSAGE_TIMEOUT);
    }
    else
    {
        editInstrument_bad = 0; 
        game_message[0] = 0;
    }
    chip_player[editTrack_player].track_volume = 0;
    chip_player[editTrack_player].track_volumed = 0;
}

void editInstrument_adjust_parameter(int direction)
{
    if (!direction)
        return;
    uint8_t cmd = instrument[editInstrument_instrument].cmd[editInstrument_cmd_index];
    uint8_t param = cmd>>4;
    cmd &= 15;
    if (cmd == InstrumentWaveform)
    {
        param = param+direction;
        if (param > 15)
            param = WfViolet;
        // TODO: add more wavefunction types
        else if (param > WfViolet)
            param = WfSine;
    }
    else
    {
        param = (param+direction)&15;
    }
    instrument[editInstrument_instrument].cmd[editInstrument_cmd_index] = cmd | (param<<4);

    check_instrument();
}

int __check_instrument(uint8_t j, uint8_t j_max)
{
    // check for a InstrumentJump which loops back on itself without waiting at least a little bit.
    // return 1 if so, 0 if not.
    int j_last_jump = -1;
    int found_wait = 0;
    for (int k=0; k<32; ++k)
    {
        if (j >= j_max) // got to the end
        {
            message("made it to end, good!\n");
            return 0;
        }
        message("scanning instrument %d: line %d\n", editInstrument_instrument, j);
        if (j_last_jump >= 0)
        {
            if (j == j_last_jump) // we found our loop-back point
            {
                message("returned to the jump\n");
                return !(found_wait); // did we find a wait?
            }
            int j_next_jump = -1;
            switch (instrument[editInstrument_instrument].cmd[j]&15)
            {
                case InstrumentJump:
                    j_next_jump = instrument[editInstrument_instrument].cmd[j]>>4;
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
                case InstrumentWait:
                    message("saw wait at j=%d\n", j);
                    found_wait = 1;
                    ++j;
                    break;
                case InstrumentBreak:
                    // check for a randomizer behind
                    if (j > 0 && (instrument[editInstrument_instrument].cmd[j-1]&15) == InstrumentRandomize)
                    {}
                    else if ((instrument[editInstrument_instrument].cmd[j]>>4) == 0)
                        return 0;
                    // fall through to ++j
                default:
                    ++j;
            }
        }
        else switch (instrument[editInstrument_instrument].cmd[j]&15)
        {
            case InstrumentJump:
                j_last_jump = j;
                j = instrument[editInstrument_instrument].cmd[j]>>4;
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
            case InstrumentBreak:
                // check for a randomizer behind
                if (j > 0 && (instrument[editInstrument_instrument].cmd[j-1]&15) == InstrumentRandomize)
                {}
                else if ((instrument[editInstrument_instrument].cmd[j]>>4) == 0)
                    return 0;
                // fall through to ++j
            default:
                ++j;
        }
    }
    message("couldn't finish after iterations. congratulations.\nprobably looping back on self, but with waits.");
    return 0;
}

int _check_instrument()
{
    if (instrument[editInstrument_instrument].is_drum)
    {
        return __check_instrument(0, 2*DRUM_SECTION_LENGTH) ||
            __check_instrument(2*DRUM_SECTION_LENGTH, 3*DRUM_SECTION_LENGTH) ||
            __check_instrument(3*DRUM_SECTION_LENGTH, 4*DRUM_SECTION_LENGTH);
    }
    else
        return __check_instrument(0, MAX_INSTRUMENT_LENGTH);
}

void editInstrument_line()
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
        {
            uint16_t *dst = draw_buffer + (22 + editInstrument_cursor*7) * 9 - 1;
            const uint16_t color = BOX_COLOR;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
        }
        return;
    }
    --internal_line;
    uint8_t buffer[24];
    switch (line)
    {
        case 0:
        {
            // edit instrument
            uint8_t msg[] = { 'i', 'n', 's', 't', 'r', 'u', 'm', 'e', 'n', 't', 
                ' ', hex_character[editInstrument_instrument],
                ' ',  'o', 'c', 't', 'a', 'v', 'e',
                ' ',  hex_character[instrument[editInstrument_instrument].octave],
                ' ', 'd', 'r', 'u', 'm', ' ', (instrument[editInstrument_instrument].is_drum ? 'Y' : 'N'),
            0 };
            font_render_line_doubled(msg, 16, internal_line, 65535, BG_COLOR*257);
            break;
        }
        case 1:
            break;
        case 2:
        {
            show_instrument = 1; 
            editInstrument_render_command(line-2, internal_line);
            // command
            uint8_t msg[] = { 'c', 'o', 'm', 'm', 'a', 'n', 'd', ' ', hex_character[editInstrument_cmd_index], ':', 0 };
            font_render_line_doubled(msg, 96, internal_line, 65535, BG_COLOR*257);
            break;
        }
        case 10:
        case 14:
            if (instrument[editInstrument_instrument].is_drum || show_instrument)
            {
                show_instrument = 1; 
                editInstrument_render_command(line-2, internal_line);
            }
            break;
        case 3:
        {   uint8_t cmd_param = instrument[editInstrument_instrument].cmd[editInstrument_cmd_index];
            switch (cmd_param&15)
            {
                case InstrumentBreak:
                    strcpy((char *)buffer, "end if before tkpos");
                    break;
                case InstrumentPan:
                    strcpy((char *)buffer, "pan left/right");
                    break;
                case InstrumentWaveform:
                    strcpy((char *)buffer, "waveform");
                    break;
                case InstrumentVolume:
                    strcpy((char *)buffer, "volume");
                    break;
                case InstrumentNote:
                    strcpy((char *)buffer, "relative note from C");
                    break;
                case InstrumentWait:
                    strcpy((char *)buffer, "wait");
                    break;
                case InstrumentFadeMagnitude:
                    strcpy((char *)buffer, "fade magnitude");
                    break;
                case InstrumentFadeBehavior:
                    switch (cmd_param >> 4)
                    {   case InstrumentFadeClampWithNegativeVolumeD:
                            strcpy((char *)buffer, "fade to zero");
                            break;
                        case InstrumentFadeClampWithPositiveVolumeD:
                            strcpy((char *)buffer, "fade to max");
                            break;
                        case InstrumentFadeReverseClampWithNegativeVolumeD:
                            strcpy((char *)buffer, "fade to zero, insta-max");
                            break;
                        case InstrumentFadeReverseClampWithPositiveVolumeD:
                            strcpy((char *)buffer, "fade to max, insta-min");
                            break;
                        case InstrumentFadePingPongStartingWithNegativeVolumeD:
                            strcpy((char *)buffer, "fade ping pong down");
                            break;
                        case InstrumentFadePingPongStartingWithPositiveVolumeD:
                            strcpy((char *)buffer, "fade ping pong up");
                            break;
                        case InstrumentFadePingPongLoweringVolumeStartingWithNegativeVolumeD:
                            strcpy((char *)buffer, "ping pong dn decresc.");
                            break;
                        case InstrumentFadePingPongLoweringVolumeStartingWithPositiveVolumeD:
                            strcpy((char *)buffer, "ping pong up decresc.");
                            break;
                        case InstrumentFadePingPongRaisingVolumeStartingWithNegativeVolumeD:
                            strcpy((char *)buffer, "ping pong dn cresc.");
                            break;
                        case InstrumentFadePingPongRaisingVolumeStartingWithPositiveVolumeD:
                            strcpy((char *)buffer, "ping pong up cresc.");
                            break;
                        case InstrumentFadeWrapWithNegativeVolumeD:
                            strcpy((char *)buffer, "fade wrap down");
                            break;
                        case InstrumentFadeWrapWithPositiveVolumeD:
                            strcpy((char *)buffer, "fade wrap up");
                            break;
                        case InstrumentFadeWrapLoweringVolumeWithNegativeVolumeD:
                            strcpy((char *)buffer, "wrap down decresc.");
                            break;
                        case InstrumentFadeWrapLoweringVolumeWithPositiveVolumeD:
                            strcpy((char *)buffer, "wrap up decresc.");
                            break;
                        case InstrumentFadeWrapRaisingVolumeWithNegativeVolumeD:
                            strcpy((char *)buffer, "wrap down cresc.");
                            break;
                        case InstrumentFadeWrapRaisingVolumeWithPositiveVolumeD:
                            strcpy((char *)buffer, "wrap up cresc.");
                            break;
                    }
                    break;
                case InstrumentInertia:
                    strcpy((char *)buffer, "note inertia");
                    break;
                case InstrumentVibrato:
                    strcpy((char *)buffer, "vibrato rate, depth");
                    break;
                case InstrumentBend:
                    strcpy((char *)buffer, "bend");
                    break;
                case InstrumentSpecial:
                    // TODO:
                    strcpy((char *)buffer, "???");
                    break;
                case InstrumentDuty:
                    strcpy((char *)buffer, "duty");
                    break;
                case InstrumentDutyDelta:
                    strcpy((char *)buffer, "change in duty");
                    break;
                case InstrumentRandomize:
                    strcpy((char *)buffer, "randomize next cmd");
                    break;
                case InstrumentJump:
                    strcpy((char *)buffer, "jump to command");
                    break;
            }
            font_render_line_doubled(buffer, 102, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_instrument;
        }
        case 4:
            if ((instrument[editInstrument_instrument].cmd[editInstrument_cmd_index]&15) == InstrumentVibrato)
            {
                uint8_t msg[16];
                switch ((instrument[editInstrument_instrument].cmd[editInstrument_cmd_index]/16)/4) // rate
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
                msg[8] = hex_character[(instrument[editInstrument_instrument].cmd[editInstrument_cmd_index]/16)%4];
                msg[9] = 0;
                font_render_line_doubled(msg, 156, internal_line, 65535, BG_COLOR*257);
            }
            goto maybe_show_instrument;
        case 5:
            font_render_line_doubled((uint8_t *)"switch to:", 102 - 6*music_editor_in_menu, internal_line, 65535, BG_COLOR*257); 
            goto maybe_show_instrument;
        case 6:
            if (music_editor_in_menu)
            {
                font_render_line_doubled((uint8_t *)"L:prev instrument", 112, internal_line, 65535, BG_COLOR*257);
            }
            else
            {
                buffer[0] = 'L'; buffer[1] = ':';
                editInstrument_short_command_message(buffer+2, instrument[editInstrument_instrument].cmd[editInstrument_cmd_index]-1);
                font_render_line_doubled(buffer, 112, internal_line, 65535, BG_COLOR*257);
            }
            goto maybe_show_instrument;
        case 7:
            if (music_editor_in_menu)
            {
                font_render_line_doubled((uint8_t *)"R:next instrument", 112, internal_line, 65535, BG_COLOR*257);
            }
            else
            {
                buffer[0] = 'R'; buffer[1] = ':';
                editInstrument_short_command_message(buffer+2, instrument[editInstrument_instrument].cmd[editInstrument_cmd_index]+1);
                font_render_line_doubled(buffer, 112, internal_line, 65535, BG_COLOR*257);
            }
            goto maybe_show_instrument;
        case 8:
            font_render_line_doubled((uint8_t *)"dpad:", 102 - 6*music_editor_in_menu, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_instrument;
        case 9:
            font_render_line_doubled((uint8_t *)"adjust parameters", 112, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_instrument;
        case 11:
            if (music_editor_in_menu)
            {
                if (editInstrument_copying < 16)
                    font_render_line_doubled((uint8_t *)"A:cancel copy", 96, internal_line, 65535, BG_COLOR*257);
                else if (!editInstrument_bad)
                    font_render_line_doubled((uint8_t *)"A:save to file", 96, internal_line, 65535, BG_COLOR*257);
            }
            else
                font_render_line_doubled((uint8_t *)"X:cut cmd", 96, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_instrument;
        case 12:
            if (music_editor_in_menu)
            {
                if (editInstrument_copying < 16)
                    font_render_line_doubled((uint8_t *)"B/X:\"     \"", 96, internal_line, 65535, BG_COLOR*257);

                else
                    font_render_line_doubled((uint8_t *)"B:load from file", 96, internal_line, 65535, BG_COLOR*257);
            }
            else
                font_render_line_doubled((uint8_t *)"Y:insert cmd", 96, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_instrument;
        case 13:
            if (music_editor_in_menu)
            {
                if (editInstrument_copying < 16)
                    font_render_line_doubled((uint8_t *)"Y:paste", 96, internal_line, 65535, BG_COLOR*257);

                else if (!editInstrument_bad)
                    font_render_line_doubled((uint8_t *)"X:copy", 96, internal_line, 65535, BG_COLOR*257);
            }
            else
            {
                if (!editInstrument_bad)
                    font_render_line_doubled((uint8_t *)"A/B:play note", 96, internal_line, 65535, BG_COLOR*257);
            }
            goto maybe_show_instrument;
        case 15:
            if (music_editor_in_menu && !editInstrument_bad)
            {
                if (editInstrument_copying < 16)
                    goto maybe_show_instrument;
                strcpy((char *)buffer, "Y:file ");
                strcpy((char *)(buffer+7), (char *)base_song_filename);
                font_render_line_doubled(buffer, 96, internal_line, 65535, BG_COLOR*257);
            }
            goto maybe_show_instrument;
        case 17:
            if (music_editor_in_menu)
                font_render_line_doubled((uint8_t *)"start:edit instrument", 96, internal_line, 65535, BG_COLOR*257);
            else
                font_render_line_doubled((uint8_t *)"start:instrument menu", 96, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_instrument;
        case 18:
            if (music_editor_in_menu)
                font_render_line_doubled((uint8_t *)"select:song menu", 96, internal_line, 65535, BG_COLOR*257);
            else
                font_render_line_doubled((uint8_t *)"select:edit song", 96, internal_line, 65535, BG_COLOR*257);
            break;
        case 19:
            font_render_line_doubled(game_message, 36, internal_line, 65535, BG_COLOR*257);
            break;
        default:
          maybe_show_instrument:
            if (show_instrument)
                editInstrument_render_command(line-2, internal_line);
            break; 
    }
}

static inline void editInstrument_menu_controls()
{   // When in the menu, these are the controls
    if (GAMEPAD_PRESS(0, left) || GAMEPAD_PRESS(0, right))
    {
        editInstrument_cursor = 1 - editInstrument_cursor;
        return;
    }

    int moved = 0;
    if (GAMEPAD_PRESSING(0, up))
        ++moved;
    if (GAMEPAD_PRESSING(0, down))
        --moved;
    if (moved)
    {
        game_message[0] = 0;
        gamepad_press_wait = GAMEPAD_PRESS_WAIT;
        if (editInstrument_cursor) // drums
        {
            if (instrument[editInstrument_instrument].is_drum) 
                instrument[editInstrument_instrument].is_drum = 0;
            else
                instrument[editInstrument_instrument].is_drum = 1;
        }
        else
        {
            instrument[editInstrument_instrument].octave += moved;
            if (instrument[editInstrument_instrument].octave > 255)
                instrument[editInstrument_instrument].octave = 6;
            else if (instrument[editInstrument_instrument].octave > 6)
                instrument[editInstrument_instrument].octave = 0;
        }
    }

    io_event_t save_or_load = IoEventNone;
    if (GAMEPAD_PRESS(0, A))
    {   if (editInstrument_bad)
            return game_set_message_with_timeout("can't save bad jump.", MESSAGE_TIMEOUT);
        save_or_load = IoEventSave;
    }
    if (GAMEPAD_PRESS(0, B))
        save_or_load = IoEventLoad;
    if (save_or_load)
    {   if (editInstrument_copying < 16)
        {   // cancel the copy instead of saving
            editInstrument_copying = 16;
            return;
        }

        io_error_t error;
        if (save_or_load == IoEventSave)
            error = io_save_instrument(editInstrument_instrument);
        else
        {
            error = io_load_instrument(editInstrument_instrument);
            check_instrument();
        }
        io_message_from_error(game_message, error, save_or_load);
        return;
    }

    if (editInstrument_bad)
        return;

    if (GAMEPAD_PRESS(0, X))
    {
        game_message[0] = 0;
        // copy or uncopy
        if (editInstrument_copying < 16)
            editInstrument_copying = 16;
        else if (!editInstrument_bad)
            editInstrument_copying = editInstrument_instrument;
        return;
    }
    
    if (GAMEPAD_PRESS(0, Y))
    {   if (editInstrument_copying < 16)
        {   // paste
            if (editInstrument_instrument == editInstrument_copying)
            {   // don't try to overwrite the same data
                editInstrument_copying = 16;
                game_set_message_with_timeout("pasting to same thing", MESSAGE_TIMEOUT); 
                return;
            }
            uint8_t *src, *dst;
            src = &instrument[editInstrument_copying].cmd[0];
            dst = &instrument[editInstrument_instrument].cmd[0];
            instrument[editInstrument_instrument].octave = instrument[editInstrument_copying].octave;
            instrument[editInstrument_instrument].is_drum = instrument[editInstrument_copying].is_drum;
            memcpy(dst, src, MAX_INSTRUMENT_LENGTH);
            game_set_message_with_timeout("pasted.", MESSAGE_TIMEOUT); 
            editInstrument_copying = 16;
        }
        else
        {   // switch to choose name and hope to come back
            game_message[0] = 0;
            game_switch(ModeNameSong);
        }
        return;
    }

    moved = 0;
    if (GAMEPAD_PRESSING(0, R))
        ++moved;
    if (GAMEPAD_PRESSING(0, L))
        --moved;
    if (moved)
    {
        game_message[0] = 0;
        editInstrument_instrument = (editInstrument_instrument + moved)&15;
        editInstrument_cmd_index = 0;
        gamepad_press_wait = GAMEPAD_PRESS_WAIT*2;
        return;
    }
}

static inline void editInstrument_edit_controls()
{   // not in the menu, editing the instrument
    int movement = 0;
    if (GAMEPAD_PRESSING(0, down))
    {
        if (!instrument[editInstrument_instrument].is_drum)
        {
            if (editInstrument_cmd_index < MAX_INSTRUMENT_LENGTH-1 &&
                instrument[editInstrument_instrument].cmd[editInstrument_cmd_index])
                ++editInstrument_cmd_index;
            else
                editInstrument_cmd_index = 0;
        }
        else
        {
            int next_j;
            if (editInstrument_cmd_index < 2*DRUM_SECTION_LENGTH)
                next_j = 2*DRUM_SECTION_LENGTH;
            else if (editInstrument_cmd_index < 3*DRUM_SECTION_LENGTH)
                next_j = 3*DRUM_SECTION_LENGTH;
            else
                next_j = 0;

            if (editInstrument_cmd_index < MAX_INSTRUMENT_LENGTH-1)
            {
                if (instrument[editInstrument_instrument].cmd[editInstrument_cmd_index] == InstrumentBreak)
                    editInstrument_cmd_index = next_j;
                else
                    ++editInstrument_cmd_index;
            }
            else
                editInstrument_cmd_index = 0;
        }
        movement = 1;
    }
    if (GAMEPAD_PRESSING(0, up))
    {
        if (!instrument[editInstrument_instrument].is_drum)
        {
            if (editInstrument_cmd_index)
                --editInstrument_cmd_index;
            else
            {
                while (editInstrument_cmd_index < MAX_INSTRUMENT_LENGTH-1 && 
                    instrument[editInstrument_instrument].cmd[editInstrument_cmd_index] != InstrumentBreak)
                    ++editInstrument_cmd_index;
            }
        }
        else 
        {
            int move_here_then_up = -1;
            int but_no_further_than;
            switch (editInstrument_cmd_index)
            {
                case 0:
                    move_here_then_up = 3*DRUM_SECTION_LENGTH; 
                    but_no_further_than = 4*DRUM_SECTION_LENGTH;
                    break;
                case 2*DRUM_SECTION_LENGTH:
                    move_here_then_up = 0; 
                    but_no_further_than = 2*DRUM_SECTION_LENGTH;
                    break;
                case 3*DRUM_SECTION_LENGTH:
                    move_here_then_up = 2*DRUM_SECTION_LENGTH; 
                    but_no_further_than = 3*DRUM_SECTION_LENGTH;
                    break;
                default:
                    --editInstrument_cmd_index;
            }
            if (move_here_then_up >= 0)
            {
                editInstrument_cmd_index = move_here_then_up;
                while (editInstrument_cmd_index < but_no_further_than-1 && 
                    instrument[editInstrument_instrument].cmd[editInstrument_cmd_index] != InstrumentBreak)
                {
                    ++editInstrument_cmd_index;
                }
            }
        }
        movement = 1;
    }
    if (GAMEPAD_PRESSING(0, left))
    {
        editInstrument_adjust_parameter(-1);
        movement = 1;
    }
    if (GAMEPAD_PRESSING(0, right))
    {
        editInstrument_adjust_parameter(+1);
        movement = 1;
    }
    if (movement)
    {
        gamepad_press_wait = GAMEPAD_PRESS_WAIT;
        return;
    }

    if (GAMEPAD_PRESS(0, X))
    {
        // delete
        editInstrument_command_copy = instrument[editInstrument_instrument].cmd[editInstrument_cmd_index]; // cut
        if (!instrument[editInstrument_instrument].is_drum)
        {
            for (int j=editInstrument_cmd_index; j<MAX_INSTRUMENT_LENGTH-1; ++j)
            {
                if ((instrument[editInstrument_instrument].cmd[j] = instrument[editInstrument_instrument].cmd[j+1]) == 0)
                    break;
            }
            instrument[editInstrument_instrument].cmd[MAX_INSTRUMENT_LENGTH-1] = InstrumentBreak;
        }
        else
        {
            int max_j;
            if (editInstrument_cmd_index < 2*DRUM_SECTION_LENGTH)
                max_j = 2*DRUM_SECTION_LENGTH;
            else if (editInstrument_cmd_index < 3*DRUM_SECTION_LENGTH)
                max_j = 3*DRUM_SECTION_LENGTH;
            else
                max_j = 4*DRUM_SECTION_LENGTH;
            
            for (int j=editInstrument_cmd_index; j<max_j-1; ++j)
            {
                if ((instrument[editInstrument_instrument].cmd[j] = instrument[editInstrument_instrument].cmd[j+1]) == 0)
                    break;
            }
            instrument[editInstrument_instrument].cmd[max_j-1] = InstrumentBreak;
        }
        check_instrument();
        return;
    }

    if (GAMEPAD_PRESS(0, Y))
    {
        // insert
        if (!instrument[editInstrument_instrument].is_drum)
        {
            if ((instrument[editInstrument_instrument].cmd[MAX_INSTRUMENT_LENGTH-1]&15) != InstrumentBreak)
                return game_set_message_with_timeout("list full, can't insert.", MESSAGE_TIMEOUT); 
            for (int j=MAX_INSTRUMENT_LENGTH-1; j>editInstrument_cmd_index; --j)
                instrument[editInstrument_instrument].cmd[j] = instrument[editInstrument_instrument].cmd[j-1];
        }
        else
        {
            int next_j;
            if (editInstrument_cmd_index < 2*DRUM_SECTION_LENGTH)
                next_j = 2*DRUM_SECTION_LENGTH;
            else if (editInstrument_cmd_index < 3*DRUM_SECTION_LENGTH)
                next_j = 3*DRUM_SECTION_LENGTH;
            else
                next_j = MAX_INSTRUMENT_LENGTH;
            if ((instrument[editInstrument_instrument].cmd[next_j-1]&15) != InstrumentBreak)
                return game_set_message_with_timeout("list full, can't insert.", MESSAGE_TIMEOUT); 

            for (int j=next_j-1; j>editInstrument_cmd_index; --j)
                instrument[editInstrument_instrument].cmd[j] = instrument[editInstrument_instrument].cmd[j-1];
        }
        instrument[editInstrument_instrument].cmd[editInstrument_cmd_index] = editInstrument_command_copy;
        check_instrument();
        return;
    }

    if (GAMEPAD_PRESS(0, L))
    {
        uint8_t *cmd = &instrument[editInstrument_instrument].cmd[editInstrument_cmd_index];
        *cmd = ((*cmd - 1)&15) | ((*cmd)&240);
        check_instrument();
    }
    if (GAMEPAD_PRESS(0, R))
    {
        uint8_t *cmd = &instrument[editInstrument_instrument].cmd[editInstrument_cmd_index];
        *cmd = ((*cmd + 1)&15) | ((*cmd)&240);
        check_instrument();
    }
    
    if (editInstrument_bad) // can't do anything else until you fix this
        return;

    if (GAMEPAD_PRESS(0, A))
    {   // play a note and ascend
        game_message[0] = 0;
        editInstrument_note = (editInstrument_note + 1)%24;
        chip_reset_player(editTrack_player);
        chip_player[editTrack_player].octave = instrument[editInstrument_instrument].octave;
        chip_play_note(editTrack_player, editInstrument_instrument, editInstrument_note, 240); 
    }
    
    if (GAMEPAD_PRESS(0, B))
    {   // play a note and descend
        game_message[0] = 0;
        if (--editInstrument_note > 23)
            editInstrument_note = 23;
        chip_reset_player(editTrack_player);
        chip_player[editTrack_player].octave = instrument[editInstrument_instrument].octave;
        chip_play_note(editTrack_player, editInstrument_instrument, editInstrument_note, 240); 
    }
}

void editInstrument_controls()
{
    if (GAMEPAD_PRESS(0, start))
    {
        if (game_message[0] == 's' || game_message[0] == 'l')
            game_message[0] = 0;
        music_editor_in_menu = 1 - music_editor_in_menu; 
        editInstrument_copying = 16;
        chip_kill();
        return;
    }

    if (music_editor_in_menu)
        editInstrument_menu_controls();
    else
        editInstrument_edit_controls();

    // Don't allow escaping if the instrument sequence is invalid:
    if (editInstrument_bad)
        return;

    if (GAMEPAD_PRESS(0, select))
    {   // Switch to editing the song, cancel any copy
        editInstrument_copying = 16;
        game_message[0] = 0;
        game_switch(ModeEditSong);
    } 
}

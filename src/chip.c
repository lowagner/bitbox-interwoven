/* Simple soundengine for the BitBox, modified
 * Copyright 2016, Lucas Wagner <lowagner@gmail.com>
 * Copyright 2015, Makapuf <makapuf2@gmail.com>
 * Copyright 2014, Adrien Destugues <pulkomandy@pulkomandy.tk>
 * Copyright 2007, Linus Akesson
 * Based on the "Hardware Chiptune" project
 *
 * This main file is a player for the packed music format used in the original
 * "hardware chiptune"
 * http://www.linusakesson.net/hardware/chiptune.php
 * There is a tracker for this but the format here is slightly different (mostly
 * because of the different replay rate - 32KHz instead of 16KHz).
 *
 * Because of this the sound in the tracker will be a bit different, but it can
 * easily be tweaked. This version has a somewhat bigger but much simplified song format.
 */
#include "bitbox.h"
#include "chip.h"

#include <stdint.h>
#include <stdlib.h>

chip_playing_t chip_playing CCM_MEMORY;
uint8_t chip_repeat CCM_MEMORY;
uint8_t chip_volume CCM_MEMORY;

// the structs that get updated multiple times a frame,
// to generate the sound samples.
struct oscillator oscillator[CHIP_PLAYERS] CCM_MEMORY;

// you can have up to 16 instruments, which need to be played to modify the oscillators.
struct instrument instrument[16] CCM_MEMORY;

// a track corresponds to a single melody or single harmony, which has instructions on which instruments to play.
uint8_t track_length CCM_MEMORY;
uint8_t chip_track[16][CHIP_PLAYERS][MAX_TRACK_LENGTH] CCM_MEMORY;
// Current absolute position since starting to play the track; shared between all players,
// even if their current track command index is different (e.g. due to jumps).
uint8_t track_pos CCM_MEMORY;

// the players combine commands from tracks and instruments to update the oscillators.
struct chip_player chip_player[CHIP_PLAYERS] CCM_MEMORY;
// instruments can be allowed to finish a note sometimes.
// tied in with creating a note randomly for sfx; maybe we should store more data on old oscillator state to restore.
uint8_t chip_instrument_for_next_note_for_player[CHIP_PLAYERS] CCM_MEMORY;

// a song is made out of a sequence of "measures".
// for each "measure" in the song, you specify which track each player will play;
// there are 16 tracks possible, so each measure takes up 16 bits (4 bits for specifying the track per player).
uint16_t chip_song[MAX_SONG_LENGTH]; // a nibble for the track to play for each player/channel
uint8_t song_wait CCM_MEMORY; // >0 means wait N frames, 0 means play now. 
uint8_t song_speed CCM_MEMORY;
uint8_t song_pos CCM_MEMORY;
uint8_t song_length CCM_MEMORY; // capped at MAX_SONG_LENGTH
uint8_t song_transpose CCM_MEMORY;

// At each sample the phase is incremented by frequency/4. It is then used to
// compute the output of the oscillator depending on the waveform.
// This means the frequency unit is 65536*4/31000 or about 8.456Hz
// and the frequency range is 0 to 554180Hz. Maybe it would be better to adjust
// the scaling factor to allow less high frequencies (they are useless) but
// more fine grained resolution. Not only we could play notes more in tune,
// but also we would get a more subtle vibrato effect.

// ... and that's it for the engine, which is very simple as you see.
// The parameters for the oscillators can be updated in your game_frame callback.
// Since the audio buffer is generated in one go it is useless to try to tweak
// the parameters more often than that.

static const uint16_t freq_table[MAX_NOTE] = {
    0x010b, 0x011b, 0x012c, 0x013e, 0x0151, 0x0165, 0x017a, 0x0191, 0x01a9,
    0x01c2, 0x01dd, 0x01f9, 0x0217, 0x0237, 0x0259, 0x027d, 0x02a3, 0x02cb,
    0x02f5, 0x0322, 0x0352, 0x0385, 0x03ba, 0x03f3, 0x042f, 0x046f, 0x04b2,
    0x04fa, 0x0546, 0x0596, 0x05eb, 0x0645, 0x06a5, 0x070a, 0x0775, 0x07e6,
    0x085f, 0x08de, 0x0965, 0x09f4, 0x0a8c, 0x0b2c, 0x0bd6, 0x0c8b, 0x0d4a,
    0x0e14, 0x0eea, 0x0fcd, 0x10be, 0x11bd, 0x12cb, 0x13e9, 0x1518, 0x1659,
    0x17ad, 0x1916, 0x1a94, 0x1c28, 0x1dd5, 0x1f9b, 0x217c, 0x237a, 0x2596,
    0x27d3, 0x2a31, 0x2cb3, 0x2f5b, 0x322c, 0x3528, 0x3851, 0x3bab, 0x3f37,
    0x42f9, 0x46f5, 0x4b2d, 0x4fa6, 0x5462, 0x5967, 0x5eb7, 0x6459, 0x6a51,
    0x70a3, 0x7756, 0x7e6f
}; // 84 long.

static const int8_t sine_table[] = { // 16 values per row, 64 total
    0, 12, 25, 37, 49, 60, 71, 81, 90, 98, 106, 112, 117, 122, 125, 126,
    127, 126, 125, 122, 117, 112, 106, 98, 90, 81, 71, 60, 49, 37, 25, 12,
    0, -12, -25, -37, -49, -60, -71, -81, -90, -98, -106, -112, -117, -122, -125, -126,
    -127, -126, -125, -122, -117, -112, -106, -98, -90, -81, -71, -60, -49, -37, -25, -12,
};

// Assumes phase >= duty, updates duty to be the difference to U16_MAX and enforces phase < duty
#define REPHASE16(phase, duty) \
    phase -= duty; \
    duty = 65535 - duty

uint8_t instrument_max_index(uint8_t i, uint8_t j)
{   // Returns the max index we shouldn't run (instrument command-wise) based on the current index.
    if (instrument[i].is_drum)
    {   // Drums are split up into multiple sections so multiple noises can be made with one instrument.
        if (j < 2*DRUM_SECTION_LENGTH)
            return 2*DRUM_SECTION_LENGTH;
        else if (j < 3*DRUM_SECTION_LENGTH)
            return 3*DRUM_SECTION_LENGTH;
    }
    return MAX_INSTRUMENT_LENGTH;
}

int instrument_jump_bad(uint8_t inst, uint8_t max_index, uint8_t jump_from_index, uint8_t j)
{   // Check if this is an ok jump; if the jump command hits itself again before a wait,
    // we'll get stuck in an infinite loop.
    for (int k=0; k<20; ++k)
    {   // Test going through the commands.
        if (j >= max_index) // it's ok to jump out of the commands, this will be interpreted as stopping
            return 0;
        else if (j == jump_from_index) // returning to the same spot is out of the question
            return 1;
        switch (instrument[inst].cmd[j]&15)
        {   // Update instrument position j based on command:
            case InstrumentJump:
                j = instrument[inst].cmd[j]>>4;
                break;
            case InstrumentWait:
                // We found a wait, this jump is ok.
                return 0;
            case InstrumentBreak:
                if ((instrument[inst].cmd[j]>>4) == 0)
                    return 0;
            default:
                ++j;
        }
    }
    // do not proceed if no wait was found:
    return 1;
}

int track_jump_bad(uint8_t t, uint8_t i, uint8_t jump_from_index, uint8_t j)
{   // Check if this is an ok jump; if the jump command hits itself again before a wait,
    // we'll get stuck in an infinite loop.
    for (int k=0; k<36; ++k)
    {   // Test going through the commands.
        if (j >= MAX_TRACK_LENGTH) // it's ok to jump out of the commands, this will be interpreted as stopping
            return 0;
        else if (j == jump_from_index) // returning to the same spot is out of the question
            return 1;
        switch (chip_track[t][i][j]&15)
        {   // Update track position j based on command:
            case TRACK_JUMP:
                j = 2*(chip_track[t][i][j]>>4);
                break;
            case TRACK_WAIT:
            case TRACK_NOTE_WAIT:
                // We found a wait, this jump is ok.
                return 0;
            case TRACK_BREAK:
                if ((chip_track[t][i][j]>>4) == 0)
                    return 0;
            default:
                ++j;
        }
    }
    // do not proceed if no wait was found:
    return 1;
}

static uint8_t randomize(uint8_t arg)
{   // Returns a random number between 0 and 15
    switch (arg)
    {   // The argument specifies what kind of randomization is desired.
        case 0:
            return rand()%16;
        case 1:
            return 1 + (rand()%8)*2;
        case 2:
            return (rand()%8)*2;
        case 3:
            return 1 + 7*(rand()%3);
        case 4:
            return rand()%8;
        case 5:
            return 4 + rand()%8;
        case 6:
            return 8 + rand()%8;
        case 7:
            return (12 + rand()%8)&15;
        case 8:
            return rand()%4;
        case 9:
            return 4+rand()%4;
        case 10:
            return 8+rand()%4;
        case 11:
            return 12+rand()%4;
        case 12:
            return 4*(rand()%4);
        case 13:
            return 1 + 4*(rand()%4);
        case 14:
            return 2 + 4*(rand()%4);
        case 15:
            return 3 + 4*(rand()%4);
    }
    return 0;
}

static void instrument_run_command(uint8_t i, uint8_t inst, uint8_t cmd) 
{   // Have player i update their oscillator based on running a command on the instrument
    uint8_t param = cmd >> 4;
    switch (cmd&15) 
    {   // The lower nibble of cmd determines what to do, and the upper nibble is the param:
        case InstrumentBreak:
            if (!param || track_pos < param*4)
                chip_player[i].cmd_index = MAX_INSTRUMENT_LENGTH; // end instrument commmands
            break;
        case InstrumentPan: // p = switch side (L/R) for full volume
            // 1 = left side, 8 = both sides, 15 = right side
            // 0 also implies both sides full volume as well.
            oscillator[i].pan = param;
            break;
        case InstrumentVolume: // v = volume
            chip_player[i].volume = param*17;
            break;
        case InstrumentWaveform: // w = select waveform
            oscillator[i].waveform = param;
            break;
        case InstrumentNote: // C-Eb = set relative note
            chip_player[i].note = (param + chip_player[i].track_note + song_transpose)%MAX_NOTE;
            break;
        case InstrumentWait: // t = timing 
            if (param)
                chip_player[i].wait = param;
            else
                chip_player[i].wait = 16;
            break;
        case InstrumentFadeMagnitude: // > = fade amplitude
            // normally starts a decrescendo, but depends on InstrumentFadeBehavior.
            chip_player[i].fade_saved_max_volume = 255;
            chip_player[i].fade_saved_min_volume = 0;
            param = param + param*param/15;
            if (chip_player[i].fade_behavior % 2 == InstrumentFadeNegativeVolumeD)
                chip_player[i].volumed = -param;
            else
                chip_player[i].volumed = param;
            break;
        case InstrumentFadeBehavior: // < = fade behavior, can flip a decrescendo to a crescendo, etc.
            chip_player[i].fade_behavior = param;
            int8_t volumed = chip_player[i].volumed;
            if (param % 2 == InstrumentFadeNegativeVolumeD)
                chip_player[i].volumed = volumed <= 0 ? volumed : -volumed;
            else
                chip_player[i].volumed = volumed < 0 ? -volumed : volumed;
            // TODO: maybe want to set these differently for various behaviors, but that complicates things...
            chip_player[i].fade_saved_max_volume = 255;
            chip_player[i].fade_saved_min_volume = 0;
            break;
        case InstrumentInertia: // i = inertia (auto note slides)
            chip_player[i].inertia = param;
            break;
        case InstrumentVibrato: // ~ = vibrato depth and rate
            chip_player[i].vibrato_depth = (param&3)*3 + (param>3)*2;
            chip_player[i].vibrato_rate = 2 + (param & 12)/2;
            break;
        case InstrumentBend: // / or \ = increases or drops frequency
            if (param < 8)
                chip_player[i].bendd = param + param*param/4;
            else
                chip_player[i].bendd = -(16-param) - (16-param)*(16-param)/4;
            break;
        case InstrumentSpecial: // s = special
            // TODO
            break;
        case InstrumentDuty: // d = duty cycle.  param==0 makes for a square wave if waveform is WfPulse
            oscillator[i].duty = 32768 + (param << 11);
            break;
        case InstrumentDutyDelta: // m = duty variation
            chip_player[i].dutyd = (param + 1 + param/8) * param;
            break;
        case InstrumentRandomize:
        {   // Randomize the next command's param in the list based on this command:
            uint8_t next_index = chip_player[i].cmd_index;
            uint8_t max_index = instrument_max_index(inst, next_index-1);
            if (next_index >= max_index)
                break;
            uint8_t random = randomize(param);
            uint8_t next_command_type = instrument[inst].cmd[next_index] & 15;
            if (next_command_type == InstrumentJump)
            {   // don't allow a randomized jump to cause an infinite loop:
                if (instrument_jump_bad(inst, max_index, next_index, random))
                    break; // do not continue, do not allow this number as a jump
            }
            else if (next_command_type == InstrumentWaveform)
            {   // TODO: until we add more waveforms, make sure we get something not silent:
                random %= 8;
                if (random == 7)
                    random = 4 + rand()%3;
            }
            instrument[inst].cmd[next_index] = next_command_type | (random<<4);
            break;
        }
        case InstrumentJump:
            chip_player[i].cmd_index = param;
            break;
    }
}

void chip_init()
{   // initialize things (only happens once).
    chip_volume = 128;
    song_length = 16;
    track_length = 32;
    song_speed = 4;
}

void chip_reset_player(int i)
{   // resets player i to a clean slate
    chip_instrument_for_next_note_for_player[i] = i;
    chip_player[i].instrument = i;
    chip_player[i].cmd_index = 0;
    chip_player[i].track_cmd_index = 0;
    chip_player[i].track_wait = 0;
    chip_player[i].volume = 0;
    chip_player[i].volumed = 0;
    chip_player[i].fade_behavior = 0;
    chip_player[i].fade_saved_max_volume = 255;
    chip_player[i].fade_saved_min_volume = 0;
    chip_player[i].track_volume = 0;
    chip_player[i].track_volumed = 0;
    chip_player[i].track_inertia = 0;
    chip_player[i].track_vibrato_rate = 0;
    chip_player[i].track_vibrato_depth = 0;
    chip_player[i].bendd = 0;
    chip_player[i].octave = instrument[i].octave;
}

void chip_kill()
{   // Stops playing all sounds
    chip_playing = PlayingNone;
    for (int i=0; i<CHIP_PLAYERS; ++i)
    {   // Turn off all oscillators for good measure
        oscillator[i].volume = 0;
        chip_player[i].track_volume = 0;
        chip_player[i].track_volumed = 0;
    }
}

void chip_play_song(int pos) 
{   // Start playing a song from the provided position.
    if (pos == 0)
    {   // Reset to song defaults (let the tracks update as necessary):
        track_length = 32;
        song_speed = 4;
        song_transpose = 0;
    }

    song_wait = 0;
    track_pos = 0;
    chip_playing = PlayingSong;
    song_pos = pos % song_length;
    
    for (int i=0; i<CHIP_PLAYERS; ++i)
        chip_reset_player(i);

    uint16_t tracks = chip_song[song_pos];
    chip_player[0].track_index = tracks & 15;
    chip_player[1].track_index = (tracks >> 4) & 15;
    chip_player[2].track_index = (tracks >> 8) & 15;
    chip_player[3].track_index = tracks >> 12;

    tracks = chip_song[(song_pos+1)%song_length];
    chip_player[0].next_track_index = tracks & 15;
    chip_player[1].next_track_index = (tracks >> 4) & 15;
    chip_player[2].next_track_index = (tracks >> 8) & 15;
    chip_player[3].next_track_index = tracks >> 12;
}

void chip_play_track(int track)
{   // Makes all players play the passed-in track.
    song_wait = 0;
    track_pos = 0;
    chip_playing = PlayingTrack;

    track &= 15;
    for (int i=0; i<CHIP_PLAYERS; ++i)
    {   // Make the players loop on this track.
        chip_reset_player(i);
        chip_player[i].next_track_index = track;
        chip_player[i].track_index = track;
    }
}

static void chip_play_note_internal(uint8_t p, uint8_t note)
{   // For player p, plays a note, for internal use only.  see `chip_play_note` for externally allowed usage.
    #ifdef DEBUG_CHIPTUNE
    message("note %d on player %d\n", (note+12*chip_player[p].octave), p);
    #endif
    chip_player[p].instrument = chip_instrument_for_next_note_for_player[p];
    // now set some defaults and the command index
    if (instrument[chip_player[p].instrument].is_drum)
    {   // a drum instrument has 3 sub instruments.
        note %= 12;
        if (note < 10)
        {   // first subinstrument is 2*DRUM_SECTION_LENGTH commands long, and takes up first 10 notes.
            chip_player[p].cmd_index = 0;
            chip_player[p].max_drum_index = 2*DRUM_SECTION_LENGTH;
        }
        else if (note == 10)
        {   // second instrument takes up DRUM_SECTION_LENGTH commands
            chip_player[p].cmd_index = 2*DRUM_SECTION_LENGTH;
            chip_player[p].max_drum_index = 3*DRUM_SECTION_LENGTH;
        }
        else
        {   // third instrument takes up DRUM_SECTION_LENGTH commands
            chip_player[p].cmd_index = 3*DRUM_SECTION_LENGTH;
            chip_player[p].max_drum_index = 4*DRUM_SECTION_LENGTH;
        }
        oscillator[p].waveform = WfNoise; // by default
        chip_player[p].volume = 255; 
    }
    else
    {   // Normal instrument
        chip_player[p].cmd_index = 0;
        oscillator[p].waveform = WfTriangle; // by default
        chip_player[p].volume = 14*17; 
    }
    chip_player[p].track_note = note + chip_player[p].octave*12;
    chip_player[p].volumed = 0;
    chip_player[p].fade_behavior = 0;
    chip_player[p].fade_saved_max_volume = 255;
    chip_player[p].fade_saved_min_volume = 0;
    chip_player[p].inertia = 0;
    chip_player[p].wait = 0;
    chip_player[p].dutyd = 0;
    chip_player[p].vibrato_depth = 0;
    chip_player[p].vibrato_rate = 1;
    chip_player[p].bend = 0;
    chip_player[p].bendd = 0;
    oscillator[p].pan = 8; // default to output both L/R
    oscillator[p].duty = 0x8000; // default to square wave
}

void chip_play_note(uint8_t p, uint8_t inst, uint8_t note, uint8_t volume)
{   // for player `p`, using instrument `inst`, plays a note with some volume
    uint8_t old_instrument = chip_instrument_for_next_note_for_player[p];
    chip_instrument_for_next_note_for_player[p] = inst;
    chip_play_note_internal(p, note);
    chip_player[p].track_volume = volume;
    chip_player[p].track_volumed = 0;
    chip_instrument_for_next_note_for_player[p] = old_instrument;
}

static void track_run_command(uint8_t i, uint8_t cmd) 
{   // Run a command for player i on their track.
    uint8_t param = cmd >> 4;
    switch(cmd&15) 
    {   // Command type is in lower nibble, param was from upper nibble.
        case TRACK_BREAK: // f = wait til a given quarter note, or break if passed that.
            if (4*param > track_pos)
                chip_player[i].track_wait = 4*param - track_pos;
            else
                chip_player[i].track_cmd_index = MAX_TRACK_LENGTH; // end track commmands
            break;
        case TRACK_OCTAVE: // O = octave, or + or - for relative octave
            if (param < 7)
                chip_player[i].octave = param;
            else if (param == 7)
            {   // use instrument default octave
                chip_player[i].octave = instrument[chip_instrument_for_next_note_for_player[i]].octave;
            }
            else if (param < 12)
            {   // add a delta to current octave
                chip_player[i].octave += (param-6)/2;
                if (chip_player[i].octave > 6)
                {   // keep octave in bounds...
                    if (param%2) // additive, no wrap around
                        chip_player[i].octave = 6;
                    else // wrap around
                        chip_player[i].octave %= 7;
                }
            }
            else
            {   // remove a delta from current octave
                uint8_t delta = (17-param)/2;
                if (delta > chip_player[i].octave)
                {   // keep octave in bounds...
                    if (param%2) // wrap around
                        chip_player[i].octave = 7 - delta + chip_player[i].octave;
                    else // subtractive only, no wrap around
                        chip_player[i].octave = 0;
                }
                else
                    chip_player[i].octave -= delta;
            }
            break;
        case TRACK_INSTRUMENT:
            chip_instrument_for_next_note_for_player[i] = param;
            break;
        case TRACK_VOLUME: // v = volume
            chip_player[i].track_volume = param*17;
            break;
        case TRACK_NOTE: // 
            chip_play_note_internal(i, param);
            break;
        case TRACK_WAIT: // w = wait 
            if (param)
                chip_player[i].track_wait = param;
            else
                chip_player[i].track_wait = 16;
            break;
        case TRACK_NOTE_WAIT:
        {   // hit a note relative to previous, and wait based on parameter
            // lower 2 bits indicate how long to wait:
            chip_player[i].track_wait = (param&3)+1;
            uint8_t old_note = chip_player[i].track_note;
            chip_play_note_internal(i, old_note);
            switch (param >> 2)
            {   // upper 2 bits indicate which note to play
                case 0:
                    chip_player[i].track_note = old_note;
                    break;
                case 1:
                    chip_player[i].track_note = old_note+1;
                    if (chip_player[i].track_note >= MAX_NOTE)
                        chip_player[i].track_note -= 12;
                    break;
                case 2:
                    chip_player[i].track_note = old_note+2;
                    if (chip_player[i].track_note >= MAX_NOTE)
                        chip_player[i].track_note -= 12;
                    break;
                case 3:
                    if (old_note)
                        chip_player[i].track_note = old_note-1;
                    else
                        chip_player[i].track_note = old_note+11;
                    break;
            }
            break;
        }
        case TRACK_FADE_IN: // < = fade in, or crescendo
            chip_player[i].track_volumed = param + param*param/15;
            break;
        case TRACK_FADE_OUT: // > = fade out, or decrescendo
            if (!param)
                param = 16;
            chip_player[i].track_volumed = -param - param*param/15;
            break;
        case TRACK_INERTIA: // i = inertia (auto note slides)
            chip_player[i].track_inertia = param;
            break;
        case TRACK_VIBRATO: // ~ = vibrato depth
            chip_player[i].track_vibrato_depth = (param&3)*3 + (param>3)*2;
            chip_player[i].track_vibrato_rate = 1 + (param & 12)/2;
            break;
        case TRACK_TRANSPOSE: // T = global transpose
            if (param == 0) // reset song transpose
                song_transpose = 0;
            else
                song_transpose = (song_transpose+param)%12;
            break;
        case TRACK_SPEED: // 
            song_speed = 16 - param;
            break;
        case TRACK_LENGTH: // M - measure length, in quarter notes
            param = param ? param : 16;
            track_length = 4 * param;
            break;
        case TRACK_RANDOMIZE:
        {
            uint8_t next_index = chip_player[i].track_cmd_index;
            if (next_index >= MAX_TRACK_LENGTH)
                break;
            uint8_t random = randomize(param);
            uint8_t t = chip_player[i].track_index;
            if ((chip_track[t][i][next_index]&15) == TRACK_JUMP && 
                track_jump_bad(t, i, next_index, 2*random))
                break;
            chip_track[t][i][next_index] = 
                (chip_track[t][i][next_index]&15) | (random<<4);
            break;
        }
        case TRACK_JUMP: // 
            chip_player[i].track_cmd_index = 2*param;
            break;
    }
}

static inline void chip_update_players_for_track()
{   // Runs updates on players based on track commands
    #ifdef DEBUG_CHIPTUNE
    message("%02d", track_pos);
    #endif

    for (int i=0; i<CHIP_PLAYERS; ++i) 
    {
        #ifdef DEBUG_CHIPTUNE
        message(" | t: %02d/%02d) ", i, chip_player[i].track_cmd_index, track_pos);
        #endif
        while (!chip_player[i].track_wait && chip_player[i].track_cmd_index < MAX_TRACK_LENGTH) 
            track_run_command(i, chip_track[chip_player[i].track_index][i][chip_player[i].track_cmd_index++]);

        if (chip_player[i].track_wait)
            --chip_player[i].track_wait;
    }

    #ifdef DEBUG_CHIPTUNE
    message("\n");
    #endif

    if (++track_pos == track_length)
    {
        for (int i=0; i<CHIP_PLAYERS; ++i)
        {
            chip_player[i].track_cmd_index = 0;
            chip_player[i].track_index = chip_player[i].next_track_index;
            chip_player[i].track_wait = 0;
        }
        track_pos = 0;
    }
}

static inline void chip_update_players_for_song()
{   // Called if user is playing a song
    if (!track_pos) // == 0.  load the next track.
    {
        if (song_pos >= song_length) 
        {
            if (chip_repeat)
                song_pos = 0;
            else
            {
                chip_playing = 0;
                return;
            }
        } 
        #ifdef DEBUG_CHIPTUNE
        message("Now at position %d of song\n", song_pos);
        #endif
       
        song_pos = (song_pos+1)%song_length;
        uint16_t tracks = chip_song[song_pos];
        chip_player[0].next_track_index = tracks & 15;
        chip_player[1].next_track_index = (tracks >> 4) & 15;
        chip_player[2].next_track_index = (tracks >> 8) & 15;
        chip_player[3].next_track_index = tracks >> 12;
    }
    
    chip_update_players_for_track();
}

static inline void chip_update_players()
{   // Updates players if necessary based on the song or track that is playing.
    if (song_wait)
    {   // Don't update the players just yet
        --song_wait;
        return;
    }

    song_wait = song_speed;
    switch (chip_playing)
    {   // Check if we need to update players here:
        case PlayingSong:
            chip_update_players_for_song();
            break;
        case PlayingTrack:
            chip_update_players_for_track();
            break;
        default:
            break;
    }
}

static void chip_update_oscillators()
{   // Updates oscillators based on players' instrument commands
    for (int i=0; i<CHIP_PLAYERS; ++i) 
    {   // Run through all oscillators...

        if (!chip_player[i].track_volume && chip_player[i].track_volumed <= 0)
        {   // Short-circuit quiet tracks
            oscillator[i].volume = 0;
            continue;
        }
        int16_t vol;
        uint16_t inertia_slide;
        int16_t inertia;
        uint8_t inst = chip_player[i].instrument;
    
        if (instrument[inst].is_drum)
        {   // Run drum commands (with smaller instrument sequence):
            // We can get a command to set wait to nonzero, so make sure we escape in that case:
            while (!chip_player[i].wait && chip_player[i].cmd_index < chip_player[i].max_drum_index)
                instrument_run_command(i, inst, instrument[inst].cmd[chip_player[i].cmd_index++]);
        }
        else
        {   // Run instrument commands, similarly awaiting a wait to escape:
            while (!chip_player[i].wait && chip_player[i].cmd_index < MAX_INSTRUMENT_LENGTH) 
                instrument_run_command(i, inst, instrument[inst].cmd[chip_player[i].cmd_index++]);
        }

        if (chip_player[i].wait)
            --chip_player[i].wait;
        
        // calculate instrument frequency
        if (chip_player[i].inertia || chip_player[i].track_inertia)
        {   // if sliding around
            inertia = 1024/(chip_player[i].inertia + chip_player[i].track_inertia);
            inertia_slide = chip_player[i].inertia_slide;
            int16_t diff = freq_table[chip_player[i].note] - inertia_slide;
            if (diff > inertia) 
                diff = inertia;
            else if (diff < -inertia) 
                diff = -inertia;
            inertia_slide += diff;
            chip_player[i].inertia_slide = inertia_slide;
        } 
        else 
        {   // no slide
            inertia_slide = freq_table[chip_player[i].note];
        }
        oscillator[i].freq = inertia_slide + chip_player[i].bend + ((
            (chip_player[i].vibrato_depth + chip_player[i].track_vibrato_depth) * 
                sine_table[chip_player[i].vibrato_phase & 63]
        ) >> 2);
        chip_player[i].vibrato_phase += chip_player[i].vibrato_rate + chip_player[i].track_vibrato_rate;
        chip_player[i].bend += chip_player[i].bendd;

        vol = chip_player[i].volume + chip_player[i].volumed;
        int16_t max_vol = chip_player[i].fade_saved_max_volume;
        int16_t min_vol = chip_player[i].fade_saved_min_volume;

        if (vol > max_vol)
        switch (chip_player[i].fade_behavior/2)
        {   case InstrumentFadeClamp:
                vol = max_vol;
                break;
            case InstrumentFadeReverseClamp:
                vol = chip_player[i].fade_saved_min_volume;
                chip_player[i].volumed = 0;
                break;
            case InstrumentFadePingPong:
            case InstrumentFadePingPongRaisingVolume:
                // Update min volume for increasing volume at the bottom of the ping/pong.
                if (chip_player[i].volumed > 0)
                {   // Only update delta volume at top:
                    vol = max_vol;
                    chip_player[i].volumed *= -1;
                }
                else
                    ASSERT(vol < 256);
                break;
            case InstrumentFadePingPongLoweringVolume:
                if (chip_player[i].volumed > 0)
                {   // Update delta volume at top with max volume:
                    vol = max_vol;
                    chip_player[i].volumed *= -1;
                    // Decrease max volume:
                    max_vol += chip_player[i].volumed;
                    chip_player[i].fade_saved_max_volume = max_vol > min_vol ? max_vol : min_vol;
                }
                else
                    ASSERT(vol < 256);
                break;
            case InstrumentFadeWrap:
                vol = min_vol;
                break;
            case InstrumentFadeWrapLoweringVolume:
                vol = min_vol;
                // Decrease max volume:
                max_vol -= chip_player[i].volumed;
                chip_player[i].fade_saved_max_volume = max_vol > min_vol ? max_vol : min_vol;
                break;
            case InstrumentFadeWrapRaisingVolume:
                vol = min_vol;
                // Increase min volume:
                min_vol += chip_player[i].volumed;
                chip_player[i].fade_saved_min_volume = min_vol < max_vol ? min_vol : max_vol;
                break;
        }
        else if (vol < min_vol)
        switch (chip_player[i].fade_behavior/2)
        {   case InstrumentFadeClamp:
                vol = min_vol;
                break;
            case InstrumentFadeReverseClamp:
                vol = chip_player[i].fade_saved_max_volume;
                chip_player[i].volumed = 0;
                break;
            case InstrumentFadePingPong:
            case InstrumentFadePingPongLoweringVolume:
                // Update max volume for decreasing volume at the top of the ping/pong.
                if (chip_player[i].volumed < 0)
                {   // Only update delta volume at bottom:
                    vol = min_vol;
                    chip_player[i].volumed *= -1;
                }
                else
                    ASSERT(vol >= 0);
                break;
            case InstrumentFadePingPongRaisingVolume:
                if (chip_player[i].volumed < 0)
                {   // Update delta volume at bottom with max volume:
                    vol = min_vol;
                    chip_player[i].volumed *= -1;
                    // Increase min volume:
                    min_vol += chip_player[i].volumed;
                    chip_player[i].fade_saved_min_volume = min_vol < max_vol ? min_vol : max_vol;
                }
                else
                    ASSERT(vol >= 0);
                break;
            case InstrumentFadeWrap:
                vol = max_vol; 
                break;
            case InstrumentFadeWrapLoweringVolume:
                vol = max_vol; 
                // Decrease max volume:
                max_vol += chip_player[i].volumed;
                chip_player[i].fade_saved_max_volume = max_vol > min_vol ? max_vol : min_vol;
                break;
            case InstrumentFadeWrapRaisingVolume:
                vol = max_vol; 
                // Increase min volume:
                min_vol -= chip_player[i].volumed;
                chip_player[i].fade_saved_min_volume = min_vol < max_vol ? min_vol : max_vol;
                break;
        }
        chip_player[i].volume = vol;
      
        // TODO: maybe move this into chip_update_players()
        if (song_wait == song_speed)
        {
            vol = chip_player[i].track_volume + chip_player[i].track_volumed;
            if (vol < 0) vol = 0;
            else if (vol > 255) vol = 255;
            chip_player[i].track_volume = vol;
        }

        oscillator[i].volume = (chip_player[i].volume * chip_player[i].track_volume * chip_volume) >> 16;

        oscillator[i].duty += chip_player[i].dutyd << 6;
    }
}

static inline uint16_t gen_sample()
{   // This function generates one audio sample for all CHIP_PLAYERS oscillators. The returned
    // value is a 2*8bit stereo audio sample ready for putting in the audio buffer.

    // This is a simple noise generator based on an LFSR (linear feedback shift
    // register). It is fast and simple and works reasonably well for audio.
    // Note that we always run this so the noise is not dependent on the
    // oscillators frequencies.
    static uint32_t noiseseed = 1;
    static uint32_t rednoise = 0;
    static uint32_t violetnoise = 0;
    uint32_t newbit;
    newbit = 0;
    if (noiseseed & 0x80000000L) newbit ^= 1;
    if (noiseseed & 0x01000000L) newbit ^= 1;
    if (noiseseed & 0x00000040L) newbit ^= 1;
    if (noiseseed & 0x00000200L) newbit ^= 1;
    noiseseed = (noiseseed << 1) | newbit;
    rednoise = 3*rednoise/4 + (noiseseed&255)/4;
    // violet should be the derivative of white noise, but that wasn't nice:
    // this gives some higher freqs, and a metallic ring too:
    violetnoise = violetnoise/6 + ((noiseseed&255)-128); 

    int16_t acc[2] = { 0, 0 }; // accumulators for each channel
    
    for (int i=0; i<CHIP_PLAYERS; i++) 
    {   // Now compute the value of each oscillator and mix them
        if (!oscillator[i].volume)
            continue;
        
        int16_t value; // [-128, 127]

        uint16_t phase = oscillator[i].phase;
        uint16_t duty = oscillator[i].duty;
        switch (oscillator[i].waveform) 
        {   // Waveform changes the timbre:
            case WfSine:
                if (phase < duty)
                    value = sine_table[(phase << 5) / duty];
                else
                {   REPHASE16(phase, duty);
                    value = sine_table[32 + (phase << 5) / duty];
                }
                break;
            case WfTriangle:
                // Triangle: the part before duty raises, then it goes back down.
                if (phase < duty)
                    value = -128 + (phase << 8) / duty;
                else
                {   REPHASE16(phase, duty);
                    value = 127 - (phase << 8) / duty;
                }
                break;
            case WfSineTriangle:
                if (phase < duty)
                    // Start at top of sine wave and go to bottom (e.g. 16 to 48 in the sine_table[64])
                    value = sine_table[16 + (phase << 5) / duty];
                else
                {   REPHASE16(phase, duty);
                    // go from bottom to top like a saw
                    value = -128 + (phase << 8) / duty;
                }
                break;
            case WfSaw:
                // Sawtooth: always raising, but at half speed.
                if (phase < duty)
                    value = -128 + (phase << 7) / duty;
                else
                {   REPHASE16(phase, duty);
                    // going from zero up to 127:
                    value = (phase << 7) / duty;
                }
                break;
            case WfSineSaw:
                // Sine + Sawtooth
                if (phase < duty)
                    value = sine_table[(phase << 5) / duty];
                else
                {   REPHASE16(phase, duty);
                    // going from -128 up to 127:
                    value = -128 + (phase << 8) / duty;
                }
                break;
            case WfHalfUpSaw:
                // Half saw: the part before duty raises, then it drops to min
                if (phase < duty)
                    value = -128 + (phase << 8) / duty;
                else
                    value = -128;
                break;
            case WfSplitSaw:
                // Sounds like a radio-filtered saw (less low end)
                if (phase < duty)
                {   value = -128 + (phase << 8) / duty;
                    if (value > 0)
                        value = 0;
                }
                else
                {   REPHASE16(phase, duty);
                    value = -128 + (phase << 8) / duty;
                    if (value < 0)
                        value = 0;
                }
                break;
            case WfPulse:
                // Pulse: max value until we reach "duty", then min value.
                value = phase > duty ? -128 : 127;
                break;
            case WfHalfUpSine:
                if (phase < duty)
                    value = sine_table[(phase << 5) / duty];
                else
                    value = -128;
                break;
            case WfHalfDownSine:
                if (phase < duty)
                    value = sine_table[32 + (phase << 5) / duty];
                else
                    value = 127;
                break;
            case WfInvertedSine:
                // Sounds like a radio-filtered square (less low end)
                // sine bumps going the wrong way
                if (phase < duty)
                    value = 127 - sine_table[(phase << 5) / duty];
                else
                {   REPHASE16(phase, duty);
                    value = -128 + sine_table[(phase << 5) / duty];
                }
                break;
            case WfNoise:
                // Noise: from the generator. Only the low order bits are used.
                value = (noiseseed & 255) - 128;
                break;
            case WfRed:
                // Red Noise, integrated from white noise, but responds to duty != default 
                value =
                (   phase <= (uint16_t)(32767 - duty) ?
                        (rednoise & 255) :
                        (noiseseed & 255)
                ) - 128;
                break;
            case WfViolet:
                // Violet Noise, "derivative" of white noise, responds to duty != default
                value =
                (   phase <= (uint16_t)(32767 - duty) ?
                        (violetnoise & 255) :
                        (noiseseed & 255)
                ) - 128;
                break;
            case WfRedViolet:
                // Red + Violet Noise, but can be dutied
                value =
                (   phase < duty ?
                        (rednoise & 255) :
                        (violetnoise & 255)
                ) - 128;
                break;
            default:
                value = 0;
                break;
        }
        // Compute the oscillator phase (position in the waveform) for next time
        oscillator[i].phase += oscillator[i].freq / 4;

        // bit crusher effect; bitcrush == 0 does nothing:
        if (oscillator[i].bitcrush < 7)
            value |= ((1<<oscillator[i].bitcrush) - 1);
        else
            value &= 85>>(oscillator[i].bitcrush-7);

        // addition has range [-8160,7905], roughly +- 2**13
        int16_t add = (oscillator[i].volume * value) >> 2;
        
        switch (oscillator[i].pan)
        {   // Mix it in the appropriate output channel
            case 1:
                acc[0] += add;
                break;
            case 0:
            case 8:
                acc[0] += add;
                acc[1] += add;
                break;
            case 15:
                acc[1] += add;
                break;
            default:
                // Not exactly a perfect left-to-right pan, but close enough:
                if (oscillator[i].pan < 8)
                {   // Full Left, Quieter Right:
                    acc[0] += add;
                    acc[1] += add >> (8 - oscillator[i].pan);
                }
                else
                {   // Full Right, Quieter Left:
                    acc[0] += add >> (oscillator[i].pan - 8);
                    acc[1] += add;
                }
        }
    }
    // Now put the two channels together in the output word
    // acc has roughly +- (4 instr)*2**13  needs to return as 2*[1,251],  (roughly 128 +- 2**7)
    return (128 + (acc[0] >> 8))|(((128 + (acc[1] >> 8))) << 8);  // 2*[1,251]
}

void game_snd_buffer(uint16_t* buffer, int len) 
{   // Called by Bitbox to create the sound buffer; DO NOT CALL yourself.
    if (chip_playing)
        chip_update_players();

    // Even if a song/track is not playing, update oscillators in case a "chip_play_note" gets called.
    chip_update_oscillators();

    // Generate enough samples to fill the buffer.
    for (int i=0; i<len; i++)
        buffer[i] = gen_sample();
}

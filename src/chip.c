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

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

chip_playing_t chip_playing CCM_MEMORY;
uint8_t chip_volume CCM_MEMORY;

// the structs that get updated multiple times a frame,
// to generate the sound samples.
struct oscillator oscillator[CHIP_PLAYERS] CCM_MEMORY;

// you can have up to 16 instruments, which need to be played to modify the oscillators.
chip_instrument_t chip_instrument[16] CCM_MEMORY;

// a track corresponds to a single melody or single harmony, which has instructions on which instruments to play.
uint8_t chip_track_playtime CCM_MEMORY;
uint8_t chip_track[MAX_TRACKS][CHIP_PLAYERS][MAX_TRACK_LENGTH] CCM_MEMORY;
// Current absolute position since starting to play the track; shared between all players,
// even if their current track command index is different (e.g. due to jumps).
uint8_t track_pos CCM_MEMORY;

// the players combine commands from tracks and instruments to update the oscillators.
struct chip_player chip_player[CHIP_PLAYERS] CCM_MEMORY;
// instruments can be allowed to finish a note sometimes.
// tied in with creating a note randomly for sfx; maybe we should store more data on old oscillator state to restore.
uint8_t chip_instrument_for_next_note_for_player[CHIP_PLAYERS] CCM_MEMORY;

// a song is made out of a sequence of song commands (see song_cmd_t).
uint8_t chip_song_cmd[MAX_SONG_LENGTH];
uint8_t chip_song_cmd_index CCM_MEMORY;
uint8_t chip_song_players CCM_MEMORY;
uint8_t chip_song_volume CCM_MEMORY;
int8_t chip_song_volumed CCM_MEMORY;
uint8_t song_transpose CCM_MEMORY;
uint8_t song_wait CCM_MEMORY; // >0 means wait N frames, 0 means play now. 
uint8_t song_speed CCM_MEMORY;
uint8_t chip_song_variable_A CCM_MEMORY;
uint8_t chip_song_variable_B CCM_MEMORY;
uint8_t chip_song_squarify CCM_MEMORY;


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
    duty = 65536 - duty; \
    ASSERT(duty != 0); \
    ASSERT(phase < duty)

uint8_t chip_instrument_max_index(uint8_t i, uint8_t j)
{   // Returns the max index we shouldn't run (instrument command-wise) based on the current index.
    if (chip_instrument[i].is_drum)
    {   // Drums are split up into multiple sections so multiple noises can be made with one instrument.
        if (j < 2*DRUM_SECTION_LENGTH)
            return 2*DRUM_SECTION_LENGTH;
        else if (j < 3*DRUM_SECTION_LENGTH)
            return 3*DRUM_SECTION_LENGTH;
    }
    return MAX_INSTRUMENT_LENGTH;
}

int chip_instrument_invalid_jump
(   uint8_t inst, uint8_t max_index, uint8_t jump_from_index, uint8_t j
)
{   // Check if this is an ok jump; if the jump command hits itself again before a wait,
    // we'll get stuck in an infinite loop.
    for (int k=0; k<20; ++k)
    {   // Test going through the commands.
        if (j >= max_index) // it's ok to jump out of the commands, this will be interpreted as stopping
            return 0;
        else if (j == jump_from_index) // returning to the same spot is out of the question
            return 1;
        switch (chip_instrument[inst].cmd[j]&15)
        {   // Update instrument position j based on command:
            case InstrumentJump:
                j = chip_instrument[inst].cmd[j]>>4;
                break;
            case InstrumentWait:
                // We found a wait, this jump is ok.
                return 0;
            case InstrumentBreak:
                if ((chip_instrument[inst].cmd[j]>>4) == 0)
                    return 0;
            default:
                ++j;
        }
    }
    // do not proceed if no wait was found:
    return 1;
}

int chip_track_invalid_jump(uint8_t t, uint8_t i, uint8_t jump_from_index, uint8_t j)
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
            case TrackJump:
                j = 2*(chip_track[t][i][j]>>4);
                break;
            case TrackWait:
            case TrackArpNote:
                // We found a wait, this jump is ok.
                return 0;
            case TrackBreak:
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
        // TODO: Name these arguments using an enum
        case 0:
            return rand()%16;
        case 1:
            return 1 + (rand()%8)*2;
        case 2:
            return (rand()%8)*2;
        case 3:
            return (-1 + (rand()%3)) & 15;
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

static void chip_instrument_run_command(uint8_t i, uint8_t inst, uint8_t cmd) 
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
        case InstrumentStatic: // s = static
            oscillator[i].static_amt = param;
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
            uint8_t max_index = chip_instrument_max_index(inst, next_index-1);
            if (next_index >= max_index)
                break;
            uint8_t random = randomize(param);
            uint8_t next_command_type = chip_instrument[inst].cmd[next_index] & 15;
            if (next_command_type == InstrumentJump)
            {   // don't allow a randomized jump to cause an infinite loop:
                if (chip_instrument_invalid_jump(inst, max_index, next_index, random))
                    break; // do not continue, do not allow this number as a jump
            }
            chip_instrument[inst].cmd[next_index] = next_command_type | (random<<4);
            break;
        }
        case InstrumentJump:
            chip_player[i].cmd_index = param;
            break;
    }
}

static inline void chip_reset_song()
{   chip_playing = PlayingNone;
    chip_song_cmd_index = 0;
    song_speed = 4;
    song_transpose = 0;
    song_wait = 0;
    track_pos = 0;
    chip_song_volume = 255;
    chip_song_variable_A = 0;
    chip_song_variable_B = 0;
    chip_song_squarify = 0;
    chip_player[0].track_index = MAX_TRACKS;
    chip_player[0].next_track_index = MAX_TRACKS;
    chip_player[1].track_index = MAX_TRACKS;
    chip_player[1].next_track_index = MAX_TRACKS;
    chip_player[2].track_index = MAX_TRACKS;
    chip_player[2].next_track_index = MAX_TRACKS;
    chip_player[3].track_index = MAX_TRACKS;
    chip_player[3].next_track_index = MAX_TRACKS;
}

void chip_init()
{   // initialize things (only happens once).
    chip_volume = 128;
    chip_reset_song();
    // we assume it's 256 so that wrap around works immediately with u8's:
    static_assert(MAX_SONG_LENGTH == 256);
}

void chip_reset_player(int i)
{   // resets player i to a clean slate
    chip_instrument_for_next_note_for_player[i] = i;
    chip_player[i].instrument = i;
    chip_player[i].cmd_index = 0;
    chip_player[i].track_arp_low_note = 12;
    chip_player[i].track_arp_high_note = 12;
    chip_player[i].track_arp_scale = 0;
    chip_player[i].track_arp_wait = 1;
    chip_player[i].track_cmd_index = 0;
    chip_player[i].track_wait = 0;
    chip_player[i].volume = 0;
    chip_player[i].volumed = 0;
    chip_player[i].fade_behavior = 0;
    chip_player[i].fade_saved_max_volume = 255;
    chip_player[i].fade_saved_min_volume = 0;
    chip_player[i].track_volume = 255;
    chip_player[i].track_volumed = 0;
    chip_player[i].track_inertia = 0;
    chip_player[i].track_vibrato_rate = 0;
    chip_player[i].track_vibrato_depth = 0;
    chip_player[i].bendd = 0;
    chip_player[i].octave = chip_instrument[i].octave;
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
    chip_reset_song();
    chip_playing = PlayingSong;
    chip_song_cmd_index = pos;
    
    for (int i=0; i<CHIP_PLAYERS; ++i)
        chip_reset_player(i);
}

void chip_play_track(int track, int which_players)
{   // Makes all players loop the passed-in track.
    // If track >= MAX_TRACKS, no track will be played.
    // If which_players == 0, everyone plays, otherwise use a bit mask
    // for which players should play (via 1 << player_index).
    chip_reset_song();
    chip_playing = PlayingTrack;
    if (which_players == 0)
        which_players = (1 << CHIP_PLAYERS) - 1;

    for (int i=0; i<CHIP_PLAYERS; ++i)
    {   // Make the players loop on this track.
        chip_reset_player(i);
        if (which_players & 1)
        {   chip_player[i].next_track_index = track;
            chip_player[i].track_index = track;
        }
        else
        {   chip_player[i].track_index = MAX_TRACKS;
            chip_player[i].next_track_index = MAX_TRACKS;
        }
        which_players >>= 1;
    }
}

static void chip_play_note_internal(uint8_t p, uint8_t note)
{   // For player p, plays a note, for internal use only.  see `chip_play_note` for externally allowed usage.
    // Does not set the octave based on the current instrument, that must be part of `note`.
    #ifdef DEBUG_CHIPTUNE
    message("note %d on player %d\n", note, p);
    #endif
    chip_player[p].instrument = chip_instrument_for_next_note_for_player[p];
    // now set some defaults and the command index
    if (chip_instrument[chip_player[p].instrument].is_drum)
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
    chip_player[p].track_note = note;
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
    chip_play_note_internal(p, note + 12 * chip_player[p].octave);
    chip_player[p].track_volume = volume;
    chip_player[p].track_volumed = 0;
    chip_instrument_for_next_note_for_player[p] = old_instrument;
}

static inline int chip_get_next_note_in_scale(const int *scale, int note, int direction)
{   // pass in a note relative to the current scale (0 = 12 = root, 4 = major third, 7 = fifth)
    // and you will get the next note up or down, based on direction.
    // scale[i] should be > 0 and <= 12; it should increase until the last note which is a 12 (to indicate finish).
    int octave = note / 12;
    note = note % 12;
    // ensure that scale[i] == 12 at the end of the input:
    ASSERT(scale[1] == 12 || scale[2] == 12 || scale[3] == 12 || scale[4] == 12 || scale[5] == 12 || scale[6] == 12);
    if (direction > 0)
    while (1)
    {   // get first note in scale that is greater than note and return it.
        int scale_note = *scale++;
        if (scale_note > note)
        {   note = scale_note;
            break;
        }
    }
    else
    {   // get last note in scale that is smaller than note and return it.
        if (note == 0)
        {   // at the bottom of the scale we need to think of note as the top of the next-lower octave:
            note = 12;
            --octave;
        }
        int previous_note = 0;
        while (1)
        {   int scale_note = *scale++;
            if (scale_note >= note)
            {   note = previous_note;
                break;
            }
            previous_note = scale_note;
        }
    }
    return 12 * octave + note;
}

static inline int chip_try_finding_root_in_scale(const int *scale1, int low_note, int high_note)
{   // given a scale and the low and high notes,
    // find the root note that would be the key of that scale (i.e., which includes low and high notes).
    // if we can't find anything meaningful, return low_note.

    // if the high and low notes are octaves apart, then we wouldn't have any information to guess root
    // besides the note itself.
    int delta_high_low = (high_note - low_note) % 12;
    if (delta_high_low == 0)
        return low_note;

    while (1)
    {   // check what root note would satisfy 
        //  * (low_note - root) % 12 == scale_noteA     (equation 1.A)
        //  * (high_note - root) % 12 == scale_noteB    (equation 1.B)
        // where (A, B) can be (1, 2) or (2, 1).
        // expanding the modulus, low_note - root = 12*m + scale_noteA, for m an integer,
        // so: root = low_note - scale_noteA + 12*n     # set n = -m, still an integer.
        // choose `n` such that root is reasonable.
        // we'll have to find a situation where
        //  * (high_note - low_note) % 12 == (scale_note2 - scale_note1) % 12
        // and then check to see if scale_noteA is related to scale_note1 or 2.
        // i.e., set root = low_note - scale_note1 and see if equations 1.A,B are satisfied.
        // i.e., otherwise try root = low_note - scale_note2.
        // otherwise set root = low_note - scale_note2.

        // check scale1 < scale2, so N^2 / 2 times around the loop
        int scale_note1 = *scale1++;
        if (scale_note1 == 12)
            break;
        const int *scale2 = scale1;
        while (1)
        {   int scale_note2 = *scale2++;
            int delta12 = scale_note2 - scale_note1;
            ASSERT(delta12 > 0 && delta12 < 12);
            // since we only check scale1 < scale2, (scale_note2 - scale_note1) could theoretically
            // be reversed for high/low notes, so check 12 - delta12 as an option as well.
            if (delta12 == delta_high_low || (12 - delta12 == delta_high_low))
            {   int try_root = low_note - scale_note2;
                if ((high_note - try_root) % 12 == scale_note1) // scale_note1 is between 0 and 11
                    return try_root;
                try_root = low_note - scale_note1;
                 // note the extra % 12; scale_note2 is between 1 and 12:
                ASSERT((high_note - try_root) % 12 == scale_note2 % 12);
                return try_root;
            }
            if (scale_note2 == 12)
                break;
        }
    }
    return low_note;
}

static inline uint8_t chip_player_get_next_arp_note(uint8_t p, int direction)
{   int high_note = chip_player[p].track_arp_high_note;
    int low_note = chip_player[p].track_arp_low_note;
    int delta_high_low = high_note - low_note;
    int note = chip_player[p].track_note;
    if (delta_high_low <= 0)
    {   // high/low are backwards and unreliable, just jump between high and low:
        return note == low_note ? high_note : low_note;
    }
    // Make sure note is within range as well, use wrapping when getting to a boundary:
    if (note < low_note || (note == low_note && direction < 0))
        return high_note;
    if (note > high_note || (note == high_note && direction > 0))
        return low_note;
    // delta_high_low > 0 and low_note <= (current) note <= high_note, and
    // if the current note is at the boundary, it is coming away from it.
    int root;
    switch (chip_player[p].track_arp_scale)
    {   case ScaleMajorTriad:
        {   int scale[] = {4, 7, 12}; // ensure having 12 at end
            root = chip_try_finding_root_in_scale(scale, low_note, high_note);
            note = chip_get_next_note_in_scale(scale, note - root, direction) + root;
            break;
        }
        case ScaleMinorTriad:
        {   int scale[] = {3, 7, 12}; // ensure having 12 at end
            root = chip_try_finding_root_in_scale(scale, low_note, high_note);
            note = chip_get_next_note_in_scale(scale, note - root, direction) + root;
            break;
        }
        case ScaleMajor7:
        {   int scale[] = {4, 7, 11, 12}; // ensure having 12 at end
            root = chip_try_finding_root_in_scale(scale, low_note, high_note);
            note = chip_get_next_note_in_scale(scale, note - root, direction) + root;
            break;
        }
        case ScaleMinor7:
        {   int scale[] = {3, 7, 10, 12}; // ensure having 12 at end
            root = chip_try_finding_root_in_scale(scale, low_note, high_note);
            note = chip_get_next_note_in_scale(scale, note - root, direction) + root;
            break;
        }
        case ScaleSuspended4Triad:
        {   int scale[] = {5, 7, 12}; // ensure having 12 at end
            root = chip_try_finding_root_in_scale(scale, low_note, high_note);
            note = chip_get_next_note_in_scale(scale, note - root, direction) + root;
            break;
        }
        case ScaleSuspended2Triad:
        {   int scale[] = {2, 7, 12}; // ensure having 12 at end
            root = chip_try_finding_root_in_scale(scale, low_note, high_note);
            note = chip_get_next_note_in_scale(scale, note - root, direction) + root;
            break;
        }
        case ScaleMajor6:
        {   int scale[] = {4, 7, 9, 12}; // ensure having 12 at end
            root = chip_try_finding_root_in_scale(scale, low_note, high_note);
            note = chip_get_next_note_in_scale(scale, note - root, direction) + root;
            break;
        }
        case ScaleMinor6:
        {   int scale[] = {3, 7, 9, 12}; // ensure having 12 at end
            root = chip_try_finding_root_in_scale(scale, low_note, high_note);
            note = chip_get_next_note_in_scale(scale, note - root, direction) + root;
            break;
        }
        case ScaleAugmentedTriad:
        {   int scale[] = {4, 8, 12}; // ensure having 12 at end
            root = chip_try_finding_root_in_scale(scale, low_note, high_note);
            note = chip_get_next_note_in_scale(scale, note - root, direction) + root;
            break;
        }
        case ScaleDiminishedTriad:
        {   int scale[] = {3, 6, 12}; // ensure having 12 at end
            root = chip_try_finding_root_in_scale(scale, low_note, high_note);
            note = chip_get_next_note_in_scale(scale, note - root, direction) + root;
            break;
        }
        case ScaleAugmented7:
        {   int scale[] = {4, 8, 10, 12}; // ensure having 12 at end
            root = chip_try_finding_root_in_scale(scale, low_note, high_note);
            note = chip_get_next_note_in_scale(scale, note - root, direction) + root;
            break;
        }
        case ScaleDiminished7:
        {   int scale[] = {3, 6, 9, 12}; // ensure having 12 at end
            root = chip_try_finding_root_in_scale(scale, low_note, high_note);
            note = chip_get_next_note_in_scale(scale, note - root, direction) + root;
            break;
        }
        case ScaleFifths:
        {   int scale[] = {7, 12}; // ensure having 12 at end
            root = chip_try_finding_root_in_scale(scale, low_note, high_note);
            note = chip_get_next_note_in_scale(scale, note - root, direction) + root;
            break;
        }
        case ScaleOctaves:
            note += 12 * direction;
            break;
        case ScaleDominant7:
        {   int scale[] = {4, 7, 10, 12}; // ensure having 12 at end
            root = chip_try_finding_root_in_scale(scale, low_note, high_note);
            note = chip_get_next_note_in_scale(scale, note - root, direction) + root;
            break;
        }
        case ScaleChromatic:
            note += direction;
            break;
    }
    // use wrapping here, too:
    if (note < low_note)
        return high_note;
    if (note > high_note)
        return low_note;
    return note;
}

static void track_run_command(uint8_t i, uint8_t cmd) 
{   // Run a command for player i on their track.
    uint8_t param = cmd >> 4;
    switch(cmd&15) 
    {   // Command type is in lower nibble, param was from upper nibble.
        case TrackBreak: // f = wait til a given quarter note, or break if passed that.
            if (4*param > track_pos)
                chip_player[i].track_wait = 4*param - track_pos;
            else
                chip_player[i].track_cmd_index = MAX_TRACK_LENGTH; // end track commmands
            break;
        case TrackOctave: // O = octave, or + or - for relative octave
            if (param < 7)
                chip_player[i].octave = param;
            else if (param == 7)
            {   // use instrument default octave
                chip_player[i].octave = chip_instrument
                [   chip_instrument_for_next_note_for_player[i]
                ].octave;
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
        case TrackInstrument:
            chip_instrument_for_next_note_for_player[i] = param;
            break;
        case TrackVolume: // v = volume
            chip_player[i].track_volume = param*17;
            break;
        case TrackFadeInOrOut:
            // TODO: double check
            if (param > 7)
            {   // fade out
                param = 16 - param;
                chip_player[i].track_volumed = -param - param*param/15;
            }
            else
            {   // fade in
                chip_player[i].track_volumed = param + param*param/15;
            }
            break;
        case TrackNote:
            param += 12 * chip_player[i].octave;
            chip_play_note_internal(i, param);
            chip_player[i].track_arp_high_note = param;
            break;
        case TrackWait: // w = wait 
            if (param)
                chip_player[i].track_wait = param;
            else
                chip_player[i].track_wait = 16;
            break;
        case TrackArpNote:
        {   uint8_t note;
            // Plays a note in the scale (if param > 11) or sets the low note in the arpeggio and plays that.
            // In either case, we also set a following wait based on the track arpeggio wait.
            if (param > 11)
            switch (param)
            {   case ArpPlayLowNote:
                    note = chip_player[i].track_arp_low_note;
                    break;
                case ArpPlayHighNote:
                    note = chip_player[i].track_arp_high_note;
                    break;
                case ArpPlayNextNoteDown:
                    note = chip_player_get_next_arp_note(i, -1);
                    break;
                case ArpPlayNextNoteUp:
                    note = chip_player_get_next_arp_note(i, +1);
                    break;
            }
            else
            {   note = param + 12 * chip_player[i].octave;
                chip_player[i].track_arp_low_note = note;
            }
            chip_play_note_internal(i, note);
            chip_player[i].track_wait = chip_player[i].track_arp_wait;
            break;
        }
        case TrackArpScale:
            chip_player[i].track_arp_scale = param;
            break;
        case TrackArpWait:
            chip_player[i].track_arp_wait = param + 1;
            break;
        case TrackVibrato: // ~ = vibrato depth
            chip_player[i].track_vibrato_depth = (param&3)*3 + (param>3)*2;
            chip_player[i].track_vibrato_rate = 1 + (param & 12)/2;
            break;
        case TrackInertia: // i = inertia (auto note slides)
            chip_player[i].track_inertia = param;
            break;
        case TrackBend:
            // TODO:
            break;
        case TrackStatic:
            // TODO:
            break;
        case TrackRandomize:
        {   uint8_t next_index = chip_player[i].track_cmd_index;
            if (next_index >= MAX_TRACK_LENGTH)
                break;
            uint8_t random = randomize(param);
            uint8_t t = chip_player[i].track_index;
            if ((chip_track[t][i][next_index]&15) == TrackJump && 
                chip_track_invalid_jump(t, i, next_index, 2*random))
                break;
            chip_track[t][i][next_index] = 
                (chip_track[t][i][next_index]&15) | (random<<4);
            break;
        }
        case TrackJump: // 
            chip_player[i].track_cmd_index = 2*param;
            break;
    }
}

static inline void chip_song_set_player_tracks(uint8_t track, uint8_t next_track)
{   uint8_t players = chip_song_players ? chip_song_players : 15;
    if (players & 1)
    {   chip_player[0].track_index = track;
        chip_player[0].next_track_index = next_track;
    }
    if (players & 2)
    {   chip_player[1].track_index = track;
        chip_player[1].next_track_index = next_track;
    }
    if (players & 4)
    {   chip_player[2].track_index = track;
        chip_player[2].next_track_index = next_track;
    }
    if (players & 8)
    {   chip_player[3].track_index = track;
        chip_player[3].next_track_index = next_track;
    }
}

static inline void chip_update_players_for_track()
{   // Runs updates on players based on track commands
    #ifdef DEBUG_CHIPTUNE
    message("%02d", track_pos);
    #endif

    for (int p=0; p<CHIP_PLAYERS; ++p) 
    {   uint8_t track_index = chip_player[p].track_index;
        #ifdef DEBUG_CHIPTUNE
        message(" | [t%02d: %02d]", track_index, chip_player[p].track_cmd_index);
        #endif
        if (track_index >= MAX_TRACKS)
            continue;

        while (!chip_player[p].track_wait && chip_player[p].track_cmd_index < MAX_TRACK_LENGTH) 
            track_run_command(p, chip_track[track_index][p][chip_player[p].track_cmd_index++]);

        if (chip_player[p].track_wait)
            --chip_player[p].track_wait;
    }

    #ifdef DEBUG_CHIPTUNE
    message("\n");
    #endif

    if (++track_pos == chip_track_playtime)
    {   for (int p=0; p<CHIP_PLAYERS; ++p)
        {   chip_player[p].track_cmd_index = 0;
            chip_player[p].track_index = chip_player[p].next_track_index;
            chip_player[p].track_wait = 0;
        }
        track_pos = 0;
    }
}

static int chip_song_invalid_jump(uint8_t jump_from_index, uint8_t j)
{   // returns true if jump (from jump_from_index) to the index j is invalid.
    for (int k=0; k<20; ++k)
    {   if (j == jump_from_index) // returning to the same spot is out of the question
            return 1;
        uint8_t command = chip_song_cmd[j];
        switch (command & 15)
        {   // Update instrument position j based on command:
            // TODO: SongBreak (0 isn't ok, 1-15 is ok, but check for randomization before)
            case SongPlayTracksForCount:
                // We found a wait, this jump is ok.
                return 0;
            case SongSpecial:
                // if we have special conditionals, also die.
                // make sure to play tracks for count before these.
                if ((command>>4) < 4)
                    return 1;
            case SongJump:
                j = (command >> 4) * 16;
                break;
            default:
                ++j;
        }
    }
    // invalid jump
    return 1;
}

static void chip_song_try_setting_current_command_param_to(uint8_t param)
{   // checks for jumps and conditionals before assigning new param.
    uint8_t current_command = chip_song_cmd[chip_song_cmd_index];
    uint8_t current_param = current_command >> 4;
    current_command &= 15;
    switch (current_command)
    {   case SongSpecial:
            // ignore changes to Song conditionals unless param is also a conditional:
            if (current_param < 4 && param >= 4)
                return;
            break;
        case SongJump:
            // require jumps to be valid:
            if (chip_song_invalid_jump(chip_song_cmd_index, param))
                return;
            break;
    }
    // if we're here, we can change command to use new param:
    chip_song_cmd[chip_song_cmd_index] = current_command | (param << 4);
}

static inline void chip_song_run_command(uint8_t cmd)
{   uint8_t param = cmd >> 4;
    cmd &= 15;
    switch (cmd)
    {   case SongBreak:
            chip_reset_song();
            chip_playing = PlayingSong;
            // TODO: adjust multiplier on param if needed:
            chip_track_playtime = 2 * param;
            break;
        case SongVolume:
            // TODO: use
            chip_song_volume = param * 17;
            break;
        case SongFadeInOrOut:
            // TODO: use
            chip_song_volumed = param < 8 ? param : -16 + param; 
            break;
        case SongChoosePlayers:
            chip_song_players = param;
            break;
        case SongSetLowTrackForChosenPlayers:
            chip_song_set_player_tracks(param, MAX_TRACKS);
            break;
        case SongSetHighTrackForChosenPlayers:
            chip_song_set_player_tracks(16 + param, MAX_TRACKS);
            break;
        case SongRepeatLowTrackForChosenPlayers:
            chip_song_set_player_tracks(param, param);
            break;
        case SongRepeatHighTrackForChosenPlayers:
            chip_song_set_player_tracks(16 + param, 16 + param);
            break;
        case SongPlayTracksForCount:
            chip_track_playtime = param ? 4 * param : 64;
            break;
        case SongSpeed:
            song_speed = param ? param : 16;
            break;
        case SongTranspose:
            song_transpose = param;
            break;
        case SongSquarify:
            chip_song_squarify = param;
            break;
        case SongSetVariableA:
            chip_song_variable_A = param;
            break;
        case SongSpecial:
            switch (param)
            {   case SongIfAEqualsZeroExecuteNextOtherwiseFollowingCommand:
                    if (chip_song_variable_A)
                        ++chip_song_cmd_index;
                    break;
                case SongIfAGreaterThanZeroExecuteNextOtherwiseFollowingCommand:
                    if (!chip_song_variable_A)
                        ++chip_song_cmd_index;
                    break;
                case SongIfALessThanBExecuteNextOtherwiseFollowingCommand:
                    if (chip_song_variable_A >= chip_song_variable_B)
                        ++chip_song_cmd_index;
                    break;
                case SongIfAEqualsBExecuteNextOtherwiseFollowingCommand:
                    if (chip_song_variable_A != chip_song_variable_B)
                        ++chip_song_cmd_index;
                    break;
                case SongSetNextCommandParameterToA:
                    chip_song_try_setting_current_command_param_to(chip_song_variable_A);
                    break;
                case SongSetBEqualToA:
                    chip_song_variable_B = chip_song_variable_A;
                    break;
                case SongSwapAAndB:
                    param = chip_song_variable_A;
                    chip_song_variable_A = chip_song_variable_B;
                    chip_song_variable_B = param;
                    break;
                case SongModAByB:
                    if (chip_song_variable_B)
                        chip_song_variable_A %= chip_song_variable_B;
                    else
                        chip_song_variable_A = 0;
                    break;
                case SongDivideAByB:
                    if (chip_song_variable_B)
                        chip_song_variable_A /= chip_song_variable_B;
                    else
                        chip_song_variable_A = 15;
                    break;
                case SongAddBToA:
                    chip_song_variable_A = (chip_song_variable_A + chip_song_variable_B) & 15;
                    break;
                case SongSubtractBFromA:
                    chip_song_variable_A = (chip_song_variable_A - chip_song_variable_B) & 15;
                    break;
                case SongHalveA:      
                    chip_song_variable_A /= 2; 
                    break;
                case SongIncrementAWithWraparound:
                    chip_song_variable_A = (chip_song_variable_A + 1) & 15;
                    break;
                case SongDecrementAWithWraparound:
                    chip_song_variable_A = (chip_song_variable_A + 15) & 15;
                    break;
                case SongIncrementANoWrap:
                    if (chip_song_variable_A < 15)
                        ++chip_song_variable_A;
                    break;
                case SongDecrementANoWrap:
                    if (chip_song_variable_A)
                        --chip_song_variable_A;
                    break;
            }
            break;
        case SongRandomize:
            chip_song_try_setting_current_command_param_to(randomize(param));
            break;
        case SongJump:
            chip_song_cmd_index = 16 * param;
            break;
    }
}

static inline void chip_update_players_for_song()
{   // Called if user is playing a song
    if (!track_pos) // track_pos == 0
    {   // run the next song command(s) 
        #ifdef DEBUG_CHIPTUNE
        message("\n{s: %02d}\n", chip_song_cmd_index);
        #endif
        chip_track_playtime = 0;
        int k = 0;
        while (1)
        {   chip_song_run_command(chip_song_cmd[chip_song_cmd_index++]);
            if (chip_track_playtime)
                break;
            if (++k >= 32)
            {   message("song should have a PlayTracksForCount command at least every 32 steps.\n");
                break;
            }
        }
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
   
        // Run drum commands (with smaller instrument sequence), or up to all commands if not a drum:
        uint8_t max_index = chip_instrument[inst].is_drum ?
            chip_player[i].max_drum_index : MAX_INSTRUMENT_LENGTH;
        // We can get a command to set wait to nonzero, so make sure we escape in that case:
        while (!chip_player[i].wait && chip_player[i].cmd_index < max_index)
        {   chip_instrument_run_command
            (   i, inst, chip_instrument[inst].cmd[chip_player[i].cmd_index++]
            );
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
        if (!oscillator[i].duty || oscillator[i].duty == 65535)
            oscillator[i].duty = 1;
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
            case WfHalfSine:
                if (phase < duty)
                    value = sine_table[(phase << 6) / duty];
                else
                    value = 0;
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
            case WfSquareTriangle:
                // Quickly from max to min, stay at min for half a cycle, then triangle back to max 
                if (phase < duty)
                    value = phase < duty/2 ? 127 - (phase << 8) / duty : -128;
                else
                {   REPHASE16(phase, duty);
                    value = phase < duty/2 ? -128 : -128 + (phase << 8) / duty;
                }
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
            case WfJumpSine:
                // top quarter of sine wave for first half of period
                // bottom quarter of sine wave for second half of period
                if (phase < duty)
                    value = sine_table[(phase << 4) / duty];
                else
                {   REPHASE16(phase, duty);
                    value = sine_table[32 + (phase << 4) / duty];
                }
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

        int static_amt = oscillator[i].static_amt;
        if (static_amt && phase < duty/2)
        {   // TODO: this doesn't do anything fun for whitenoise, we should make it exciting
            value = (value * (16 - static_amt) + static_amt * (((noiseseed >> 4) & 255) - 128)) / 16;
        }

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

#ifdef EMULATOR
void test_chip()
{   {   // next note in random scale
        int scale[] = {4, 7, 9, 10, 12};
        // first octave:
        ASSERT(chip_get_next_note_in_scale(scale, 0, +1) == 4);
        ASSERT(chip_get_next_note_in_scale(scale, 3, +1) == 4);
        ASSERT(chip_get_next_note_in_scale(scale, 4, +1) == 7);
        ASSERT(chip_get_next_note_in_scale(scale, 6, +1) == 7);
        ASSERT(chip_get_next_note_in_scale(scale, 9, +1) == 10);
        ASSERT(chip_get_next_note_in_scale(scale, 10, +1) == 12);
        // second octave:
        ASSERT(chip_get_next_note_in_scale(scale, 12 + 0, +1) == 12 + 4);
        ASSERT(chip_get_next_note_in_scale(scale, 12 + 7, +1) == 12 + 9);
        ASSERT(chip_get_next_note_in_scale(scale, 12 + 9, +1) == 12 + 10);
        // random (fourth) octave:
        ASSERT(chip_get_next_note_in_scale(scale, 3*12 + 0, +1) == 3*12 + 4);
        ASSERT(chip_get_next_note_in_scale(scale, 3*12 + 10, +1) == 3*12 + 12);
    }
    {   // previous note in random scale
        int scale[] = {3, 4, 5, 10, 12};
        // Can go negative:
        ASSERT(chip_get_next_note_in_scale(scale, 0, -1) == -2);
        ASSERT(chip_get_next_note_in_scale(scale, 3, -1) == 0);
        ASSERT(chip_get_next_note_in_scale(scale, 4, -1) == 3);
        ASSERT(chip_get_next_note_in_scale(scale, 6, -1) == 5);
        ASSERT(chip_get_next_note_in_scale(scale, 9, -1) == 5);
        ASSERT(chip_get_next_note_in_scale(scale, 10, -1) == 5);
        // second octave:
        ASSERT(chip_get_next_note_in_scale(scale, 12, -1) == 10);
        ASSERT(chip_get_next_note_in_scale(scale, 12 + 3, -1) == 12 + 0);
        ASSERT(chip_get_next_note_in_scale(scale, 12 + 5, -1) == 12 + 4);
        ASSERT(chip_get_next_note_in_scale(scale, 12 + 9, -1) == 12 + 5);
        // random (fifth) octave
        ASSERT(chip_get_next_note_in_scale(scale, 4*12 + 9, -1) == 4*12 + 5);
        ASSERT(chip_get_next_note_in_scale(scale, 4*12 + 12, -1) == 4*12 + 10);
    }
    {   // finding the root in a scale
        int scale[] = {4, 7, 12};

        for (int key = 0; key < 12; ++key)
        {   // high note is "higher" than low note in the scale:
            /// 4, 12
            ASSERT(chip_try_finding_root_in_scale(scale, key + 4, 12+key + 12) == key);
            /// 7, 12
            ASSERT(chip_try_finding_root_in_scale(scale, 3*12+key + 7, 5*12+key + 12) == 3*12 + key);
            /// 4, 7
            ASSERT(chip_try_finding_root_in_scale(scale, 12+key + 4, 3*12+key + 7) == 12 + key);

            // high note is "lower" than low note in the scale:
            /// 12, 4 
            // TODO: probably should equal key+12 and not key, but this is ok.
            ASSERT(chip_try_finding_root_in_scale(scale, 12+key, 2*12+key + 4) == key);
            /// 12, 7 
            // TODO: probably should equal key and not key-12, but this is ok.
            ASSERT(chip_try_finding_root_in_scale(scale, key, 5*12+key + 7) == key - 12);
            /// 7, 4 
            ASSERT(chip_try_finding_root_in_scale(scale, 2*12+key + 7, 3*12+key + 4) == 2*12 + key);
        }
        // TODO: do test for more complicated scale, ensuring that the "simple" root is used if multiple options are available
        // or just don't use any scales which are ambiguous??

        // octaves return the low note:
        ASSERT(chip_try_finding_root_in_scale(scale, 5, 12 + 5) == 5);
        ASSERT(chip_try_finding_root_in_scale(scale, 12 + 9, 5*12 + 9) == 12 + 9);
    }
    // TODO: other random scale
    message("chip tests passed!\n");
}
#endif

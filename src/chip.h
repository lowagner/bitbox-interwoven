/* Simple soundengine for the BitBox
 * Copyright 2014, Adrien Destugues <pulkomandy@pulkomandy.tk>
 * Copyright 2007, Linus Akesson
 * Based on the "Hardware Chiptune" project
 */
#ifndef CHIP_H
#define CHIP_H
#include <stdint.h>
#include "game.h"

#define MAX_INSTRUMENT_LENGTH 16
#define DRUM_SECTION_LENGTH (MAX_INSTRUMENT_LENGTH/4)
#define MAX_SONG_LENGTH 60
#define MAX_TRACK_LENGTH 64
#define MAX_NOTE 84
#define CHIP_PLAYERS 4

typedef enum {
    PlayingNone = 0,
    PlayingSong,
    PlayingTrack,
} chip_playing_t;

extern chip_playing_t chip_playing;
extern uint8_t chip_repeat;
extern uint8_t chip_volume;

// These are our possible waveforms.
typedef enum 
{   WfSine = 0,  // = ^v but smoother 
    WfTriangle, // = \/
    WfHalfSine,  // = ^v--  sine for half of the period, silence the other half
    WfSineTriangle, // '-./ half a period's sine wave followed by triangle rise

    WfSaw, // = /|/|
    WfSineSaw, // = ^/
    WfSquareTriangle, // = \_/
    WfSplitSaw, // .-' with the middle section being flat

    WfPulse, // = |_|- (adjustable duty)
    WfJumpSine,  // = cc with top half, then bottom half of c's
    WfHalfDownSine,  // = v'' but smoother at the beginning
    WfInvertedSine, // bump on bottom going wrong way, bump on top going wrong way

    WfNoise, // = !*@?
    WfRed, // = "integral" of WfNoise, mixes with WfNoise when duty is not default
    WfViolet, // = "derivative" of WfNoise, mixes with WfNoise when duty is not default
    WfRedViolet, // waveform flips between Red and Violet based on phase/duty.
} wf_t;

// TODO: double check our order is ok:
typedef enum
{   InstrumentBreak = 0,
    InstrumentPan = 1,
    InstrumentWaveform = 2,
    InstrumentVolume = 3,
    InstrumentNote = 4,
    InstrumentWait = 5,
    InstrumentFadeMagnitude = 6,
    InstrumentFadeBehavior = 7,
    InstrumentInertia = 8,
    InstrumentVibrato = 9, // Frequency oscillation
    InstrumentBend = 10,
    // TODO: actually figure something out here:
    InstrumentSpecial = 11,
    InstrumentDuty = 12,
    InstrumentDutyDelta = 13,
    InstrumentRandomize = 14,
    InstrumentJump = 15,
    // Nothing 16 or above is allowed
} inst_cmd_t;

typedef enum
{   // Positive/negative enforcement for the fade magnitude.  i.e., can be a fade in or fade out.
    InstrumentFadeNegativeVolumeD = 0, // when user sets the magnitude, make it a fade out
    InstrumentFadePositiveVolumeD = 1, // when user sets the magnitude, make it a fade in
} inst_fade_volumed_behavior_t;

typedef enum
{   // What happens when a fade reaches the limit (min volume or max volume)
    InstrumentFadeClamp = 0, // Stop at min (or max) and stay there
    InstrumentFadeReverseClamp = 1, // go to max volume and then cut to min, or vice versa.
    InstrumentFadePingPong = 2, // Go back and forth between min and max volume
    InstrumentFadePingPongLoweringVolume = 3, // Go back and forth between min and max, decrease max over time
    InstrumentFadePingPongRaisingVolume = 4, // Go back and forth between min and max, increase min over time
    InstrumentFadeWrap = 5, // When reaching the max, return to min.  Or vice versa. 
    InstrumentFadeWrapLoweringVolume = 6, // When hitting a limit, decrease the max. 
    InstrumentFadeWrapRaisingVolume = 7, // When hitting a limit, increase the min 
} inst_fade_behavior_type_t;

typedef enum
{   // Combined behavior_type and volumed_behavior:
    //                       enforce-negative  enforce-positive
    // clamp                        0   \__         1   /‾‾
    // reverse-clamp                2   \‾‾         3   /__
    //                       enforce-negative  enforce-positive
    // ping-pong                    4   \/\/\       5   /\/\/
    // ping-pong with lowering vol  6               7
    // ping-pong increasing vol     8               9
    // wrap                         a   \\\\        b   ////
    // wrap with decreasing vol     b               c
    // wrap with increasing vol     e               f
    InstrumentFadeClampWithNegativeVolumeD = 2 * InstrumentFadeClamp + InstrumentFadeNegativeVolumeD,
    InstrumentFadeClampWithPositiveVolumeD = 2 * InstrumentFadeClamp + InstrumentFadePositiveVolumeD,
    InstrumentFadeReverseClampWithNegativeVolumeD = 2 * InstrumentFadeReverseClamp + InstrumentFadeNegativeVolumeD,
    InstrumentFadeReverseClampWithPositiveVolumeD = 2 * InstrumentFadeReverseClamp + InstrumentFadePositiveVolumeD,
    InstrumentFadePingPongStartingWithNegativeVolumeD = 2 * InstrumentFadePingPong + InstrumentFadeNegativeVolumeD,
    InstrumentFadePingPongStartingWithPositiveVolumeD = 2 * InstrumentFadePingPong + InstrumentFadePositiveVolumeD,
    InstrumentFadePingPongLoweringVolumeStartingWithNegativeVolumeD
            = 2 * InstrumentFadePingPongLoweringVolume + InstrumentFadeNegativeVolumeD,
    InstrumentFadePingPongLoweringVolumeStartingWithPositiveVolumeD
            = 2 * InstrumentFadePingPongLoweringVolume + InstrumentFadePositiveVolumeD,
    InstrumentFadePingPongRaisingVolumeStartingWithNegativeVolumeD
            = 2 * InstrumentFadePingPongRaisingVolume + InstrumentFadeNegativeVolumeD,
    InstrumentFadePingPongRaisingVolumeStartingWithPositiveVolumeD
            = 2 * InstrumentFadePingPongRaisingVolume + InstrumentFadePositiveVolumeD,
    InstrumentFadeWrapWithNegativeVolumeD = 2 * InstrumentFadeWrap + InstrumentFadeNegativeVolumeD,
    InstrumentFadeWrapWithPositiveVolumeD = 2 * InstrumentFadeWrap + InstrumentFadePositiveVolumeD,
    InstrumentFadeWrapLoweringVolumeWithNegativeVolumeD
            = 2 * InstrumentFadeWrapLoweringVolume + InstrumentFadeNegativeVolumeD,
    InstrumentFadeWrapLoweringVolumeWithPositiveVolumeD
            = 2 * InstrumentFadeWrapLoweringVolume + InstrumentFadePositiveVolumeD,
    InstrumentFadeWrapRaisingVolumeWithNegativeVolumeD
            = 2 * InstrumentFadeWrapRaisingVolume + InstrumentFadeNegativeVolumeD,
    InstrumentFadeWrapRaisingVolumeWithPositiveVolumeD
            = 2 * InstrumentFadeWrapRaisingVolume + InstrumentFadePositiveVolumeD,
} inst_fade_behavior_t;

// TODO:
typedef enum
{   TrackBreak = 0,
    TrackOctave = 1,
    TrackInstrument = 2,
    TrackVolume = 3,
    TrackFadeInOrOut = 4,
    // Also counts as ArpHighNote:
    TrackNote = 5,
    TrackWait = 6,
    TrackArpLowNote = 7,
    TrackArpScale = 8,
    // TODO: something better than vibrato, we can just switch an instrument.
    // Maybe we can add to the oscillator.bitcrush/special/static
    TrackVibrato = 9, // Frequency oscillation
    TrackGlobalTranspose = 10,
    TrackGlobalSpeed = 11,
    TrackGlobalTrackLength = 12,
    // TODO: Execute next command if some condition holds, otherwise following.
    // Argument provides the condition.
    // TODO: show entire track, since jumps can go anywhere??
    TrackConditional = 13,
    TrackRandomize = 14,
    TrackJump = 15,
    // Nothing 16 or above is allowed
} track_cmd_t;

// TODO: just move into chip_player
struct oscillator {
    uint8_t pan;
    uint8_t volume;
    uint8_t waveform; // waveform (from the enum above)
    uint8_t bitcrush;

    uint16_t duty; // duty cycle (pulse wave only)
    uint16_t freq; // frequency
    uint16_t phase; // phase
};

extern struct oscillator oscillator[CHIP_PLAYERS];

struct instrument 
{
    uint8_t is_drum;
    uint8_t octave;
    // commands which create the instrument sound
    // stuff in the cmd array can be modified externally.
    uint8_t cmd[MAX_INSTRUMENT_LENGTH];
};

// TODO: preface with chip_
extern struct instrument instrument[16];

extern uint8_t chip_track[16][CHIP_PLAYERS][MAX_TRACK_LENGTH];

struct chip_player 
{
    uint8_t wait;
    uint8_t cmd_index;
    uint8_t instrument;
    uint8_t max_drum_index;
    
    uint8_t note; // actual note being played    
    uint8_t track_note; // includes song_transpose
    uint8_t octave;
    uint8_t dutyd;

    uint8_t volume;
    int8_t volumed; 
    // TODO: add track_fade_behavior...
    // TODO: add a track_note_initial_volumed for things like reverse clamps
    // TODO: add a track_note_initial_volume for things like clamps and fades
    uint8_t track_volume;
    int8_t track_volumed; 

    // a u8 of the inst_fade_behavior_t:
    uint8_t fade_behavior;
    // for fade echo and other effects bouncing around:
    uint8_t fade_saved_max_volume;
    uint8_t fade_saved_min_volume;

    uint16_t inertia_slide; // internally how we keep track of note with inertia 
    uint8_t inertia;
    uint8_t track_inertia;
    
    uint8_t vibrato_depth;
    uint8_t track_vibrato_depth;
    uint8_t vibrato_rate;
    uint8_t track_vibrato_rate; 
    
    uint8_t vibrato_phase; 
    int8_t bendd;
    int16_t bend;

    uint8_t track_wait;
    uint8_t track_cmd_index;
    uint8_t track_index; // current track index, 0 - 15
    uint8_t next_track_index; // used for looping a track or playing next track in the song
};

void chip_reset_player(int i);
extern struct chip_player chip_player[CHIP_PLAYERS];

extern uint16_t chip_song[MAX_SONG_LENGTH];

// TODO: preface with chip_ and probably change pos to position or abs_position
extern uint8_t track_pos;
// TODO: preface with chip_
extern uint8_t track_length;

// TODO: preface with chip_
extern uint8_t song_transpose;
extern uint8_t song_length;
extern uint8_t song_speed;
// TODO: preface with chip_ and probably change pos to position or abs_position
extern uint8_t song_pos;

void chip_init();
void chip_reset();
void chip_kill();
void chip_play_song(int pos);
void chip_play_track(int track);
// TODO: chip_snapshot() and chip_restore() if people want to make a broken record sound

// play a note of this instrument now - useful for SFX !
void chip_play_note(uint8_t p, uint8_t inst, uint8_t note, uint8_t track_volume);

// TODO: use an enum instead, e.g. TrackCommandType, e.g. with TrackBreak
#define TRACK_BREAK 0
#define TRACK_OCTAVE 1
#define TRACK_INSTRUMENT 2
#define TRACK_VOLUME 3
#define TRACK_NOTE 4
#define TRACK_WAIT 5
// Maybe remove this and do something cooler, e.g. arpeggio
#define TRACK_NOTE_WAIT 6
// TODO: combine FADE_IN and OUT
#define TRACK_FADE_IN 7
#define TRACK_FADE_OUT 8
#define TRACK_INERTIA 9
#define TRACK_VIBRATO 10
#define TRACK_TRANSPOSE 11
#define TRACK_SPEED 12
#define TRACK_LENGTH 13
#define TRACK_RANDOMIZE 14
#define TRACK_JUMP 15

// TODO: preface with chip_ 
uint8_t instrument_max_index(uint8_t i, uint8_t j);
// TODO: preface with chip_, switch to sequence_invalid instead of jump_bad
int instrument_jump_bad(uint8_t inst, uint8_t max_index, uint8_t jump_from_index, uint8_t j);
int track_jump_bad(uint8_t t, uint8_t i, uint8_t jump_from_index, uint8_t j);

#endif

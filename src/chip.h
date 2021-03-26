/* Simple soundengine for the BitBox
 * Copyright 2014, Adrien Destugues <pulkomandy@pulkomandy.tk>
 * Copyright 2007, Linus Akesson
 * Based on the "Hardware Chiptune" project
 */
#ifndef CHIP_H
#define CHIP_H
#include <stdint.h>

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
{   WfSine = 0,  // 
    WfTriangle, // = /\/\,
    WfSaw, // = /|/| 
    WfPulse, // = |_|- (adjustable duty)
    WfNoise, // = !*@?
    WfRed, // = "integral" of WfNoise
    WfViolet, // = "derivative" of WfNoise
    // TODO: potentially we can add more, up to 16 total.
} wf_t;

struct oscillator {
    uint8_t pan;
    uint8_t volume;
    uint8_t waveform; // waveform (from the enum above)
    // TODO: remove, this doesn't do much
    uint8_t bitcrush; // 0-f level of quantization (power of 2) 

    // TODO: make this do something interesting for other waves:
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
    uint8_t track_volume;
    int8_t track_volumed; 

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

// TODO: use an enum instead, e.g. InstrumentCommandType, with e.g. InstrumentPan
#define BREAK 0
#define PAN 1
#define WAVEFORM 2
#define VOLUME 3
#define NOTE 4
#define WAIT 5
// TODO: combine FADE_IN and OUT
#define FADE_IN 6
#define FADE_OUT 7
#define INERTIA 8
#define VIBRATO 9
#define BEND 10
// Maybe remove this and do something cooler, e.g. repeat note every so often
#define BITCRUSH 11
#define DUTY 12
#define DUTY_DELTA 13
#define RANDOMIZE 14
#define JUMP 15

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

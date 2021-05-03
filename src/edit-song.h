#ifndef EDIT_SONG_H
#define EDIT_SONG_H

#include <stdint.h> // uint

extern uint8_t music_editor_in_menu;
extern const uint8_t hex_character[64];
extern const uint8_t note_name[12][2];

void editSong_start(int load_song);
void editSong_controls();
void editSong_line();

#endif


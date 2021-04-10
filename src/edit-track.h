#ifndef EDIT_TRACK_H
#define EDIT_TRACK_H

#include <stdint.h>

extern uint8_t editTrack_track;
extern uint8_t editTrack_player;

void editTrack_init();
void editTrack_load_defaults();
void editTrack_controls();
void editTrack_line();

#endif

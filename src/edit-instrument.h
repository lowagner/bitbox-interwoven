#ifndef EDIT_INSTRUMENT_H
#define EDIT_INSTRUMENT_H

#include <stdint.h> // uint

extern const uint8_t note_name[12][2];
extern uint8_t editInstrument_instrument;
extern uint8_t editInstrument_cmd_index;

void editInstrument_init();
void editInstrument_load_defaults();
void editInstrument_controls();
void editInstrument_line();

#endif

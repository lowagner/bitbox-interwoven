#ifndef IO_H
#define IO_H

#include <stdint.h>
#define MAX_FILES 1024

typedef enum {
    IoNoError = 0,
    IoMountError,
    IoConstraintError,
    IoOpenError,
    IoReadError,
    IoWriteError,
    IoNoDataError,
    IoMissingDataError,
} io_error_t;

typedef enum {
    IoEventNone = 0,
    IoEventInit,
    IoEventSave,
    IoEventLoad,
} io_event_t;

io_error_t io_init();
void io_message_from_error(uint8_t *msg, io_error_t error, io_event_t attempt);

extern uint8_t base_song_filename[9];

io_error_t io_load_recent_song_filename();
io_error_t io_load_recent_song();
io_error_t io_save_instrument(unsigned int i);
io_error_t io_load_instrument(unsigned int i);
io_error_t io_save_track(unsigned int i);
io_error_t io_load_track(unsigned int i);
io_error_t io_save_song();
io_error_t io_load_song();

#endif

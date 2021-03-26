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
} file_error_t;

typedef enum {
    IoTriedLoad,
    IoTriedSave,
    IoTriedInit,
} file_event_t;

file_error_t io_init();
void io_message_from_error(uint8_t *msg, file_error_t error, file_event_t attempt);

extern uint8_t base_song_filename[9];

file_error_t io_load_recent_song();
file_error_t io_save_instrument(unsigned int i);
file_error_t io_load_instrument(unsigned int i);
file_error_t io_save_track(unsigned int i);
file_error_t io_load_track(unsigned int i);
file_error_t io_save_song();
file_error_t io_load_song();

#endif

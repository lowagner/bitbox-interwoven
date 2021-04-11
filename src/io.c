#include "bitbox.h"
#include "chip.h"
#include "game.h"
#include "io.h"
#include "name.h"

#include "fatfs/ff.h"

#define RECENT_MUSIC_FILE "LASTM16.TXT"

#define INSTRUMENTS_BYTE_OFFSET (0)
#define INSTRUMENTS_BYTE_STRIDE (1 /* drum/octave */ + MAX_INSTRUMENT_LENGTH /* commands */)
#define INSTRUMENTS_BYTE_LENGTH (16 /* # instruments */ * INSTRUMENTS_BYTE_STRIDE)
#define TRACKS_BYTE_OFFSET (INSTRUMENTS_BYTE_OFFSET + INSTRUMENTS_BYTE_LENGTH)
#define TRACKS_BYTE_STRIDE (CHIP_PLAYERS /* # players */ * MAX_TRACK_LENGTH)
#define TRACKS_BYTE_LENGTH (MAX_TRACKS * TRACKS_BYTE_STRIDE)
#define SONG_BYTE_OFFSET (TRACKS_BYTE_OFFSET + TRACKS_BYTE_LENGTH)
#define SONG_BYTE_LENGTH (1 /* length of song */ + sizeof(chip_song))
#define TOTAL_FILE_SIZE (SONG_BYTE_OFFSET + SONG_BYTE_LENGTH)

#define SAVE_INDEXED(name, i, max) \
    io_error_t ferr = io_save_recent_song_filename(); \
    if (ferr) \
        return ferr; \
    message("opening %s to load " #name " %d / " #max " with stride %d\n", full_song_filename, i, name##_BYTE_STRIDE); \
    ferr = io_open_or_zero_file(full_song_filename, TOTAL_FILE_SIZE); \
    if (ferr) \
        return ferr; \
    if (i >= max) \
    {   f_lseek(&fat_file, name##_BYTE_OFFSET); \
        for (i=0; i<max; ++i) \
          if ((ferr = _io_save_##name(i))) \
            break; \
    } \
    else \
    {   f_lseek(&fat_file, name##_BYTE_OFFSET + i*(name##_BYTE_STRIDE));  \
        ferr = _io_save_##name(i); \
    } \
    f_close(&fat_file); \
    message("done loading " #name "\n"); \
    return ferr

#define LOAD_INDEXED(name, i, max) \
    io_error_t ferr = io_save_recent_song_filename(); \
    if (ferr) \
        return ferr; \
    message("opening %s to load " #name " %d / " #max " with stride %d\n", full_song_filename, i, name##_BYTE_STRIDE); \
    fat_result = f_open(&fat_file, (char *)full_song_filename, FA_READ | FA_OPEN_EXISTING); \
    if (fat_result != FR_OK) \
        return IoOpenError; \
    if (i >= max) \
    {   f_lseek(&fat_file, name##_BYTE_OFFSET); \
        for (i=0; i<max; ++i) \
          if ((ferr = _io_load_##name(i))) \
            break; \
    } \
    else \
    {   f_lseek(&fat_file, name##_BYTE_OFFSET + i*(name##_BYTE_STRIDE));  \
        ferr = _io_load_##name(i); \
    } \
    f_close(&fat_file); \
    return ferr

int io_mounted = 0;
FATFS fat_fs;
FIL fat_file;
FRESULT fat_result;

// Song-related 
uint8_t saved_base_song_filename[9];
uint8_t saved_chip_volume;
/// FatFS uses 8 characters pre-extension, but leave space for the null terminator:
uint8_t base_song_filename[9];
uint8_t full_song_filename[13];

// TODO: add parameter n for how many bytes can be copied into msg
void io_message_from_error(uint8_t *msg, io_error_t error, io_event_t attempt)
{   // Copies a short error message into msg based on file error, depending on save or load
    // msg is assumed to be `game_message` (or a small offset away), so we also set game_message_timeout
    switch (error)
    {   case IoNoError:
            // TODO: support other io_event_t types
            if (attempt == IoEventSave)
                strcpy((char *)msg, "saved!");
            else
                strcpy((char *)msg, "loaded!");
            break;
        case IoMountError:
            strcpy((char *)msg, "fs unmounted!");
            break;
        case IoConstraintError:
            strcpy((char *)msg, "unconstrained!");
            break;
        case IoOpenError:
            strcpy((char *)msg, "no open!");
            break;
        case IoReadError:
            strcpy((char *)msg, "no read!");
            break;
        case IoWriteError:
            strcpy((char *)msg, "no write!");
            break;
        case IoNoDataError:
            strcpy((char *)msg, "no data!");
            break;
        case IoMissingDataError:
            strcpy((char *)msg, "miss data!");
            break;
        default:
            strcpy((char *)msg, "fully bungled.");
            break;
    }
    game_message_timeout = 0;
}

io_error_t io_init()
{   // Initializes the input-output module.  Can return an error if filesystem not mountable.
    saved_base_song_filename[0] = 0;
    fat_result = f_mount(&fat_fs, "", 1); // mount now...
    if (fat_result != FR_OK)
    {   io_mounted = 0;
        return IoMountError;
    }
    io_mounted = 1;
    return IoNoError;
}

static io_error_t io_open_or_zero_file(const uint8_t *fname, unsigned int size)
{   // Opens a file, or creates a new file with zeros with a given size
    // so that we can seek to write anywhere within that size.
    // Leaves the file open for users to write to.
    if (size == 0)
        return IoNoDataError;

    // Assume an existing file can be seeked...
    fat_result = f_open(&fat_file, (char *)fname, FA_WRITE | FA_OPEN_EXISTING);
    if (fat_result == FR_OK)
        return IoNoError;

    fat_result = f_open(&fat_file, (char *)fname, FA_WRITE | FA_OPEN_ALWAYS);
    if (fat_result != FR_OK)
        return IoOpenError;

    uint8_t zero[128] = {0};
    while (size) 
    {   // Iteratively zero out the file so that we don't get any bitbox hangs:
        UINT write_size = size <= 128 ? size : 128;
        size -= write_size;

        UINT bytes_get;
        f_write(&fat_file, zero, write_size, &bytes_get);
        if (bytes_get != write_size)
        {   f_close(&fat_file);
            return IoMissingDataError;
        }
    }
    return IoNoError;
}

static io_error_t io_save_recent_song_filename()
{   // Saves base_song_filename to a recent music file so that we can load it without user-input on next boot.
    // Also updates full_song_filename to have the correct name (from base_song_filename) and extension for
    // saving/loading a song, so this method must be called before doing any FatFS manipulation
    // in all other save/load music functions.
    message("checking if we need to update recent song filename: %s\n", base_song_filename);
    int filename_len = strlen((char *)base_song_filename);
    if (filename_len == 0)
        return IoConstraintError;

    if (io_mounted == 0 && io_init())
        return IoMountError;
    
    if (strcmp((char *)saved_base_song_filename, (char *)base_song_filename) == 0 && chip_volume == saved_chip_volume)
        return IoNoError; // don't rewrite  

    message(">> saving recent song filename to disk...\n");
    for (int i=0; i<8; ++i)
    {   // Copy over base_song_filename to full_song_filename:
        if (base_song_filename[i] != 0)
        {   full_song_filename[i] = base_song_filename[i];
            continue;
        }
        full_song_filename[i] = '.';
        full_song_filename[++i] = 'M';
        full_song_filename[++i] = '1';
        full_song_filename[++i] = '6';
        full_song_filename[++i] = 0;
        break;
    }

    // TODO: abstract the f_open to f_write calls so there's less boilerplate??
    fat_result = f_open(&fat_file, RECENT_MUSIC_FILE, FA_WRITE | FA_CREATE_ALWAYS); 
    if (fat_result != FR_OK) 
        return IoOpenError;

    UINT bytes_get;
    fat_result = f_write(&fat_file, (char *)base_song_filename, filename_len, &bytes_get);
    if (fat_result != FR_OK)
    {   f_close(&fat_file);
        return IoWriteError;
    }
    if (bytes_get != filename_len)
    {   f_close(&fat_file);
        return IoMissingDataError;
    }
    strcpy((char *)saved_base_song_filename, (char *)base_song_filename);
    uint8_t msg[2] = { '\n', chip_volume };
    fat_result = f_write(&fat_file, msg, 2, &bytes_get);
    f_close(&fat_file);
    if (fat_result != FR_OK)
        return IoWriteError;
    if (bytes_get != 2)
        return IoMissingDataError;
    saved_chip_volume = chip_volume;

    return IoNoError;
}

io_error_t io_load_recent_song_filename()
{   // Loads the song name and volume from the RECENT_MUSIC_FILE file.
    message("loading recent song filename\n");
    if (io_mounted == 0 && io_init())
        return IoMountError;

    fat_result = f_open(&fat_file, RECENT_MUSIC_FILE, FA_READ | FA_OPEN_EXISTING); 
    if (fat_result != FR_OK) 
        return IoOpenError;

    UINT bytes_get;
    uint8_t buffer[10];
    fat_result = f_read(&fat_file, &buffer[0], 10, &bytes_get); 
    f_close(&fat_file);
    if (fat_result != FR_OK)
        return IoReadError;
    if (bytes_get == 0)
        return IoNoDataError;

    int i=0;
    while (i < bytes_get)
    {
        if (buffer[i] == '\n')
        {
            base_song_filename[i] = 0;
            if (++i < bytes_get)
                chip_volume = buffer[i];
            message(">> got volume %d and recent song filename: \"%s\"\n", chip_volume, base_song_filename);
            return IoNoError;
        }
        if (i < 8)
            base_song_filename[i] = buffer[i];
        else
        {
            base_song_filename[8] = 0;
            return IoConstraintError;
        }
        ++i;
    }
    base_song_filename[i] = 0;
    message(">> got recent song filename: \"%s\"\n", base_song_filename);
    return IoNoError;
}

io_error_t io_load_recent_song()
{   // Attempts to load the most recently-used song.
    io_error_t error = io_load_recent_song_filename();
    if (error)
        return error;
    return io_load_song();
}

static io_error_t _io_load_INSTRUMENTS(unsigned int i)
{   // Loads an instrument.  i should be between 0 and 15 (inclusive).
    UINT bytes_get;
    uint8_t read;
    message(">> loading instrument %d\n", i);
    fat_result = f_read(&fat_file, &read, 1, &bytes_get);
    if (fat_result != FR_OK)
        return IoReadError;
    if (bytes_get != 1)
        return IoMissingDataError;
    instrument[i].is_drum = read&15;
    instrument[i].octave = read >> 4;
    fat_result = f_read(&fat_file, &instrument[i].cmd[0], MAX_INSTRUMENT_LENGTH, &bytes_get);
    if (fat_result != FR_OK)
        return IoReadError;
    if (bytes_get != MAX_INSTRUMENT_LENGTH)
        return IoMissingDataError;
    
    return IoNoError;
}

static io_error_t _io_save_INSTRUMENTS(unsigned int i)
{   // Saves an instrument.  i should be between 0 and 15 (inclusive).
    UINT bytes_get; 
    uint8_t write = (instrument[i].is_drum ? 1 : 0) | (instrument[i].octave << 4);
    fat_result = f_write(&fat_file, &write, 1, &bytes_get);
    if (fat_result != FR_OK)
        return IoWriteError;
    if (bytes_get != 1)
        return IoMissingDataError;
    fat_result = f_write(&fat_file, &instrument[i].cmd[0], MAX_INSTRUMENT_LENGTH, &bytes_get);
    if (fat_result != FR_OK)
        return IoWriteError;
    if (bytes_get != MAX_INSTRUMENT_LENGTH)
        return IoMissingDataError;
    
    return IoNoError;
}

io_error_t io_load_instrument(unsigned int i)
{   // Loads an instrument (or multiple if i >= 16).
    LOAD_INDEXED(INSTRUMENTS, i, 16);
}

io_error_t io_save_instrument(unsigned int i)
{   // Saves an instrument (or multiple if i >= 16).
    SAVE_INDEXED(INSTRUMENTS, i, 16);
}

io_error_t _io_load_TRACKS(unsigned int i)
{   // Loads a track.  i should be between 0 and 31 (inclusive).
    message(">> loading track %d\n", i);
    UINT bytes_get; 
    fat_result = f_read(&fat_file, chip_track[i], TRACKS_BYTE_STRIDE, &bytes_get);
    if (fat_result != FR_OK)
        return IoReadError;
    if (bytes_get != TRACKS_BYTE_STRIDE)
        return IoMissingDataError;
    return IoNoError;
}

io_error_t _io_save_TRACKS(unsigned int i)
{   // Saves a track.  i should be between 0 and 31 (inclusive).
    UINT bytes_get; 
    fat_result = f_write(&fat_file, chip_track[i], TRACKS_BYTE_STRIDE, &bytes_get);
    if (fat_result != FR_OK)
        return IoWriteError;
    if (bytes_get != sizeof(chip_track[0]))
        return IoMissingDataError;
    return IoNoError;
}

io_error_t io_load_track(unsigned int i)
{   // Loads a track (or multiple if i >= 16).
    LOAD_INDEXED(TRACKS, i, MAX_TRACKS);
}

io_error_t io_save_track(unsigned int i)
{   // Saves a track (or multiple if i >= MAX_TRACKS).
    SAVE_INDEXED(TRACKS, i, MAX_TRACKS);
}

io_error_t io_load_song()
{   // Loads the song.

    // set some defaults
    chip_track_length = 32;
    song_speed = 4;
    song_length = 16;

    io_error_t ferr = io_save_recent_song_filename();
    if (ferr)
        return ferr;

    fat_result = f_open(&fat_file, (char *)full_song_filename, FA_READ | FA_OPEN_EXISTING); 
    if (fat_result)
        return IoOpenError;
    
    f_lseek(&fat_file, SONG_BYTE_OFFSET);

    UINT bytes_get; 
    fat_result = f_read(&fat_file, &song_length, 1, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return IoReadError;
    }
    if (bytes_get != 1)
    {
        f_close(&fat_file);
        return IoMissingDataError;
    }

    if (song_length < 16 || song_length > MAX_SONG_LENGTH)
    {
        message("got song length %d\n", song_length);
        song_length = 16;
        f_close(&fat_file);
        return IoConstraintError;
    }
    
    fat_result = f_read(&fat_file, &chip_song[0], 2*song_length, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return IoReadError;
    }
    if (bytes_get != 2*song_length)
    {
        f_close(&fat_file);
        return IoMissingDataError;
    }

    f_close(&fat_file);
    return IoNoError;
}

io_error_t io_save_song()
{   // Saves the song.

    io_error_t ferr = io_save_recent_song_filename();
    if (ferr)
        return ferr;

    ferr = io_open_or_zero_file(full_song_filename, TOTAL_FILE_SIZE);
    if (ferr)
        return ferr;
    
    f_lseek(&fat_file, SONG_BYTE_OFFSET);

    UINT bytes_get; 
    fat_result = f_write(&fat_file, &song_length, 1, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return IoWriteError;
    }
    if (bytes_get != 1)
    {
        f_close(&fat_file);
        return IoMissingDataError;
    }
    
    fat_result = f_write(&fat_file, &chip_song[0], 2*song_length, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return IoWriteError;
    }
    if (bytes_get != 2*song_length)
    {
        f_close(&fat_file);
        return IoMissingDataError;
    }

    f_close(&fat_file);
    return IoNoError;
}

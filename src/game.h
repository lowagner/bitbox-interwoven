#ifndef GAME_H
#define GAME_H
#include "bitbox.h"
#include <stdint.h>
#include <string.h>

#define SCREEN_W 320
#define SCREEN_H 240

#ifdef EMULATOR
#define EMU_ONLY(x) x
#else
#define EMU_ONLY(x) {}
#endif

#define ASSERT(x) EMU_ONLY({if (!(x)) \
{   message(#x " was not true at " __FILE__ ":%d!\n", __LINE__); \
    bitbox_die(-1, 0); \
}})

#define GAMEPAD_PRESS_WAIT 8
#define GAMEPAD_PRESS(id, key) ((gamepad_buttons[id]) & (~old_gamepad[id]) & (gamepad_##key))
#define GAMEPAD_PRESSING(id, key) ((gamepad_buttons[id]) & (gamepad_##key) & (~old_gamepad[id] | ((gamepad_press_wait == 0)*gamepad_##key)))

extern uint16_t new_gamepad[2];
extern uint16_t old_gamepad[2];
extern uint8_t gamepad_press_wait;

typedef enum {
    ModeNone=0,
    // TODO: add skippable intro
    ModeMainMenu,
    ModeNameSong,
    ModeEditInstrument,
    ModeEditSong,
    ModeEditTrack,
} game_mode_t;

extern game_mode_t game_mode;
extern game_mode_t previous_game_mode;

void game_switch(game_mode_t new_game_mode);
void game_switch_to_previous_or(game_mode_t new_game_mode);

extern uint16_t game_palette[255 + 15 + 1];
extern uint8_t game_message[32];
extern int game_message_timeout;

void game_set_message_with_timeout(const char *msg, int timeout);

#define MESSAGE_TIMEOUT (10*64)

#define LL_NEW(container, next_name, previous_name, MAX_COUNT) \
    uint8_t index = container[0].next_free; \
    if (!index) \
        return 0; \
    ASSERT(index < MAX_COUNT); \
    container##_t *new_element = &container[index]; \
    \
    /* update the free list: */ \
    container[0].next_free = new_element->next_free; \
    \
    /* do the insertion: */ \
    uint8_t next = container[0].next_object; \
    new_element->##next_name = next; \
    new_element->##previous_name = 0; \
    container[next].##previous_name = index; \
    \
    return index;

#define LL_FREE(container, index) \
    container[index].next_free = container[0].next_free; \
    container[0].next_free = index;

#endif

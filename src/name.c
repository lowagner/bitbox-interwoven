#include "bitbox.h"
#include "font.h"
#include "game.h"
#include "name.h"

#define OFFSET_X 230 // offset for alphabet square
#define TEXT_OFFSET ((16+6*9))
#define BG_COLOR 32 // 32/128 is dark red/green

uint8_t name_position; // position in the base filename

uint8_t name_x, name_y; // position in alphabet table (below):
static const uint8_t allowed_chars[6][7] = {
    {'E', 'T', 'A', 'O', 'I', 'N', 0},
    {'S', 'R', 'H', 'L', 'D', 'C', 0},
    {'U', 'M', 'W', 'F', 'G', 'P', 0},
    {'Y', 'B', 'V', 'K', 'X', 'J', 0},
    {'Q', 'Z', '0', '1', '2', '3', 0},
    {'4', '5', '6', '7', '8', '9', 0},
};

#define NUMBER_ROWS 14
#define BOX_COLOR RGB(205, 100, 0)
#define MATRIX_WING_COLOR RGB(255, 255, 0)

// TODO: make a cancel button
uint8_t *name_to_modify;
int name_max_length;
game_mode_t name_mode_after_naming;

void name_start(game_mode_t next_mode, uint8_t *name, int max_length)
{   // Initializes name GUI to modify the passed-in name, and send back to
    // `next_mode` after finishing.  Also ensures putting a null-terminator before max_length.
    // TODO: allow lower-case letters as an option, but not for file names
    // TODO: allow passing in a title.  e.g. "choose your name"
    name_mode_after_naming = next_mode;
    name_to_modify = name;
    name_max_length = max_length;
    // Ensure name is under a certain length:
    int i;
    for (i = 0; i < name_max_length - 1; ++i)
    {   if (name[i] == 0)
            break;
    }
    name[i] = 0;
    name_position = i;
    message("starting name_start with name = \"%s\" / %d\n", name, max_length);
}

void name_line()
{   // Draw the GUI for the choose-name mode:
    if (vga_line < 22)
    {   // Show a blank strip across the top:
        if (vga_line/2 == 0)
            memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
        return;
    }
    if (vga_line >= 22 + NUMBER_ROWS*10)
    {   // Draw a blank line across the bottom of the page:
        if (vga_line/2 == (22 + NUMBER_ROWS*10)/2)
            memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
        return;
    }
    int row = (vga_line-22) / 10;
    int delta_y = (vga_line-22) % 10;
    if (delta_y >= 8)
    {
        memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
        if (delta_y % 2) return;
        // also check for character selector
        if (row == 0)
        {
            // spot in the filename to write to
            uint16_t *dst = draw_buffer + TEXT_OFFSET + 1 + name_position * 9;
            const uint16_t color = BOX_COLOR;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
        }
        else if (name_y+1 == row)
        {
            uint16_t *dst = draw_buffer + 1 + name_x * 9 + OFFSET_X;
            const uint16_t color = BOX_COLOR;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
        }
        else if (row == 6 && delta_y == 8)
        {
            uint16_t *dst = draw_buffer + 1 + name_x * 9 + OFFSET_X;
            const uint16_t color = MATRIX_WING_COLOR;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
        }
    }
    else switch (row)
    {   case 0:
            font_render_line_doubled((const uint8_t *)"name:", 16, delta_y, 65535, BG_COLOR*257);
            font_render_line_doubled(name_to_modify, TEXT_OFFSET, delta_y, 65535, BG_COLOR*257);
            break;
        case 7:
            font_render_line_doubled((const uint8_t *)"A:insert Y:overwrite", 16, delta_y, 65535, BG_COLOR*257);
            break;
        case 8:
            font_render_line_doubled((const uint8_t *)"B:backspace X:delete", 16, delta_y, 65535, BG_COLOR*257);
            break;
        case 9:
            font_render_line_doubled((const uint8_t *)"<L/R>:move cursor", 16, delta_y, 65535, BG_COLOR*257);
            break;
        case 11:
            font_render_line_doubled((const uint8_t *)"start:finish", 16, delta_y, 65535, BG_COLOR*257);
            break;
        case 12:
            if (GAMEPAD_HOLDING(0, select))
                font_render_line_doubled((const uint8_t *)"select+\x7f:debug", 16, delta_y, 65535, BG_COLOR*257);
            break;
        default:
            if (row > 6)
                break;
            font_render_line_doubled((const uint8_t *)allowed_chars[row-1], OFFSET_X, delta_y, 65535, BG_COLOR*257);

            if (row-1 != name_y)
                break;
            if (name_x < 5)
            {
                {
                uint16_t *dst = draw_buffer + (name_x * 9 + OFFSET_X);
                const uint16_t color = BOX_COLOR;
                *dst = color;
                dst += 9;
                *dst = color;
                }
                {
                uint32_t *dst = (uint32_t *)draw_buffer + (1 + 6 * 9 + OFFSET_X)/2;
                const uint32_t color = MATRIX_WING_COLOR | ((BG_COLOR*257)<<16);
                *dst++ = color;
                *dst++ = color;
                *dst++ = color;
                *dst++ = color;
                *dst++ = color;
                }
            }
            else
            {
                {
                uint16_t *dst = draw_buffer + (name_x * 9 + OFFSET_X);
                const uint16_t color = BOX_COLOR;
                *dst = color;
                }
                {
                uint32_t *dst = (uint32_t *)draw_buffer + (1 + 6 * 9 + OFFSET_X)/2;
                const uint32_t color = BOX_COLOR | ((BG_COLOR*257)<<16);
                *dst++ = color;
                *dst++ = color;
                *dst++ = color;
                *dst++ = color;
                *dst++ = color;
                }
            }
            break;
    }
}

static inline void name_overwrite_character()
{   // Overwrites a character without changing neighboring characters, increments cursor.

    // Do not overwrite the null byte:
    if (name_position >= name_max_length - 1) 
        return;

    name_to_modify[name_position++] = allowed_chars[name_y][name_x];
}

static inline void name_backspace_character()
{   // Deletes character before cursor, shifts everything down and moves cursor back.

    // If nothing before, do nothing.
    if (!name_position) return;

    for (int i=--name_position; 1; ++i)
    {   // Shift down other characters into new name_position and above.

        // ensure the end is zeroed out in case i went up too high. 
        if (i >= name_max_length - 1)
        {   name_to_modify[name_max_length - 1] = 0;
            break;
        }

        name_to_modify[i] = name_to_modify[i+1];

        if (name_to_modify[i] == 0)
            break;
    }
}

static inline void name_delete_character()
{   // Deletes current character (and moves other elements down), don't move cursor.

    if (name_position >= name_max_length - 1) return;

    for (int i=name_position; 1; ++i)
    {   // Shift down other characters into name_position and above.

        // ensure the end is zeroed out in case i went up too high. 
        if (i >= name_max_length - 1)
        {   name_to_modify[name_max_length - 1] = 0;
            break;
        }

        name_to_modify[i] = name_to_modify[i+1];

        if (name_to_modify[i] == 0)
            break;
    }
}

static inline void name_insert_character()
{   // Inserts character (and moves up everything else).  Increments cursor.

    if (name_position >= name_max_length - 1) return;

    for (int i = name_max_length - 2; i >= name_position; --i)
    {   // Move up:
        name_to_modify[i + 1] = name_to_modify[i];
    }

    // Ensure end is zeroed out.
    name_to_modify[name_max_length - 1] = 0;

    // Actually insert character:
    name_to_modify[name_position++] = allowed_chars[name_y][name_x];
}

void name_controls()
{   // Checks user input
    if (GAMEPAD_PRESS(0, start))
    {   game_switch(name_mode_after_naming);
        return;
    }
    if (GAMEPAD_HOLDING(0, select))
    {   if (GAMEPAD_PRESS(0, down))
            game_switch(ModeDebugSprite);
        return;
    }

    int make_wait = 0;
    if (GAMEPAD_PRESSING(0, left))
    {   // Navigate the ABC
        if (name_x)
            --name_x;
        else
            name_x = 5;
        make_wait = 1;
    }
    if (GAMEPAD_PRESSING(0, right))
    {   // Navigate the ABC
        if (name_x < 5)
            ++name_x;
        else
            name_x = 0;
        make_wait = 1;
    }
    if (GAMEPAD_PRESSING(0, up))
    {   // Navigate the ABC
        if (name_y)
            --name_y;
        else
            name_y = 5;
        make_wait = 1;
    }
    if (GAMEPAD_PRESSING(0, down))
    {   // Navigate the ABC
        if (name_y < 5)
            ++name_y;
        else
            name_y = 0;
        make_wait = 1;
    }
    if (make_wait)
        gamepad_press_wait[0] = GAMEPAD_PRESS_WAIT;

    if (GAMEPAD_PRESS(0, A))
    {   name_insert_character();
        game_message[0] = 0;
        return;
    }
    if (GAMEPAD_PRESS(0, B))
    {   name_backspace_character();
        game_message[0] = 0;
        return;
    }
    if (GAMEPAD_PRESS(0, X))
    {   name_delete_character();
        game_message[0] = 0;
        return;
    }
    if (GAMEPAD_PRESS(0, Y))
    {   name_overwrite_character();
        game_message[0] = 0;
        return;
    }
    if (GAMEPAD_PRESS(0, L))
    {   if (name_position)
            --name_position;
        game_message[0] = 0;
        return;
    } 
    if (GAMEPAD_PRESS(0, R))
    {   if (name_to_modify[name_position] != 0 && name_position < name_max_length - 1)
            ++name_position;
        game_message[0] = 0;
        return;
    }
}

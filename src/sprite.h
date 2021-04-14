#ifndef SPRITE_H
#define SPRITE_H

#include <stdint.h>

#define MAX_SPRITES 128 // technically we use one (0) as the head of the linked list.

typedef enum
{   // shape with coloring information after the underscore.
    NoShape_Invisible,
    Rectangle_TopHalfBottomHalf,
    /* TODO
    Rectangle_LeftRight,
    Ellipse_TopHalfBottomHalf,
    Ellipse_LeftRight,
    Ellipse_DiagonalForwardSlash,
    Ellipse_DiagonalBackSlash,
    // E.g. for round shields:
    Ellipse_ThickBorder,
    Shield_ThickBorder,
    SwordProfileAngle0_HandAndHilt,
    SwordProfileAngle1_HandAndHilt,
    SwordProfileAngle2_HandAndHilt,
    SwordProfileAngle3_HandAndHilt,
    SwordProfileAngle4_HandAndHilt,
    SwordProfileAngle5_HandAndHilt,
    SwordProfileAngle6_HandAndHilt,
    SwordProfileAngle7_HandAndHilt,
    SwordProfileAngle8_HandAndHilt,
    SwordProfileAngle9_HandAndHilt,
    SwordProfileAngleA_HandAndHilt,
    SwordProfileAngleB_HandAndHilt,
    SwordProfileAngleC_HandAndHilt,
    SwordProfileAngleD_HandAndHilt,
    SwordProfileAngleE_HandAndHilt,
    SwordProfileAngleF_HandAndHilt,
    */
    // DO NOT USE PAST THIS POINT
    ShapeCount,
} sprite_shape_t;

extern uint16_t sprite_palette[16];

typedef struct sprite
{   // struct holding sprite information for drawing on screen.

    // 32 bits for linked list stuff:
    // updated in sprite_line() and sprite_frame(), do not change outside of these.
    uint8_t next_to_draw;       // linking sprites which should be drawn next -
    uint8_t previous_to_draw;   // - as ordered from top to bottom (small iy to large iy)
    uint8_t next_visible;       // linking sprites which are currently being drawn on this vga_line -
    uint8_t previous_visible;   // - ordered in z (small z to big z)
    // 32 bits:
    float iz;    // for depth, whatever should be drawn first is lower in z.
    // 32 bits:
    int16_t iy, ix;
    // 32 bits:
    uint8_t width, height;
    union
    {   uint8_t colors;  // first nibble for color1, second nibble for color2
        uint8_t check_next_for_visible; // only used in root sprite (sprite[0]).
    };
    union
    {   uint8_t shape;
        uint8_t next_free; // next free sprite index, only used in the free (unused) list of sprites.
    };
    // try to keep to 4x32 bits = 4x4 bytes = 16 bytes
} sprite_t;

extern sprite_t sprite[MAX_SPRITES];

uint8_t sprite_new();
void sprite_free(uint8_t index);

void sprite_init();
void sprite_line();
void sprite_frame();

#endif

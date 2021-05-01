#ifndef TWENTY_FOURTY_EIGHT_H
#define TWENTY_FOURTY_EIGHT_H

#include <stdint.h>

#define TWENTY_FOURTY_EIGHT_MAX_HEIGHT 8
#define TWENTY_FOURTY_EIGHT_MAX_WIDTH 8
#define TWENTY_FOURTY_EIGHT_MAX_SIZE [TWENTY_FOURTY_EIGHT_MAX_HEIGHT][TWENTY_FOURTY_EIGHT_MAX_WIDTH]

typedef enum
{   CardinalNone = 0,
    CardinalEast,
    CardinalNorth,
    CardinalWest,
    CardinalSouth,
} cardinal_movement_t;

typedef enum
{   TransitionNone = 0,
    TransitionMerging,
    TransitionStuck,
} transition_t;

typedef struct twenty_fourty_eight_board
{   uint8_t width, height;
    // New tile logic:
    // bit values 1, 2, 4, and 8 (bits 0-3) correspond to whether to spawn those tiles.
    // bits 4-5 indicates how the value of a new tile is chosen
    //  0 - pseudo-randomly, based on time (vga_frame)
    //  1 - deterministically, based on current round, e.g. use a unique LFSR
    //  2 - deterministically, based on a pattern (e.g. 2 2 4 2 2 4 2 2 4)
    //  3 - by the user
    // bits 6-7 indicates how a new tile's location is chosen
    //  0 - pseudo-randomly, based on time (vga_frame)
    //  1 - deterministically, based on current round (e.g. use a unique LFSR)
    //  2 - deterministically, first open spot (from bottom-right, going right-to-left then up)
    //  3 - by the user
    uint8_t new_tile_behavior;
    // Use cardinal_movement_t for which direction:
    uint8_t transitioning;

    uint32_t round; 
   
    // When transitioning, capture what the current block is doing.  use transition_t
    uint8_t block_transition_state TWENTY_FOURTY_EIGHT_MAX_SIZE;
    // Current value of the block.  0 is empty, and the rest are 2**(value - 1).
    // e.g. value = 1 --> block = 1
    //      value = 2 --> block = 2
    //      value = 3 --> block = 4
    //      value = 4 --> block = 8
    //      etc.
    uint8_t block_value TWENTY_FOURTY_EIGHT_MAX_SIZE;
    uint8_t previous_block_value TWENTY_FOURTY_EIGHT_MAX_SIZE;
} twenty_fourty_eight_board_t;

extern twenty_fourty_eight_board_t twenty_fourty_eight_board;

void twentyFourtyEight_reset();
// TODO: void twentyFourtyEight_load(const char *filename)
// TODO: void twentyFourtyEight_save(const char *filename)
void twentyFourtyEight_controls();
void twentyFourtyEight_line();

#endif

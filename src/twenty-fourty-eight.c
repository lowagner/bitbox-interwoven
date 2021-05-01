#include "bitbox.h"
#include "game.h"
#include "twenty-fourty-eight.h"

twenty_fourty_eight_board_t twenty_fourty_eight_board CCM_MEMORY;

void twentyFourtyEight_reset()
{   memset(twenty_fourty_eight_board, 0, sizeof(twenty_fourty_eight_board_t));
}

int twentyFourtyEight_spawn_tile()
{   // adds a tile to the board, returning 1 if successful, 0 if not.
    // 0 means you should end the game...
    int allowed_spots[TWENTY_FOURTY_EIGHT_MAX_HEIGHT * TWENTY_FOURTY_EIGHT_MAX_WIDTH];
    int allowed_count = 0;

    ASSERT(twenty_fourty_eight_board.transitioning == CardinalNone);
    ASSERT(twenty_fourty_eight_board.width <= TWENTY_FOURTY_EIGHT_MAX_WIDTH);
    ASSERT(twenty_fourty_eight_board.height <= TWENTY_FOURTY_EIGHT_MAX_HEIGHT);

    for (int j = 0; j < twenty_fourty_eight_board.height; ++j)
    for (int i = 0; i < twenty_fourty_eight_board.width; ++i)
    {   if (twenty_fourty_eight_board.block_value[j][i] == 0)
        {   allowed_spots[allowed_count++] = j * TWENTY_FOURTY_EIGHT_MAX_WIDTH + i;
        }
    }
    if (allowed_count == 0)
        return 0;
}

void twentyFourtyEight_controls()
{
}

void twentyFourtyEight_line()
{
}

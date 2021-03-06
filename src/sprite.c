#include "bitbox.h"
#include "game.h"
#include "sprite.h"

#include <assert.h>
#include <stdlib.h> // rand

uint16_t sprite_palette[16] CCM_MEMORY;
struct sprite sprite[MAX_SPRITES] CCM_MEMORY;

void sprite_init()
{   // setup the sprites to have the correct free linked list.
    LL_RESET(sprite, next_to_draw, previous_to_draw, MAX_SPRITES);
    STATIC_ASSERT(ShapeCount <= 256);
    STATIC_ASSERT(MAX_SPRITES <= 256);

    // Set the palette to something reasonable.
    static const uint16_t colors[16] = {
        RGB(0, 0, 0),
        RGB(157, 157, 157),
        (1<<15) - 1,
        RGB(224, 111, 139),
        RGB(190, 38, 51),
        RGB(235, 137, 49),
        RGB(164, 100, 34),
        RGB(73, 60, 43),
        RGB(247, 226, 107),
        RGB(163, 206, 39),
        RGB(68, 137, 26),
        RGB(47, 72, 78),
        RGB(178, 220, 239),
        RGB(49, 162, 242),
        RGB(0, 87, 132),
        RGB(28, 20, 40),
    };
    memcpy(sprite_palette, colors, sizeof(colors));
}

uint8_t sprite_new()
{   // returns an index to a free sprite, or zero if none.
    // add new sprite to the draw_order list;
    // it's not sorted yet but that will happen in the sprite_frame();
    // users haven't updated the sprite details yet anyway so it doesn't
    // make sense to try and find any other spot for it yet.
    // the visible list shouldn't be in use yet (that is used only in sprite_line())
    // so we just have to update the draw_order list.
    LL_NEW(sprite, next_to_draw, previous_to_draw, MAX_SPRITES);
}

static inline void sprite_detach_from_draw(uint8_t index)
{   // removes sprite[index] from its current place in the linked list;
    // this function DOES NOT update sprite[index].next/prev;
    // that is something you should do based on where you are trying to attach it.
    uint8_t next_to_draw = sprite[index].next_to_draw;
    uint8_t previous_to_draw = sprite[index].previous_to_draw;

    ASSERT(sprite[previous_to_draw].next_to_draw == index);
    sprite[previous_to_draw].next_to_draw = next_to_draw;
    ASSERT(sprite[next_to_draw].previous_to_draw == index);
    sprite[next_to_draw].previous_to_draw = previous_to_draw;
}

static inline void sprite_attach_to_draw(uint8_t index, uint8_t previous_to_draw)
{   // inserts sprite[index] in linked list between sprite[previous_to_draw] and sprite[previous_index].next_to_draw.
    // assumes that sprite[index]'s current outgoing connections don't need to be updated.
    // we should only do sprite_attach_to_draw(index, ...) after a corresponding sprite_detach_from_draw(index).

    // update linkages from outside sprite[index] towards sprite[index]:
    uint8_t next_to_draw = sprite[previous_to_draw].next_to_draw;
    sprite[previous_to_draw].next_to_draw = index;
    ASSERT(sprite[next_to_draw].previous_to_draw == previous_to_draw);
    sprite[next_to_draw].previous_to_draw = index;

    // update the linkages from sprite[index] outward:
    sprite[index].next_to_draw = next_to_draw;
    sprite[index].previous_to_draw = previous_to_draw;
}

void sprite_free(uint8_t index)
{   // frees the sprite at the given index; its values should not be modified any more.

    // remove from draw_order list;
    // the visible list shouldn't be in use right now, but will pick up on changes in draw_order.
    sprite_detach_from_draw(index);

    // update the free list:
    LL_FREE(sprite, next_to_draw, previous_to_draw, index);
}

static inline void sprite_insert_before_visible(uint8_t index, uint8_t next_visible)
{   // inserts sprite[index] into visible list right before sprite[next_visible]
    // and after sprite[next_visible].previous_visible

    uint8_t previous_visible = sprite[next_visible].previous_visible;
    ASSERT(sprite[previous_visible].next_visible == next_visible);

    sprite[previous_visible].next_visible = index;
    sprite[next_visible].previous_visible = index;

    sprite[index].next_visible = next_visible;
    sprite[index].previous_visible = previous_visible;
}

static inline void sprite_add_to_visible(uint8_t index)
{
    uint8_t next_visible = sprite[0].next_visible;
    while (next_visible)
    {   // find out where sprite[index] belongs
        if (sprite[next_visible].iz >= sprite[index].iz)
            break;
    }
    // if next_visible == 0, then
    //   nothing else in the list now, or the last element which should be visible:
    // otherwise, sprite[next_visible].iz >= sprite[index].iz,
    //   which means the next_visible sprite has higher drawing priority.
    sprite_insert_before_visible(index, next_visible);
}

void sprite_line()
{   // add any new sprites to the draw line, and remove any that are no longer visible.
    // then draw sprites based on landscape iz-order.

    // check to remove sprites first, since that will make the list smaller when adding to it
    uint8_t visible = sprite[0].next_visible;
    const int16_t vga16 = vga_line;
    while (visible)
    {   int bottom16 = sprite[visible].iy + sprite[visible].height;
        if (vga16 >= bottom16)
        {   // sprite[visible] is actually not visible
            uint8_t next_visible = sprite[visible].next_visible;
            uint8_t previous_visible = sprite[visible].previous_visible;

            ASSERT(sprite[previous_visible].next_visible == visible);
            sprite[previous_visible].next_visible = next_visible;
            ASSERT(sprite[next_visible].previous_visible == visible);
            sprite[next_visible].previous_visible = previous_visible;

            visible = next_visible;
        }
        else 
            visible = sprite[visible].next_visible;
    }

    // add sprites:
    uint8_t check_next_for_visible = sprite[0].check_next_for_visible;
    while (!check_next_for_visible)
    {   const struct sprite *maybe_next = &sprite[check_next_for_visible];
        if (maybe_next->iy > vga16)
        {   // maybe_next may be on-screen, but it's definitely below the current vga_line
            break;
        }
        // maybe_next is on-screen or above the screen.
        // only draw things on-screen and that have a non-zero height:
        if (maybe_next->iy + maybe_next->height > 0 && maybe_next->height)
            sprite_add_to_visible(check_next_for_visible);
        // update the sprite we should check next:
        check_next_for_visible = sprite[check_next_for_visible].next_to_draw;
    }
    sprite[0].check_next_for_visible = check_next_for_visible;

    // actually draw the sprites that are visible on this vga_line:
    uint16_t colors[2];
    LL_ITERATE(sprite, next_visible, index, 0,
        // TODO: generate a landscape z-order before this frame, maybe for every "tile" (e.g. every 16 pixels)
        sprite_t *visible_sprite = &sprite[index];
        colors[0] = sprite_palette[sprite->colors & 15];
        colors[1] = sprite_palette[sprite->colors >> 4];
        uint8_t sprite_y = vga16 - sprite->iy;
        ASSERT(sprite_y >= 0 && sprite_y < sprite->height);
        uint16_t *dst = draw_buffer + sprite->ix;
        uint16_t *max_dst = dst + sprite->width;
        if (dst < draw_buffer)
            dst = draw_buffer;
        if (max_dst > draw_buffer + SCREEN_W)
            max_dst = draw_buffer + SCREEN_W;
        ASSERT(dst < max_dst);
        switch (visible_sprite->shape)
        {   case Rectangle_TopHalfBottomHalf:
                if (sprite_y < sprite->height / 2)
                while (dst < max_dst)
                    *dst++ = colors[0];
                else
                while (dst < max_dst)
                    *dst++ = colors[1];
                break;
            default:
                // Ignore
                break;
        }
    );
}

static inline float sprite_compare(uint8_t index1, uint8_t index2)
{   // return <0 if sprite[index1] should go before sprite[index2] in the draw order,
    // return >0 if sprite[index1] should go after sprite[index2] in the draw order,
    // return  0 if they are equal in draw order.
    const struct sprite *sprite1 = &sprite[index1];
    const struct sprite *sprite2 = &sprite[index2];
    int16_t compare_iy = sprite1->iy - sprite2->iy;
    if (compare_iy == 0)
    {   // sprite1->iy == sprite2->iy
        // TODO: see if it's worth this extra cost to make sure we are z-ordered.
        // adding to visible list will do a sorted insert anyway based on iz.
        return sprite1->iz - sprite2->iz;
    }
    else if (compare_iy < 0)
    {   // sprite1->iy < sprite2->iy
        return -1.0f;
    }
    else
    {   // compare_iy > 0: sprite1->iy > sprite2->iy
        return 1.0f;
    }
}

static inline void sprite_push_back_to_sort(uint8_t sort_this)
{   // We can assume everything in the list from sprite[0].next_to_draw
    // to sprite[sort_this].previous_to_draw is sorted.  From here,
    // push down sprite[sort_this] to the right position in the list.
    uint8_t check_sprite = sprite[sort_this].previous_to_draw;
    if (!check_sprite ||
        sprite_compare(sort_this, check_sprite) >= 0.f
    ){  // nothing to do here, list has only one element or is already sorted
        return;
    }
    // can update at least once since sprite[sort_this] < sprite[check_sprite]
    // wherever we end up, we will need to detach it from where it currently is:
    sprite_detach_from_draw(sort_this);
    while (1)
    {   // keep diving down until we find where sprite[sort_this] should be;
        // since it's a linked list we don't need to submerge sprite[sort_this]
        // at every step, we just need to find the final position
        // and update the links.
        check_sprite = sprite[check_sprite].previous_to_draw;
        if (!check_sprite || sprite_compare(sort_this, check_sprite) >= 0.f)
        {   // if check_sprite == 0, sprite[sort_this] goes at very front of list
            // else: sprite[sort_this] == sprite[check_sprite] or sprite[sort_this] > sprite[check_sprite].
            // in either case, put sprite[sort_this] just after sprite[check_sprite]
            sprite_attach_to_draw(sort_this, check_sprite);
            return;
        }
        // if we get here, sprite[sort_this] < sprite[check_sprite]
        // keep going, sort_this might be smaller than earlier sprites.
        // check_sprite is updated at the start of the loop, so don't do it here.
    }
}

void sprite_frame()
{   // Puts all sprites into order based on when they will first appear on-screen in the y direction.

    // TODO: use qsort if we're having difficulties keeping CPU within bitbox requirements
    // insertion sort all the drawing sprites on the map
    uint8_t sorted_until = sprite[0].next_to_draw;
    if (!sorted_until)
        goto finish_sprite_frame;

    // at least one thing in the list, sorted_until.
    // invariant: everything is sorted until sorted_until.
    // when we look at the next element in the list, submerge it back
    // in the linked list until it goes where it is sorted.
    while (1)
    {   uint8_t sort_this = sorted_until;
        // break the invariant to get the new tail of the list:
        sorted_until = sprite[sort_this].next_to_draw;

        // restore the invariant.
        // this may update sprite[sort_this].next_to_draw which is why we grabbed it earlier.
        sprite_push_back_to_sort(sort_this);

        if (!sorted_until)
        {   // no more sprites to be sorted
            break;
        }
    }

finish_sprite_frame:
    sprite[0].check_next_for_visible = sprite[0].next_to_draw;
    sprite[0].next_visible = 0;
    sprite[0].previous_visible = 0;
}

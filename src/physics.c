#include "game.h"
#include "physics.h"

physics_object_t physics_object[MAX_PHYSICS_OBJECTS] CCM_MEMORY;
physics_collision_t physics_collision[MAX_PHYSICS_COLLISIONS] CCM_MEMORY;

void physics_reset()
{   // Resets physics engine
    LL_RESET(physics_object, next_object, previous_object, MAX_PHYSICS_OBJECTS);
}

uint8_t physics_new_object()
{   // returns an index to a free physics_object, or zero if none.
    LL_NEW(physics_object, next_object, previous_object, MAX_PHYSICS_OBJECTS);
}

void physics_free_object(uint8_t index)
{   // returns an index to a free physics_object, or zero if none.
    LL_FREE(physics_object, index);
}

static inline void physics_reset_collisions()
{   LL_RESET(physics_collision, next_collision, previous_collision, MAX_PHYSICS_COLLISIONS); 
    uint8_t object = physics_object[0].next_object;
    while (object)
    {   memset(physics_object[object].collisions, 0, 6);
        object = physics_object[object].next_object;
    }
}

void physics_frame()
{   physics_reset_collisions();

}

int physics_overlap(const physics_boundary_t *b1, const physics_boundary_t *b2)
{   // Returns true if the boundaries overlap.
    ASSERT(b1 != NULL && b2 != NULL);
    if
    (   b1->corner_max[0] < b2->corner_min[0] ||
        b2->corner_max[0] < b1->corner_min[0] ||
        b1->corner_max[1] < b2->corner_min[1] ||
        b2->corner_max[1] < b1->corner_min[1] ||
        b1->corner_max[2] < b2->corner_min[2] ||
        b2->corner_max[2] < b1->corner_min[2]
    )
    {   return 0;
    }
    return 1;
}

#ifdef EMULATOR
void test_physics()
{   // Physics tests to make sure everything is working correctly.
    {   // Test boundaries overlapping in x only
        physics_boundary_t boundary1 = {
            .corner_min = {0, -1, 1},
            .corner_max = {2, 0, 4},
        };
        physics_boundary_t boundary2 = {
            .corner_min = {1, -7, 5},
            .corner_max = {3, -5, 6},
        };
        ASSERT(physics_overlap(&boundary1, &boundary2) == 0);
    }
    {   // Test boundaries overlapping in y only
        physics_boundary_t boundary1 = {
            .corner_min = {0, -1, 7},
            .corner_max = {2, 0, 8},
        };
        physics_boundary_t boundary2 = {
            .corner_min = {3, -1.5, 5},
            .corner_max = {4, -0.5, 6},
        };
        ASSERT(physics_overlap(&boundary1, &boundary2) == 0);
    }
    {   // Test boundaries overlapping in z only
        physics_boundary_t boundary1 = {
            .corner_min = {0, -1, 7},
            .corner_max = {2, 0, 8},
        };
        physics_boundary_t boundary2 = {
            .corner_min = {3, 2, 7.5},
            .corner_max = {4, 3, 7.75},
        };
        ASSERT(physics_overlap(&boundary1, &boundary2) == 0);
    }
    {   // Test boundaries overlapping in x and y
        physics_boundary_t boundary1 = {
            .corner_min = {0, -1, 1},
            .corner_max = {2, 0, 4},
        };
        physics_boundary_t boundary2 = {
            .corner_min = {1, -0.5, 5},
            .corner_max = {3, 1, 6},
        };
        ASSERT(physics_overlap(&boundary1, &boundary2) == 0);
    }
    {   // Test boundaries overlapping in y and z
        physics_boundary_t boundary1 = {
            .corner_min = {0, -1, 4},
            .corner_max = {2, 0, 5.5},
        };
        physics_boundary_t boundary2 = {
            .corner_min = {3, -1.5, 5},
            .corner_max = {4, -0.5, 6},
        };
        ASSERT(physics_overlap(&boundary1, &boundary2) == 0);
    }
    {   // Test boundaries overlapping in z and x
        physics_boundary_t boundary1 = {
            .corner_min = {0, -1, 7},
            .corner_max = {2, 0, 8},
        };
        physics_boundary_t boundary2 = {
            .corner_min = {1, 2, 7.5},
            .corner_max = {4, 3, 7.75},
        };
        ASSERT(physics_overlap(&boundary1, &boundary2) == 0);
    }
    {   // Test boundaries overlapping
        physics_boundary_t boundary1 = {
            .corner_min = {0, -1, 7},
            .corner_max = {2, 0, 8},
        };
        physics_boundary_t boundary2 = {
            .corner_min = {1, -2, 7.5},
            .corner_max = {1.5, 0, 9},
        };
        ASSERT(physics_overlap(&boundary1, &boundary2) == 1);
    }
    {   // Test equal boundaries overlapping
        physics_boundary_t boundary1 = {
            .corner_min = {0, -1, 1},
            .corner_max = {2, 0, 4},
        };
        physics_boundary_t boundary2 = {
            .corner_min = {0, -1, 1},
            .corner_max = {2, 0, 4},
        };
        ASSERT(physics_overlap(&boundary1, &boundary2) == 1);
    }
    message("physics tests passed!\n");
}
#endif
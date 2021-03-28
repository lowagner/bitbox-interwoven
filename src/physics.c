#include "game.h"
#include "physics.h"

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
{   
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

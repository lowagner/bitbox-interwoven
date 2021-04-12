#include "game.h"
#include "physics.h"

physics_object_t physics_object[MAX_PHYSICS_OBJECTS] CCM_MEMORY;
physics_collision_t physics_collision[MAX_PHYSICS_COLLISIONS] CCM_MEMORY;
struct physics_static physics_static CCM_MEMORY;

static inline void min_vectors(float *min, const float *va, const float *vb)
{   min[0] = va[0] < vb[0] ? va[0] : vb[0];
    min[1] = va[1] < vb[1] ? va[1] : vb[1];
    min[2] = va[2] < vb[2] ? va[2] : vb[2];
}

static inline void max_vectors(float *max, const float *va, const float *vb)
{   max[0] = va[0] > vb[0] ? va[0] : vb[0];
    max[1] = va[1] > vb[1] ? va[1] : vb[1];
    max[2] = va[2] > vb[2] ? va[2] : vb[2];
}

static inline void boundary_union(physics_boundary_t *dst, const physics_boundary_t *b1, const physics_boundary_t *b2)
{   min_vectors(dst->corner_min, b1->corner_min, b2->corner_min);
    max_vectors(dst->corner_max, b1->corner_max, b2->corner_max);
}

static inline void average_vectors(float *avg, const float *va, const float *vb)
{   avg[0] = 0.5f * (va[0] + vb[0]);
    avg[1] = 0.5f * (va[1] + vb[1]);
    avg[2] = 0.5f * (va[2] + vb[2]);
}

static inline void diff_vectors(float *diff, const float *va, const float *vb)
{   diff[0] = va[0] - vb[0];
    diff[1] = va[1] - vb[1];
    diff[2] = va[2] - vb[2];
}

void physics_static_move(int index, physics_boundary_t new_boundary)
{   // Move a static object around.  Computes the velocity and new size from the new_boundary as well.
    ASSERT(index >= 0 && index < physics_static.count);
    physics_entity_t *entity = &physics_static.entity[index];
    float new_center[3];
    float old_center[3];
    average_vectors(new_center, new_boundary.corner_min, new_boundary.corner_max);
    average_vectors(old_center, entity->boundary.corner_min, entity->boundary.corner_max);
    diff_vectors(entity->size, new_boundary.corner_max, new_boundary.corner_min);
    diff_vectors(entity->velocity, new_center, old_center);
    boundary_union(&entity->frame_movement, &new_boundary, &entity->boundary);
    memcpy(&entity->boundary, &new_boundary, sizeof(physics_boundary_t));
}

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
    {   physics_object[object].all_collisions = 0;
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
    // probably need at least one collision per object, e.g., for standing on a surface
    ASSERT(MAX_PHYSICS_COLLISIONS >= MAX_PHYSICS_OBJECTS * 2);
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
    {   // Test union boundaries
        physics_boundary_t boundary1 = {
            .corner_min = {-5, -1, 1},
            .corner_max = {-2, 10, 4},
        };
        physics_boundary_t boundary2 = {
            .corner_min = {-6, 0, 1},
            .corner_max = {-1, 3, 5},
        };
        physics_boundary_t expected = {
            .corner_min = {-6, -1, 1},
            .corner_max = {-1, 10, 5},
        };
        physics_boundary_t value;
        boundary_union(&value, &boundary1, &boundary2);
        ASSERT(memcmp(&value, &expected, sizeof(physics_boundary_t)) == 0);
    }
    message("physics tests passed!\n");
}
#endif

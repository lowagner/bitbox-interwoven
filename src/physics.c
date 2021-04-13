#include "game.h"
#include "physics.h"

#include <math.h>

#define PHYSICS_GRAVITY 1.0f

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

static inline void vector_average(float *avg, const float *va, const float *vb)
{   avg[0] = 0.5f * (va[0] + vb[0]);
    avg[1] = 0.5f * (va[1] + vb[1]);
    avg[2] = 0.5f * (va[2] + vb[2]);
}

static inline void vector_add(float *avg, const float *va, const float *vb)
{   avg[0] = va[0] + vb[0];
    avg[1] = va[1] + vb[1];
    avg[2] = va[2] + vb[2];
}

static inline void vector_diff(float *diff, const float *va, const float *vb)
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
    vector_average(new_center, new_boundary.corner_min, new_boundary.corner_max);
    vector_average(old_center, entity->boundary.corner_min, entity->boundary.corner_max);
    vector_diff(entity->size, new_boundary.corner_max, new_boundary.corner_min);
    vector_diff(entity->velocity, new_center, old_center);
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
    LL_FREE(physics_object, next_object, previous_object, index);
}

uint8_t physics_new_collision()
{   LL_NEW(physics_collision, next_collision, previous_collision, MAX_PHYSICS_COLLISIONS);
}

void physics_free_collision(uint8_t index)
{   LL_FREE(physics_collision, next_collision, previous_collision, index);
}

static inline void physics_reset_collisions()
{   LL_RESET(physics_collision, next_collision, previous_collision, MAX_PHYSICS_COLLISIONS); 
    uint8_t object = physics_object[0].next_object;
    while (object)
    {   physics_object[object].all_collisions = 0;
        object = physics_object[object].next_object;
    }
}

static inline void physics_update_object(uint8_t index)
{   ASSERT(index > 0);
    ASSERT(index < MAX_PHYSICS_OBJECTS);
    physics_entity_t *entity = &physics_object[index].entity;
    physics_boundary_t new_boundary;
    // TODO: add delta_t for slow down sections, both to velocity-delta and position-delta:
    entity->velocity[2] += PHYSICS_GRAVITY; // velocity-delta
    // TODO: if incorporating size-changes, measure velocity from center of previous boundary
    vector_add(new_boundary.corner_min, entity->boundary.corner_min, entity->velocity); // position-delta
    vector_add(new_boundary.corner_max, new_boundary.corner_min, entity->size);
    boundary_union(&entity->frame_movement, &new_boundary, &entity->boundary);
    // Don't update entity->boundary yet, since it might collide with things.
}

static inline uint8_t physics_add_collision(uint8_t index, uint8_t other)
{   // adds a collision to physics_object[index] with `other`
    // doesn't add any metadata yet besides who collided.
    ASSERT(index > 0);
    ASSERT(index < MAX_PHYSICS_OBJECTS);
    ASSERT(other > 0);
    ASSERT(other < MAX_PHYSICS_OBJECTS + physics_static.count);
    uint8_t *collision_save_location = physics_object[index].collision;
    if (*collision_save_location)
    {   if (*++collision_save_location)
        {   if (*++collision_save_location)
            {   if (*++collision_save_location)
                {   message("dropped a collision %d <-> %d, more than 4 on first.\n", (int)index, (int)other);
                    return 0;
                }
            }
        }
    }
    uint8_t collision = physics_new_collision();
    if (!collision)
    {   message("dropped a collision %d <-> %d, ran out of free collisions\n", (int)index, (int)other);
        return 0;
    }
    *collision_save_location = collision;
    physics_collision[collision].index1 = index;
    physics_collision[collision].index2 = MAX_PHYSICS_OBJECTS + other;
    return collision;
}

static inline float boundary_subtract_index
(   physics_boundary_t *boundary, const physics_boundary_t *remove, int index
)
{   // Returns impulse required to keep `boundary` out of `remove`, where index == dimension index.
    // cases: just for X (index = 0), but similarly for Y (index = 1) and Z (index = 2):
    // X.I: boundary->corner_min.x < remove->corner_min.x
    // * X.I.a: boundary->corner_max.x < remove->corner_max.x
    //        xxxxxxx       xxxxxxx
    //      bbccxxxxx ==> bbxxxxxxx     impulse < 0
    //      bbbb          bb
    // * X.I.b: boundary->corner_max.x > remove->corner_max.x
    //        xxxxxxx
    //      bbcccccccbb ==> plus or minus infinity
    //      bbbbbbbbbbb
    // X.II: boundary->corner_min.x > remove->corner_min.x
    // * X.II.a: boundary->corner_max.x < remove->corner_max.x
    //        xxxxxxx
    //        xxcccxx ==> plus or minus infinity
    //          bbb
    // * X.II.b: boundary->corner_max.x > remove->corner_max.x
    //        xxxxxxx       xxxxxxx
    //        xxxxxccbb ==> xxxxxxxbb   impulse > 0
    //             bbbb            bb
    
    // TODO: add support for a delta_t
    if (boundary->corner_min[index] < remove->corner_min[index])
    {   // I
        if (boundary->corner_max[index] < remove->corner_max[index])
            // I.a
            return remove->corner_min[index] - boundary->corner_max[index];
        else
            // I.b
            return -INFINITY;
    }
    else
    {   // II
        if (boundary->corner_max[index] < remove->corner_max[index])
            // II.a
            return INFINITY;
        else
            // II.b
            return remove->corner_max[index] - boundary->corner_min[index];
    }
}

static inline void boundary_subtract(physics_boundary_t *boundary, const physics_boundary_t *remove, float *impulse)
{   // We want to find the smallest (in magnitude) impulse that gets us out of trouble.
    // Since changing any dimension sufficiently can undo a collision, we figure out the
    // impulse required to escape in any dimension, and then zero out all but the smallest.
    impulse[0] = boundary_subtract_index(boundary, remove, 0);
    impulse[1] = boundary_subtract_index(boundary, remove, 1);
    impulse[2] = boundary_subtract_index(boundary, remove, 2);
    float i_absolute[3] = {
        fabs(impulse[0]),
        fabs(impulse[1]),
        fabs(impulse[2]),
    };
    // Find smallest absolute value:
    if (i_absolute[0] < i_absolute[1])
    {   // x < y
        if (i_absolute[2] < i_absolute[0])
            // z is the smallest
            impulse[0] = impulse[1] = 0.f;
        else
            // x is the smallest
            impulse[1] = impulse[2] = 0.f;
    }
    else
    {   // x >= y
        if (i_absolute[1] < i_absolute[2])
            // y is the smallest
            impulse[0] = impulse[2] = 0.f;
        else
            // z is the smallest
            impulse[0] = impulse[1] = 0.f;
    }
    // TODO: actually remove boundary
    // we might actually want to adjust the velocity now for static objects
    // TODO: give crush damage when two static things are pushing together on you
}

static inline void physics_add_static_collision(uint8_t index, uint8_t other)
{   // adds a collision to physics_object[index] with `other`, which is indexed in static list.
    ASSERT(index > 0);
    ASSERT(index < MAX_PHYSICS_OBJECTS);
    ASSERT(other > 0);
    ASSERT(other < physics_static.count);
    uint8_t collision = physics_add_collision(index, other + MAX_PHYSICS_OBJECTS);
    if (!collision) return;
    // See what sort of impulse it would require to remove the object from the static:
    boundary_subtract
    (   &physics_object[index].entity.frame_movement,
        &physics_static.entity[other].frame_movement,
        physics_collision[collision].impulse
    );
}

void physics_frame()
{   physics_reset_collisions();
    ASSERT(physics_static.count < MAX_PHYSICS_STATICS);
    LL_ITERATE
    (   physics_object, next_object, index,
        // update object and see if it ran into any static things.

        physics_update_object(index);
        for (uint8_t ps = 0; ps < physics_static.count; ++ps)
        if
        (   physics_overlap
            (   &physics_object[index].entity.frame_movement,
                &physics_static.entity[ps].frame_movement
            )
        )
            physics_add_static_collision(index, ps); 
    );
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

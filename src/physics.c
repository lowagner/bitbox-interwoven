#include "game.h"
#include "physics.h"

#include <assert.h>
#include <math.h>

// TODO: maybe set to 2.0f.
#define PHYSICS_GRAVITY 1.0f

physics_object_t physics_object[MAX_PHYSICS_OBJECTS] CCM_MEMORY;
physics_collision_t physics_collision[MAX_PHYSICS_COLLISIONS] CCM_MEMORY;
struct physics_static physics_static CCM_MEMORY;

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

void physics_static_move_preframe(int index, physics_boundary_t new_boundary)
{   // Move a static object around.
    // Call before calling physics_frame(), but then make sure to call physics_static_move_postframe() afterwards.
    // Computes the velocity and new size from the new_boundary as well.
    ASSERT(index >= 0 && index < physics_static.count);
    physics_entity_t *entity = &physics_static.entity[index];

    // Does not currently support changing the size.
    ASSERT(CLOSE(entity->size[0], new_boundary.corner_max[0] - new_boundary.corner_min[0]));
    ASSERT(CLOSE(entity->size[1], new_boundary.corner_max[1] - new_boundary.corner_min[1]));
    ASSERT(CLOSE(entity->size[2], new_boundary.corner_max[2] - new_boundary.corner_min[2]));
    // If we wanted to support changing size, we'd want to average corner_min/max here:
    vector_diff(entity->velocity, new_boundary.corner_max, entity->boundary.corner_max);

    if (entity->velocity[0] > 0.f)
    {   entity->snap[0] = SnapToMax;
        entity->boundary.corner_max[0] = new_boundary.corner_max[0];
    }
    else
    {   entity->snap[0] = SnapToMin;
        entity->boundary.corner_min[0] = new_boundary.corner_min[0];
    }
    if (entity->velocity[1] > 0.f)
    {   entity->snap[1] = SnapToMax;
        entity->boundary.corner_max[1] = new_boundary.corner_max[1];
    }
    else
    {   entity->snap[1] = SnapToMin;
        entity->boundary.corner_min[1] = new_boundary.corner_min[1];
    }
    if (entity->velocity[2] > 0.f)
    {   entity->snap[2] = SnapToMax;
        entity->boundary.corner_max[2] = new_boundary.corner_max[2];
    }
    else
    {   entity->snap[2] = SnapToMin;
        entity->boundary.corner_min[2] = new_boundary.corner_min[2];
    }
}

static inline void physics_entity_move_postframe(physics_entity_t *entity)
{   // moves an entity based on snap
    ASSERT(entity != NULL);
    if (entity->snap[0] == SnapToMax)
        entity->boundary.corner_min[0] = entity->boundary.corner_max[0] - entity->size[0];
    else
        entity->boundary.corner_max[0] = entity->boundary.corner_min[0] + entity->size[0];
    if (entity->snap[1] == SnapToMax)
        entity->boundary.corner_min[1] = entity->boundary.corner_max[1] - entity->size[1];
    else
        entity->boundary.corner_max[1] = entity->boundary.corner_min[1] + entity->size[1];
    if (entity->snap[2] == SnapToMax)
        entity->boundary.corner_min[2] = entity->boundary.corner_max[2] - entity->size[2];
    else
        entity->boundary.corner_max[2] = entity->boundary.corner_min[2] + entity->size[2];
}

void physics_static_move_postframe(int index)
{   // Finalizes moving a static object around.  Call after calling physics_frame(), but only if you've
    // called physics_static_move_preframe() before that.
    ASSERT(index >= 0 && index < physics_static.count);
    physics_entity_move_postframe(&physics_static.entity[index]);
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

static inline void physics_stretch_object(uint8_t index)
{   ASSERT(index > 0);
    ASSERT(index < MAX_PHYSICS_OBJECTS);
    physics_entity_t *entity = &physics_object[index].entity;
    // TODO: add delta_t for slow down sections, both to velocity-delta and position-delta:
    entity->velocity[2] += PHYSICS_GRAVITY; // velocity-delta

    if (entity->velocity[0] > 0.f)
    {   entity->snap[0] = SnapToMax;
        entity->boundary.corner_max[0] += entity->velocity[0];
    }
    else
    {   entity->snap[0] = SnapToMin;
        entity->boundary.corner_min[0] += entity->velocity[0];
    }
    if (entity->velocity[1] > 0.f)
    {   entity->snap[1] = SnapToMax;
        entity->boundary.corner_max[1] += entity->velocity[1];
    }
    else
    {   entity->snap[1] = SnapToMin;
        entity->boundary.corner_min[1] += entity->velocity[1];
    }
    if (entity->velocity[2] > 0.f)
    {   entity->snap[2] = SnapToMax;
        entity->boundary.corner_max[2] += entity->velocity[2];
    }
    else
    {   entity->snap[2] = SnapToMin;
        entity->boundary.corner_min[2] += entity->velocity[2];
    }
}

static inline uint8_t physics_add_generic_collision(uint8_t index, uint8_t other)
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

static inline float boundary_get_impulse_index
(   int index, physics_boundary_t *boundary, const physics_boundary_t *remove
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

static inline void collision_get_impulse
(   physics_collision_t *collision, physics_boundary_t *boundary, const physics_boundary_t *remove
)
{   // We want to find the smallest (in magnitude) impulse that gets us out of trouble.
    // Since changing any dimension sufficiently can undo a collision, we figure out the
    // impulse required to escape in any dimension, and then zero out all but the smallest.
    float impulse[3] = {
        boundary_get_impulse_index(0, boundary, remove),
        boundary_get_impulse_index(1, boundary, remove),
        boundary_get_impulse_index(2, boundary, remove),
    };
    float i_absolute[3] = {
        fabs(impulse[0]),
        fabs(impulse[1]),
        fabs(impulse[2]),
    };
    // Find smallest absolute value, breaking ties towards x and z (highest priority):
    if (i_absolute[1] < i_absolute[0])
    {   // y < x
        if (i_absolute[1] < i_absolute[2])
        {   // y is the smallest (y < z)
            collision->impulse.magnitude = i_absolute[1];
            collision->impulse.direction = impulse[1] > 0.f ? DirectionPlusY : DirectionMinusY;
        }
        else
        {   // z is the smallest (z <= y)
            collision->impulse.magnitude = i_absolute[2];
            collision->impulse.direction = impulse[2] > 0.f ? DirectionPlusZ : DirectionMinusZ;
        }
    }
    else
    {   // x <= y
        if (i_absolute[0] < i_absolute[2])
        {   // x is the smallest (x < z)
            collision->impulse.magnitude = i_absolute[0];
            collision->impulse.direction = impulse[0] > 0.f ? DirectionPlusX : DirectionMinusX;
        }
        else
        {   // z is the smallest (z <= x)
            collision->impulse.magnitude = i_absolute[2];
            collision->impulse.direction = impulse[2] > 0.f ? DirectionPlusZ : DirectionMinusZ;
        }
    }
}

static inline void physics_add_static_collision(uint8_t index, uint8_t other)
{   // adds a collision to physics_object[index] with `other`, which is indexed in static list.
    ASSERT(index > 0);
    ASSERT(index < MAX_PHYSICS_OBJECTS);
    ASSERT(other > 0);
    ASSERT(other < physics_static.count);
    uint8_t collision_index = physics_add_generic_collision(index, other + MAX_PHYSICS_OBJECTS);
    if (!collision_index) return;
    // See what sort of impulse it would require to remove the object from the static:
    physics_collision_t *collision = &physics_collision[collision_index];
    physics_boundary_t *dynamic_boundary = &physics_object[index].entity.boundary;
    collision_get_impulse
    (   collision,
        dynamic_boundary,
        &physics_static.entity[other].boundary
    );
    // we adjust the velocity later based on impulses from all static objects.
    // Don't add them now since we wouldn't know how much impulse the object has
    // already felt, and some impulses might be unnecessary if another
    // static object has already provided it.

    // Decrease the size of the dynamic_boundary since it shouldn't be
    // allowed to go through the static entity:
    if (collision->impulse.direction % 2)
    {   // impulse is in a negative direction
        STATIC_ASSERT(DirectionMinusX % 2 == 1);
        STATIC_ASSERT(DirectionMinusY % 2 == 1);
        STATIC_ASSERT(DirectionMinusZ % 2 == 1);
        STATIC_ASSERT(DirectionMinusX / 2 == 0);
        STATIC_ASSERT(DirectionMinusY / 2 == 1);
        STATIC_ASSERT(DirectionMinusZ / 2 == 2);
        // negative impulse means we need to reduce max boundary
        // (reducing the min boundary would make the dynamic_boundary bigger):
        dynamic_boundary->corner_max[collision->impulse.direction / 2]
                -= collision->impulse.magnitude;
    }
    else
    {   // impulse is in a positive direction
        STATIC_ASSERT(DirectionPlusX % 2 == 0);
        STATIC_ASSERT(DirectionPlusY % 2 == 0);
        STATIC_ASSERT(DirectionPlusZ % 2 == 0);
        STATIC_ASSERT(DirectionPlusX / 2 == 0);
        STATIC_ASSERT(DirectionPlusY / 2 == 1);
        STATIC_ASSERT(DirectionPlusZ / 2 == 2);
        // positive impulse means we need to increase min boundary
        // (increasing the max boundary would make the dynamic_boundary bigger):
        dynamic_boundary->corner_min[collision->impulse.direction / 2]
                += collision->impulse.magnitude;
    }
}

static inline void physics_add_dynamic_collision(uint8_t index, uint8_t other)
{   // adds a collision to physics_object[index] with physics_object[other]
    ASSERT(index > 0);
    ASSERT(index < MAX_PHYSICS_OBJECTS);
    ASSERT(other > 0);
    ASSERT(other < MAX_PHYSICS_OBJECTS);
    uint8_t collision = physics_add_generic_collision(index, other);
    if (!collision) return;
    collision_get_impulse
    (   &physics_collision[collision],
        &physics_object[index].entity.boundary,
        &physics_static.entity[other].boundary
    );
}

void physics_frame()
{   physics_reset_collisions();
    ASSERT(physics_static.count < MAX_PHYSICS_STATICS);
    LL_ITERATE
    (   physics_object, next_object, index, 0,
        // update object based on velocity/acceleration and see if it ran into any static things.

        physics_stretch_object(index);

        // interact with floor below you, if present:
        uint8_t static_floor = physics_object[index].entity.on_top_of;
        if (static_floor)
        {   ASSERT(static_floor >= MAX_PHYSICS_OBJECTS);
            static_floor -= MAX_PHYSICS_OBJECTS;
            // this is important to get right for walking over two blocks that have the same height.
            // at the edge, you can fall down into the crack and get an impulse from the side of the next block
            // if the next block is ordered before the one you're standing on top of.  here we ensure
            // that the block the user is above triggers first (to clip out the lower part of the user's motion)
            if
            (   physics_overlap
                (   &physics_object[index].entity.boundary,
                    &physics_static.entity[static_floor].boundary
                )
            )
                physics_add_static_collision(index, static_floor);

            for (uint8_t ps = 0; ps < physics_static.count; ++ps)
            if
            (   ps != static_floor /* super important, don't hit static floor again */
                && physics_overlap
                (   &physics_object[index].entity.boundary,
                    &physics_static.entity[ps].boundary
                )
            )
                physics_add_static_collision(index, ps);
        }
        else
        for (uint8_t ps = 0; ps < physics_static.count; ++ps)
        if
        (   physics_overlap
            (   &physics_object[index].entity.boundary,
                &physics_static.entity[ps].boundary
            )
        )
            physics_add_static_collision(index, ps);
    );

    LL_ITERATE
    (   physics_object, next_object, index, 0,
        // update object based on interactions with any non-static things.

        LL_ITERATE
        (   physics_object, next_object, other, index /* collide only with objects with larger indices */,
            if
            (   physics_overlap
                (   &physics_object[index].entity.boundary,
                    &physics_object[other].entity.boundary
                )
            )
                physics_add_dynamic_collision(index, other);
        );

        // Adjust velocity based on impulses from all static objects:
        if (physics_object[index].all_collisions)
        {   // there is some collision here
            float impulse[DirectionCount] = {0.f};
            for (int i = 0; i < 4; ++i)
            if (physics_object[index].collision[i])
            {   physics_collision_t *collision = &physics_collision[physics_object[index].collision[i]];
                uint8_t other = collision->index2;
                // ignore dynamic objects for now, let users adjust velocity based on hits:
                if (other < MAX_PHYSICS_OBJECTS)
                    continue;
                // this was a static object
                ASSERT(collision->impulse.direction < DirectionCount);
                if (impulse[collision->impulse.direction] < collision->impulse.magnitude)
                    impulse[collision->impulse.direction] = collision->impulse.magnitude;
                /* TODO: add friction based on impulse[collision->impulse.direction] (i.e., normal force)
                other -= MAX_PHYSICS_OBJECTS;
                */
            }
            // TODO: give crush damage when two static things are pushing together on you
            // Don't allow crush damage for now:
            ASSERT(impulse[DirectionPlusX] == 0.f || impulse[DirectionMinusX] == 0.f);
            ASSERT(impulse[DirectionPlusY] == 0.f || impulse[DirectionMinusY] == 0.f);
            ASSERT(impulse[DirectionPlusZ] == 0.f || impulse[DirectionMinusZ] == 0.f);
            // TODO: allow for delta_t
            float *velocity = physics_object[index].entity.velocity;
            velocity[0] += impulse[DirectionPlusX] - impulse[DirectionMinusX];
            velocity[1] += impulse[DirectionPlusY] - impulse[DirectionMinusY];
            velocity[2] += impulse[DirectionPlusZ] - impulse[DirectionMinusZ];

            // TODO: check if snapping to max or min still allows you to fill up the correct size.
            // i.e., if a static object is moving into you and decreasing your boundary volume,
            // while you are moving away from the static object (but not as fast), your snap-to would
            // be on the side away from the static object, which means that snapping there would 
            // still clip you into the object.
        }
        physics_entity_move_postframe(&physics_object[index].entity);
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
    message("physics tests passed!\n");
}
#endif

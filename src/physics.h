#ifndef PHYSICS_H
#define PHYSICS_H

#include <stddef.h>

#define MAX_PHYSICS_STATICS 16
#define MAX_PHYSICS_OBJECTS 16
#define MAX_PHYSICS_COLLISIONS 32

typedef struct physics_boundary
{   // A rectangular prism representing a physics boundary which should not be crossed.
    float corner_min[3], corner_max[3];
} physics_boundary_t;

typedef enum
{   SnapToMax = 0,
    SnapToMin = 1,
} physics_snap_t;

typedef struct physics_entity
{   // If a static entity, it can be moved around programatically using the `physics_static_move` function,
    // or if inside a physics_object you can just update the velocity.
    // Current boundary of the entity.
    // During a frame step, this stretches (based on velocity) to encompass both start and end points.
    physics_boundary_t boundary;
    // Helper to snap back `boundary` to the correct size at the end of a frame.
    // * if snap[0] == SnapToMin, then we set boundary.corner_max[0] = boundary.corner_min[0] + size[0]
    //   if snap[0] == SnapToMax, then we set boundary.corner_min[0] = boundary.corner_max[0] - size[0]
    // * and similarly for y ([1]) and z ([2]).
    uint8_t snap[3];
    // For dynamic objects only, this is MAX_PHYSICS_OBJECTS + static_index
    // if this object is on top of a static object at physics_static.entity[static_index]:
    uint8_t on_top_of;
    // Size in each dimension x,y,z, e.g. width/length/height:
    float size[3];
    // TODO: add support for this:
    // Things that are on top of this entity will experience a force in the direction of velocity due to friction
    float velocity[3];
} physics_entity_t;

typedef struct physics_object
{   // Representation of an object that has dynamical physics applied.
    // Linked list representation:
    uint8_t next_object;
    uint8_t previous_object;
    union
    {   uint8_t next_free; // used in root physics_object only.
        // Indices to collisions in the last frame, or zero.
        uint8_t collision[4];
        uint32_t all_collisions;
    };
    physics_entity_t entity;
} physics_object_t;

extern physics_object_t physics_object[MAX_PHYSICS_OBJECTS];

struct physics_static
{   // Rectangular prisms that only interact with dynamic `physics_object`s but does not react to any forces.
    physics_entity_t entity[MAX_PHYSICS_STATICS];
    // Number of static entities:
    uint8_t count;
};

extern struct physics_static physics_static;

typedef struct physics_collision
{   // Description of a collision
    uint8_t next_collision, previous_collision;
    // These indices are 1 to (MAX_PHYSICS_OBJECTS-1) for a dynamic object, or
    // MAX_PHYSICS_OBJECTS to (MAX_PHYSICS_OBJECTS+MAX_PHYSICS_STATICS-1) for static terrain.
    // Note: index1 should always be from 1 to (MAX_PHYSICS_OBJECTS-1),
    // since only dynamic objects care about collisions.
    uint8_t index1;
    union {
        uint8_t index2;
        // only used in root collision:
        uint8_t next_free;
    };
    // impulse on object at index1; object at index2 would get the opposite value:
    float impulse[3];
} physics_collision_t;

extern physics_collision_t physics_collision[MAX_PHYSICS_COLLISIONS];

void physics_reset();
void physics_static_move_preframe(int index, physics_boundary_t new_boundary);
void physics_static_move_postframe(int index);
uint8_t physics_new_object();
void physics_free_object(uint8_t index);
void physics_frame();

int physics_overlap(const physics_boundary_t *b1, const physics_boundary_t *b2);

#ifdef EMULATOR
void test_physics();
#endif

#endif

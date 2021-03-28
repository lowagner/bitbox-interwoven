#ifndef PHYSICS_H
#define PHYSICS_H

#include <stddef.h>

#define MAX_PHYSICS_OBJECTS 16

typedef struct physics_boundary
{   // A rectangular prism representing a physics boundary which should not be crossed.
    float corner_min[3], corner_max[3];
} physics_boundary_t;

typedef struct physics_object
{   // Representation of an object that has dynamical physics applied.
    // Linked list representation:
    uint8_t next_object;
    uint8_t previous_object;
    // Indices to collisions in the last frame, or zero.
    union
    {   uint8_t next_free; // used in root physics_object only.
        uint8_t collisions[6];
    };
    // 
    physics_boundary_t boundary;
    // Size in each dimension x,y,z, e.g. width/length/height:
    float size[3];
    float velocity[3];
    // Min/max boundaries for the movement that we just did in the last frame:
    physics_boundary_t frame_movement;
} physics_object_t;

extern physics_object_t physics_object[MAX_PHYSICS_OBJECTS];

void physics_reset();
uint8_t physics_new_object();
void physics_free_object(uint8_t index);

int physics_overlap(const physics_boundary_t *b1, const physics_boundary_t *b2);

#ifdef EMULATOR
void test_physics();
#endif

#endif

#ifndef PHYSICS_H
#define PHYSICS_H

#include <stddef.h>

typedef struct physics_boundary
{   // A rectangular prism representing a physics boundary which should not be crossed.
    float corner_min[3], corner_max[3];
} physics_boundary_t;

struct physics_object
{   // Representation of an object that has dynamical physics applied.
    // Size in each dimension x,y,z, e.g. width/length/height:
    float size[3];
    // 
    physics_boundary_t boundary;
    float velocity[3];
    // Min/max boundaries for the movement that we just did in the last frame:
    physics_boundary_t frame_movement;

};

int physics_overlap(const physics_boundary_t *b1, const physics_boundary_t *b2);

#ifdef EMULATOR
void test_physics();
#endif

#endif

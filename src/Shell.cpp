#include "Shell.h"

Shell::Shell (Vec2 startPos, Vec2 vel, int owner)
    : position (startPos), previousPosition (startPos), velocity (vel), ownerIndex (owner)
{
}

void Shell::update (float dt)
{
    previousPosition = position;
    position += velocity * dt;
}

void Shell::reflect (Vec2 normal)
{
    // Reflect velocity off the normal
    // v' = v - 2(v.n)n
    float dot = velocity.dot (normal);
    velocity = velocity - normal * (2.0f * dot);
    bounceCount++;
}

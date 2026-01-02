#include "Shell.h"
#include <cmath>

Shell::Shell (Vec2 startPos, Vec2 vel, int owner, float range, float dmg)
    : position (startPos), previousPosition (startPos), startPosition (startPos),
      velocity (vel), ownerIndex (owner), maxRange (range), damage (dmg)
{
}

void Shell::update (float dt)
{
    previousPosition = position;
    position += velocity * dt;

    // Kill shell if it has traveled max range
    if (getDistanceTraveled() >= maxRange)
        kill();
}

float Shell::getDistanceTraveled() const
{
    Vec2 diff = position - startPosition;
    return std::sqrt (diff.x * diff.x + diff.y * diff.y);
}

void Shell::reflect (Vec2 normal)
{
    // Reflect velocity off the normal
    // v' = v - 2(v.n)n
    float dot = velocity.dot (normal);
    velocity = velocity - normal * (2.0f * dot);
    bounceCount++;

    // Push shell away from wall to prevent immediate re-collision
    position = position + normal * 5.0f;

    // Reset start position so shell gets fresh range after bouncing
    startPosition = position;
}

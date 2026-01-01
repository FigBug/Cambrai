#pragma once

#include "Config.h"
#include "Vec2.h"

class Shell
{
public:
    Shell (Vec2 startPos, Vec2 velocity, int ownerIndex);

    void update (float dt);

    Vec2 getPosition() const { return position; }
    Vec2 getVelocity() const { return velocity; }
    Vec2 getPreviousPosition() const { return previousPosition; }
    int getOwnerIndex() const { return ownerIndex; }
    float getRadius() const { return config.shellRadius; }
    float getDamageRadius() const { return config.shellDamageRadius; }
    int getBounceCount() const { return bounceCount; }
    bool isAlive() const { return alive; }
    void kill() { alive = false; }

    // Reflection off walls
    void reflect (Vec2 normal);
    bool canReflect() const { return bounceCount < config.maxShellBounces; }

private:
    Vec2 position;
    Vec2 previousPosition;
    Vec2 velocity;
    int ownerIndex;
    int bounceCount = 0;
    bool alive = true;
};

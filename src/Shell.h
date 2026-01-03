#pragma once

#include "Config.h"
#include "Vec2.h"

class Shell
{
public:
    Shell (Vec2 startPos, Vec2 velocity, int ownerIndex, float maxRange, float damage);

    void update (float dt);

    Vec2 getPosition() const { return position; }
    Vec2 getVelocity() const { return velocity; }
    Vec2 getPreviousPosition() const { return previousPosition; }
    Vec2 getStartPosition() const { return startPosition; }
    int getOwnerIndex() const { return ownerIndex; }
    float getRadius() const { return config.shellRadius; }
    float getDamageRadius() const { return config.shellDamageRadius; }
    float getDamage() const { return damage; }
    int getBounceCount() const { return bounceCount; }
    bool isAlive() const { return alive; }
    void kill() { alive = false; }
    float getMaxRange() const { return maxRange; }
    float getDistanceTraveled() const;

    // Reflection off walls
    void reflect (Vec2 normal);
    bool canReflect() const { return bounceCount < config.maxShellBounces; }

    // Apply external force (from fan/magnet)
    void applyForce (Vec2 force, float dt) { velocity += force * dt; }

private:
    Vec2 position;
    Vec2 previousPosition;
    Vec2 startPosition;
    Vec2 velocity;
    int ownerIndex;
    float maxRange;
    float damage;
    int bounceCount = 0;
    bool alive = true;
};

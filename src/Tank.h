#pragma once

#include "Config.h"
#include "Shell.h"
#include "Vec2.h"
#include <raylib.h>
#include <array>
#include <vector>

struct Smoke
{
    Vec2 position;
    float radius;
    float alpha;
    float fadeRate;
};

struct TrackMark
{
    Vec2 position;
    float angle;
    float alpha;
};

class Tank
{
public:
    Tank (int playerIndex, Vec2 startPos, float startAngle, float tankSize);

    void update (float dt, Vec2 moveInput, Vec2 aimInput, bool fireInput, float arenaWidth, float arenaHeight);

    Vec2 getPosition() const                        { return position; }
    float getAngle() const                          { return angle; }
    float getTurretAngle() const                    { return turretAngle; }
    float getSize() const                           { return size; }
    float getMaxSpeed() const                       { return config.tankMaxSpeed; }
    int getPlayerIndex() const                      { return playerIndex; }
    Vec2 getCrosshairPosition() const               { return position + crosshairOffset; }
    void setCrosshairPosition (Vec2 worldPos);
    const std::vector<Smoke>& getSmoke() const      { return smoke; }
    const std::vector<TrackMark>& getTrackMarks() const { return trackMarks; }
    float getDamagePercent() const                  { return 1.0f - (health / config.tankMaxHealth); }
    std::vector<Shell>& getPendingShells()          { return pendingShells; }
    Color getColor() const;

    // Health system
    float getHealth() const         { return health; }
    float getMaxHealth() const      { return config.tankMaxHealth; }
    bool isAlive() const            { return health > 0; }
    bool isVisible() const          { return isAlive() || isDestroying(); }
    bool isDestroying() const       { return destroying && destroyTimer < config.tankDestroyDuration; }
    bool isFullyDestroyed() const   { return destroying && destroyTimer >= config.tankDestroyDuration; }
    float getDestroyProgress() const { return destroying ? destroyTimer / config.tankDestroyDuration : 0.0f; }
    void takeDamage (float damage, int attackerIndex = -1);
    int getKillerIndex() const      { return killerIndex; }

    // Collision
    Vec2 getVelocity() const        { return velocity; }
    float getSpeed() const          { return velocity.length(); }
    void applyCollision (Vec2 pushDirection, float pushDistance, Vec2 impulse);
    std::array<Vec2, 4> getCorners() const;

    // HUD info
    float getThrottle() const       { return throttle; }
    float getReloadProgress() const { return reloadTimer / config.fireInterval; }
    bool isReadyToFire() const      { return reloadTimer >= config.fireInterval && isTurretOnTarget(); }
    bool isTurretOnTarget() const;

private:
    int playerIndex;
    int killerIndex = -1;           // Who killed this tank
    Vec2 position;
    Vec2 velocity;
    float angle = 0.0f;             // Tank body angle (radians)
    float turretAngle = 0.0f;       // Turret angle relative to body
    float size;

    float throttle = 0.0f;          // -1 to 1 (current throttle)
    float reloadTimer = 0.0f;       // Time since last shot

    Vec2 crosshairOffset;           // Offset from tank position

    std::vector<Smoke> smoke;
    float smokeSpawnTimer = 0.0f;

    std::vector<TrackMark> trackMarks;
    float trackMarkTimer = 0.0f;

    // Health
    float health = config.tankMaxHealth;

    // Destruction
    bool destroying = false;
    float destroyTimer = 0.0f;

    // Shooting
    std::vector<Shell> pendingShells;

    void clampToArena (float arenaWidth, float arenaHeight);
    void updateSmoke (float dt);
    void updateTrackMarks (float dt);
    void updateTurret (float dt);
    bool fireShell();
};

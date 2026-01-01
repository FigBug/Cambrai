#pragma once

#include "Config.h"
#include "Shell.h"
#include "Vec2.h"
#include <raylib.h>
#include <vector>

enum class ObstacleType
{
    SolidWall,       // Blocks shells and tanks, indestructible
    BreakableWall,   // Blocks shells and tanks, can be destroyed
    ReflectiveWall,  // Reflects shells, blocks tanks
    Mine,            // Explodes when tank touches, instant kill
    AutoTurret       // Shoots at nearest enemy tank
};

class Tank;

class Obstacle
{
public:
    Obstacle (ObstacleType type, Vec2 position, float angle, int ownerIndex);

    void update (float dt, const std::vector<Tank*>& tanks, float arenaWidth, float arenaHeight);

    ObstacleType getType() const { return type; }
    Vec2 getPosition() const { return position; }
    float getAngle() const { return angle; }
    int getOwnerIndex() const { return ownerIndex; }

    // For walls (oriented rectangles)
    float getLength() const { return config.wallLength; }
    float getThickness() const { return config.wallThickness; }
    std::array<Vec2, 4> getCorners() const;

    // For mines (circles)
    float getRadius() const { return config.mineRadius; }
    bool isArmed() const { return armTimer >= config.mineArmTime; }
    float getArmProgress() const { return std::min (1.0f, armTimer / config.mineArmTime); }

    // For auto turrets
    float getTurretAngle() const { return turretAngle; }
    float getReloadProgress() const { return reloadTimer / config.turretFireInterval; }
    std::vector<Shell>& getPendingShells() { return pendingShells; }

    // Health (for breakable walls)
    float getHealth() const { return health; }
    float getMaxHealth() const { return config.breakableWallHealth; }
    bool isAlive() const { return alive; }
    bool isDestroyed() const { return !alive; }
    void takeDamage (float damage);

    // For placement phase
    bool isValidPlacement (const std::vector<Obstacle>& obstacles, const std::vector<Tank*>& tanks, float arenaWidth, float arenaHeight) const;

    // Collision helpers
    bool checkShellCollision (const Shell& shell, Vec2& collisionPoint, Vec2& normal) const;
    bool checkTankCollision (const Tank& tank, Vec2& pushDirection, float& pushDistance) const;

private:
    ObstacleType type;
    Vec2 position;
    float angle;
    int ownerIndex;  // Which player placed this
    bool alive = true;

    // Breakable wall health
    float health;

    // Mine arming
    float armTimer = 0.0f;

    // Auto turret
    float turretAngle = 0.0f;
    float reloadTimer = 0.0f;
    std::vector<Shell> pendingShells;

    void updateAutoTurret (float dt, const std::vector<Tank*>& tanks);
    Tank* findNearestEnemy (const std::vector<Tank*>& tanks) const;

    // Line segment collision for walls
    bool lineSegmentIntersection (Vec2 p1, Vec2 p2, Vec2 p3, Vec2 p4, Vec2& intersection) const;
};

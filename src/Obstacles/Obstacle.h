#pragma once

#include "../Config.h"
#include "../Shell.h"
#include "../Vec2.h"
#include <raylib.h>
#include <array>
#include <cmath>
#include <memory>
#include <vector>

class Tank;
class Renderer;

enum class ObstacleType
{
    SolidWall,
    BreakableWall,
    ReflectiveWall,
    Mine,
    AutoTurret,
    Pit,
    Portal,
    Flag,
    HealthPack,
    Electromagnet,
    Fan
};

// Shell collision result
enum class ShellHitResult
{
    Miss,
    Destroyed,
    Reflected
};

class Obstacle
{
public:
    Obstacle (Vec2 position, float angle, int ownerIndex)
        : position (position), angle (angle), ownerIndex (ownerIndex) {}

    virtual ~Obstacle() = default;

    virtual void update (float dt, const std::vector<Tank*>& tanks, float arenaWidth, float arenaHeight) {}

    virtual ObstacleType getType() const = 0;

    // Force application - override in Electromagnet, Fan
    virtual Vec2 getTankForce (const Tank& tank) const { return { 0, 0 }; }
    virtual Vec2 getShellForce (Vec2 shellPos) const { return { 0, 0 }; }

    // Collection effects (flag capture, health pack pickup)
    struct CollectionEffect
    {
        int playerIndex = -1;
        int scoreToAdd = 0;
        float healthPercent = 0;  // 0.5 = heal 50%
    };
    virtual CollectionEffect consumeCollectionEffect() { return {}; }

    // Tank collision handling - called when checkTankCollision returns true
    // Return true to apply normal physics push, false to skip it
    virtual bool handleTankCollision (Tank& tank, const std::vector<std::unique_ptr<Obstacle>>& allObstacles) { return true; }

    // Whether this obstacle creates an explosion when hit by shell (AutoTurret)
    virtual bool createsExplosionOnHit() const { return false; }
    virtual float getCollisionRadius() const { return 20.0f; }
    virtual bool isRectangular() const { return false; }

    Vec2 getPosition() const { return position; }
    float getAngle() const { return angle; }
    int getOwnerIndex() const { return ownerIndex; }

    float getHealth() const { return health; }
    virtual float getMaxHealth() const { return 9999.0f; }
    bool isAlive() const { return alive; }
    virtual bool isArmed() const { return false; }  // For mines

    virtual void takeDamage (float damage)
    {
        health -= damage;
        if (health <= 0)
        {
            health = 0;
            alive = false;
        }
    }

    virtual ShellHitResult checkShellCollision (const Shell& shell, Vec2& collisionPoint, Vec2& normal) const = 0;
    virtual bool checkTankCollision (const Tank& tank, Vec2& pushDirection, float& pushDistance) = 0;
    virtual bool isValidPlacement (const std::vector<std::unique_ptr<Obstacle>>& obstacles, const std::vector<Tank*>& tanks, float arenaWidth, float arenaHeight) const = 0;

    virtual void draw (Renderer& renderer) const = 0;
    virtual void drawPreview (Renderer& renderer, bool valid) const = 0;

    std::vector<Shell>& getPendingShells() { return pendingShells; }

protected:
    Vec2 position;
    float angle;
    int ownerIndex;
    bool alive = true;
    float health = 9999.0f;
    std::vector<Shell> pendingShells;

    // Shared collision helpers
    bool checkCircleCollision (const Shell& shell, float radius, Vec2& collisionPoint, Vec2& normal) const
    {
        Vec2 diff = shell.getPosition() - position;
        float dist = diff.length();

        if (dist < radius + shell.getRadius())
        {
            collisionPoint = shell.getPosition();
            normal = diff.normalized();
            return true;
        }
        return false;
    }

    bool checkCircleTankCollision (const Tank& tank, float radius, Vec2& pushDirection, float& pushDistance) const;

    bool isValidCirclePlacement (float radius, const std::vector<std::unique_ptr<Obstacle>>& obstacles, const std::vector<Tank*>& tanks, float arenaWidth, float arenaHeight) const;

    static bool lineSegmentIntersection (Vec2 p1, Vec2 p2, Vec2 p3, Vec2 p4, Vec2& intersection)
    {
        float d1 = (p4.x - p3.x) * (p1.y - p3.y) - (p4.y - p3.y) * (p1.x - p3.x);
        float d2 = (p4.x - p3.x) * (p2.y - p3.y) - (p4.y - p3.y) * (p2.x - p3.x);
        float d3 = (p2.x - p1.x) * (p3.y - p1.y) - (p2.y - p1.y) * (p3.x - p1.x);
        float d4 = (p2.x - p1.x) * (p4.y - p1.y) - (p2.y - p1.y) * (p4.x - p1.x);

        if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) &&
            ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0)))
        {
            float t = d1 / (d1 - d2);
            intersection.x = p1.x + t * (p2.x - p1.x);
            intersection.y = p1.y + t * (p2.y - p1.y);
            return true;
        }
        return false;
    }
};

// Wall base class for rectangular obstacles
class Wall : public Obstacle
{
public:
    Wall (Vec2 position, float angle, int ownerIndex)
        : Obstacle (position, angle, ownerIndex) {}

    bool isRectangular() const override { return true; }

    float getLength() const { return config.wallLength; }
    float getThickness() const { return config.wallThickness; }

    std::array<Vec2, 4> getCorners() const
    {
        float halfLength = config.wallLength / 2.0f;
        float halfThickness = config.wallThickness / 2.0f;

        float cosA = std::cos (angle);
        float sinA = std::sin (angle);

        Vec2 localCorners[4] = {
            { -halfLength, -halfThickness },
            { halfLength, -halfThickness },
            { halfLength, halfThickness },
            { -halfLength, halfThickness }
        };

        std::array<Vec2, 4> worldCorners;
        for (int i = 0; i < 4; ++i)
        {
            worldCorners[i].x = position.x + localCorners[i].x * cosA - localCorners[i].y * sinA;
            worldCorners[i].y = position.y + localCorners[i].x * sinA + localCorners[i].y * cosA;
        }
        return worldCorners;
    }

    bool checkTankCollision (const Tank& tank, Vec2& pushDirection, float& pushDistance) override;

    bool isValidPlacement (const std::vector<std::unique_ptr<Obstacle>>& obstacles, const std::vector<Tank*>& tanks, float arenaWidth, float arenaHeight) const override
    {
        float margin = 20.0f;
        auto corners = getCorners();
        for (const auto& corner : corners)
        {
            if (corner.x < margin || corner.x > arenaWidth - margin ||
                corner.y < margin || corner.y > arenaHeight - margin)
                return false;
        }
        return checkCommonPlacement (obstacles, tanks);
    }

protected:
    bool checkWallShellCollision (const Shell& shell, Vec2& collisionPoint, Vec2& normal) const
    {
        auto corners = getCorners();
        Vec2 shellPrev = shell.getPreviousPosition();
        Vec2 shellCur = shell.getPosition();

        for (int i = 0; i < 4; ++i)
        {
            Vec2 p1 = corners[i];
            Vec2 p2 = corners[(i + 1) % 4];

            Vec2 intersection;
            if (lineSegmentIntersection (shellPrev, shellCur, p1, p2, intersection))
            {
                collisionPoint = intersection;
                Vec2 edge = p2 - p1;
                normal = Vec2 { -edge.y, edge.x }.normalized();

                if ((collisionPoint - position).dot (normal) < 0)
                    normal = normal * -1.0f;

                return true;
            }
        }

        // Check if shell is inside wall
        Vec2 shellPos = shell.getPosition();
        bool inside = true;
        for (int i = 0; i < 4; ++i)
        {
            Vec2 p1 = corners[i];
            Vec2 p2 = corners[(i + 1) % 4];
            Vec2 edge = p2 - p1;
            Vec2 toPoint = shellPos - p1;

            if (edge.x * toPoint.y - edge.y * toPoint.x < 0)
            {
                inside = false;
                break;
            }
        }

        if (inside)
        {
            collisionPoint = shellPos;
            normal = shell.getVelocity().normalized() * -1.0f;
            return true;
        }

        return false;
    }

    bool checkCommonPlacement (const std::vector<std::unique_ptr<Obstacle>>& obstacles, const std::vector<Tank*>& tanks) const;

    void drawWallPreview (Renderer& renderer, bool valid) const;
};

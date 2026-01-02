#include "Obstacle.h"
#include "Tank.h"
#include <algorithm>
#include <cmath>

Obstacle::Obstacle (ObstacleType type_, Vec2 pos, float ang, int owner)
    : type (type_), position (pos), angle (ang), ownerIndex (owner)
{
    if (type == ObstacleType::BreakableWall)
        health = config.breakableWallHealth;
    else if (type == ObstacleType::AutoTurret)
        health = config.turretHealth;
    else
        health = 9999.0f;  // Effectively indestructible

    reloadTimer = config.turretFireInterval;  // Start loaded
}

void Obstacle::update (float dt, const std::vector<Tank*>& tanks, float arenaWidth, float arenaHeight)
{
    if (!alive)
        return;

    // Update mine arming
    if (type == ObstacleType::Mine)
    {
        armTimer += dt;
    }

    // Update auto turret
    if (type == ObstacleType::AutoTurret)
    {
        updateAutoTurret (dt, tanks);
    }
}

void Obstacle::updateAutoTurret (float dt, const std::vector<Tank*>& tanks)
{
    // Update reload timer
    reloadTimer += dt;
    if (reloadTimer > config.turretFireInterval)
        reloadTimer = config.turretFireInterval;

    // Find nearest enemy tank
    Tank* target = findNearestEnemy (tanks);
    if (!target)
        return;

    // Rotate turret toward target
    Vec2 toTarget = target->getPosition() - position;
    float targetAngle = std::atan2 (toTarget.y, toTarget.x);

    float angleDiff = targetAngle - turretAngle;
    while (angleDiff > pi)
        angleDiff -= 2.0f * pi;
    while (angleDiff < -pi)
        angleDiff += 2.0f * pi;

    float maxRotation = config.turretRotationSpeedAuto * dt;
    if (std::abs (angleDiff) <= maxRotation)
        turretAngle = targetAngle;
    else
        turretAngle += (angleDiff > 0 ? 1.0f : -1.0f) * maxRotation;

    // Normalize turret angle
    while (turretAngle > pi)
        turretAngle -= 2.0f * pi;
    while (turretAngle < -pi)
        turretAngle += 2.0f * pi;

    // Fire if on target and loaded
    float dist = toTarget.length();
    if (dist < config.turretRange && reloadTimer >= config.turretFireInterval)
    {
        // Check if on target (within tolerance)
        float currentAngleDiff = std::abs (targetAngle - turretAngle);
        if (currentAngleDiff > pi)
            currentAngleDiff = 2.0f * pi - currentAngleDiff;

        if (currentAngleDiff < 0.1f)  // ~6 degrees
        {
            // Fire!
            Vec2 shellDir = Vec2::fromAngle (turretAngle);
            Vec2 shellPos = position + shellDir * 20.0f;
            Vec2 shellVel = shellDir * config.shellSpeed * 0.7f;  // Slower than player shells

            pendingShells.push_back (Shell (shellPos, shellVel, ownerIndex, config.turretRange, config.turretDamage));
            reloadTimer = 0.0f;
        }
    }
}

Tank* Obstacle::findNearestEnemy (const std::vector<Tank*>& tanks) const
{
    Tank* nearest = nullptr;
    float nearestDist = config.turretRange;

    for (Tank* tank : tanks)
    {
        if (!tank || !tank->isAlive())
            continue;

        // Don't shoot at the owner
        if (tank->getPlayerIndex() == ownerIndex)
            continue;

        float dist = (tank->getPosition() - position).length();
        if (dist < nearestDist)
        {
            nearestDist = dist;
            nearest = tank;
        }
    }

    return nearest;
}

std::array<Vec2, 4> Obstacle::getCorners() const
{
    float halfLength = config.wallLength / 2.0f;
    float halfThickness = config.wallThickness / 2.0f;

    float cosA = std::cos (angle);
    float sinA = std::sin (angle);

    Vec2 corners[4] = {
        { -halfLength, -halfThickness },
        { halfLength, -halfThickness },
        { halfLength, halfThickness },
        { -halfLength, halfThickness }
    };

    std::array<Vec2, 4> worldCorners;
    for (int i = 0; i < 4; ++i)
    {
        worldCorners[i].x = position.x + corners[i].x * cosA - corners[i].y * sinA;
        worldCorners[i].y = position.y + corners[i].x * sinA + corners[i].y * cosA;
    }

    return worldCorners;
}

void Obstacle::takeDamage (float damage)
{
    if (type != ObstacleType::BreakableWall && type != ObstacleType::AutoTurret && type != ObstacleType::Mine)
        return;

    health -= damage;
    if (health <= 0)
    {
        health = 0;
        alive = false;
    }
}

bool Obstacle::checkShellCollision (const Shell& shell, Vec2& collisionPoint, Vec2& normal) const
{
    if (!alive)
        return false;

    if (type == ObstacleType::Mine || type == ObstacleType::AutoTurret)
    {
        // Circle collision for mines and turrets
        Vec2 diff = shell.getPosition() - position;
        float dist = diff.length();
        float radius = (type == ObstacleType::Mine) ? config.mineRadius : 15.0f;

        if (dist < radius + shell.getRadius())
        {
            collisionPoint = shell.getPosition();
            normal = diff.normalized();
            return true;
        }
        return false;
    }

    // Wall collision - check line segment intersection
    auto corners = getCorners();
    Vec2 shellPrev = shell.getPreviousPosition();
    Vec2 shellCur = shell.getPosition();

    // Check against all 4 edges
    for (int i = 0; i < 4; ++i)
    {
        Vec2 p1 = corners[i];
        Vec2 p2 = corners[(i + 1) % 4];

        Vec2 intersection;
        if (lineSegmentIntersection (shellPrev, shellCur, p1, p2, intersection))
        {
            collisionPoint = intersection;

            // Calculate normal (perpendicular to edge, pointing outward)
            Vec2 edge = p2 - p1;
            normal = Vec2 { -edge.y, edge.x }.normalized();

            // Make sure normal points away from wall center
            if ((collisionPoint - position).dot (normal) < 0)
                normal = normal * -1.0f;

            return true;
        }
    }

    // Also check if shell is inside wall (point in polygon)
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
        // Push out in direction of shell velocity (reversed)
        normal = shell.getVelocity().normalized() * -1.0f;
        return true;
    }

    return false;
}

bool Obstacle::checkTankCollision (const Tank& tank, Vec2& pushDirection, float& pushDistance) const
{
    if (!alive)
        return false;

    if (type == ObstacleType::Mine)
    {
        // Circle collision for mines
        Vec2 diff = tank.getPosition() - position;
        float dist = diff.length();
        float combinedRadius = config.mineRadius + tank.getSize() * 0.4f;

        if (dist < combinedRadius)
        {
            pushDirection = diff.normalized();
            pushDistance = combinedRadius - dist;
            return true;
        }
        return false;
    }

    if (type == ObstacleType::AutoTurret)
    {
        // Circle collision for turrets
        Vec2 diff = tank.getPosition() - position;
        float dist = diff.length();
        float combinedRadius = 15.0f + tank.getSize() * 0.4f;

        if (dist < combinedRadius)
        {
            pushDirection = diff.normalized();
            pushDistance = combinedRadius - dist;
            return true;
        }
        return false;
    }

    if (type == ObstacleType::Pit)
    {
        // Circle collision for pits - tank falls in if center is inside
        Vec2 diff = tank.getPosition() - position;
        float dist = diff.length();

        if (dist < config.pitRadius)
        {
            pushDirection = diff.normalized();
            pushDistance = 0.0f;  // No push, tank is trapped
            return true;
        }
        return false;
    }

    if (type == ObstacleType::Portal)
    {
        // Circle collision for portals
        Vec2 diff = tank.getPosition() - position;
        float dist = diff.length();

        if (dist < config.portalRadius)
        {
            pushDirection = diff.normalized();
            pushDistance = 0.0f;  // No push, tank teleports
            return true;
        }
        return false;
    }

    // Wall collision - OBB vs OBB
    auto wallCorners = getCorners();
    auto tankCorners = tank.getCorners();

    // Separating Axis Theorem
    float minOverlap = 999999.0f;
    Vec2 minAxis;
    bool separated = false;

    // Test axes from both rectangles
    Vec2 axes[4] = {
        (wallCorners[1] - wallCorners[0]).normalized(),
        (wallCorners[3] - wallCorners[0]).normalized(),
        (tankCorners[1] - tankCorners[0]).normalized(),
        (tankCorners[3] - tankCorners[0]).normalized()
    };

    for (int a = 0; a < 4 && !separated; ++a)
    {
        Vec2 axis = axes[a];
        Vec2 perpAxis = { -axis.y, axis.x };

        float minA = 999999.0f, maxA = -999999.0f;
        float minB = 999999.0f, maxB = -999999.0f;

        for (int c = 0; c < 4; ++c)
        {
            float projA = wallCorners[c].dot (perpAxis);
            float projB = tankCorners[c].dot (perpAxis);
            minA = std::min (minA, projA);
            maxA = std::max (maxA, projA);
            minB = std::min (minB, projB);
            maxB = std::max (maxB, projB);
        }

        if (maxA < minB || maxB < minA)
        {
            separated = true;
        }
        else
        {
            float overlap = std::min (maxA - minB, maxB - minA);
            if (overlap < minOverlap)
            {
                minOverlap = overlap;
                minAxis = perpAxis;
            }
        }
    }

    if (!separated)
    {
        // Make sure push direction points from wall to tank
        Vec2 toTank = tank.getPosition() - position;
        if (toTank.dot (minAxis) < 0)
            minAxis = minAxis * -1.0f;

        pushDirection = minAxis;
        pushDistance = minOverlap / 2.0f + 1.0f;
        return true;
    }

    return false;
}

bool Obstacle::isValidPlacement (const std::vector<Obstacle>& obstacles, const std::vector<Tank*>& tanks, float arenaWidth, float arenaHeight) const
{
    // Check arena bounds
    float margin = 20.0f;

    if (type == ObstacleType::Mine || type == ObstacleType::AutoTurret)
    {
        float radius = (type == ObstacleType::Mine) ? config.mineRadius : 15.0f;
        if (position.x - radius < margin || position.x + radius > arenaWidth - margin ||
            position.y - radius < margin || position.y + radius > arenaHeight - margin)
            return false;
    }
    else
    {
        auto corners = getCorners();
        for (const auto& corner : corners)
        {
            if (corner.x < margin || corner.x > arenaWidth - margin ||
                corner.y < margin || corner.y > arenaHeight - margin)
                return false;
        }
    }

    // Check collision with other obstacles
    for (const auto& other : obstacles)
    {
        if (&other == this)
            continue;

        Vec2 diff = position - other.getPosition();
        float dist = diff.length();

        // Simple distance check for now
        float minDist = 50.0f;
        if (dist < minDist)
            return false;
    }

    // Check not too close to tanks
    for (Tank* tank : tanks)
    {
        if (!tank || !tank->isAlive())
            continue;

        Vec2 diff = position - tank->getPosition();
        float dist = diff.length();

        // Keep obstacles away from tanks
        float minDist = 80.0f;
        if (dist < minDist)
            return false;
    }

    return true;
}

bool Obstacle::lineSegmentIntersection (Vec2 p1, Vec2 p2, Vec2 p3, Vec2 p4, Vec2& intersection) const
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

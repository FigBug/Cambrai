#include "Obstacle.h"
#include "../Tank.h"
#include "../Renderer.h"
#include <algorithm>

bool Obstacle::checkCircleTankCollision (const Tank& tank, float radius, Vec2& pushDirection, float& pushDistance) const
{
    Vec2 diff = tank.getPosition() - position;
    float dist = diff.length();
    float combinedRadius = radius + tank.getSize() * 0.4f;

    if (dist < combinedRadius)
    {
        pushDirection = diff.normalized();
        pushDistance = combinedRadius - dist;
        return true;
    }
    return false;
}

bool Obstacle::isValidCirclePlacement (float radius, const std::vector<std::unique_ptr<Obstacle>>& obstacles, const std::vector<Tank*>& tanks, float arenaWidth, float arenaHeight) const
{
    float margin = 20.0f;

    if (position.x - radius < margin || position.x + radius > arenaWidth - margin ||
        position.y - radius < margin || position.y + radius > arenaHeight - margin)
        return false;

    for (const auto& other : obstacles)
    {
        if (other.get() == this)
            continue;

        Vec2 diff = position - other->getPosition();
        float dist = diff.length();
        float minDist = 50.0f;
        if (dist < minDist)
            return false;
    }

    for (Tank* tank : tanks)
    {
        if (!tank || !tank->isAlive())
            continue;

        Vec2 diff = position - tank->getPosition();
        float dist = diff.length();
        float minDist = 80.0f;
        if (dist < minDist)
            return false;
    }

    return true;
}

bool Wall::checkTankCollision (const Tank& tank, Vec2& pushDirection, float& pushDistance) const
{
    if (!alive)
        return false;

    auto wallCorners = getCorners();
    auto tankCorners = tank.getCorners();

    float minOverlap = 999999.0f;
    Vec2 minAxis;
    bool separated = false;

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
        Vec2 toTank = tank.getPosition() - position;
        if (toTank.dot (minAxis) < 0)
            minAxis = minAxis * -1.0f;

        pushDirection = minAxis;
        pushDistance = minOverlap / 2.0f + 1.0f;
        return true;
    }

    return false;
}

bool Wall::checkCommonPlacement (const std::vector<std::unique_ptr<Obstacle>>& obstacles, const std::vector<Tank*>& tanks) const
{
    for (const auto& other : obstacles)
    {
        if (other.get() == this)
            continue;

        Vec2 diff = position - other->getPosition();
        float dist = diff.length();
        float minDist = 50.0f;
        if (dist < minDist)
            return false;
    }

    for (Tank* tank : tanks)
    {
        if (!tank || !tank->isAlive())
            continue;

        Vec2 diff = position - tank->getPosition();
        float dist = diff.length();
        float minDist = 80.0f;
        if (dist < minDist)
            return false;
    }

    return true;
}

void Wall::drawWallPreview (Renderer& renderer, bool valid) const
{
    Color color = valid ? config.colorPlacementValid : config.colorPlacementInvalid;
    renderer.drawFilledRotatedRect (position, config.wallLength, config.wallThickness, angle, color);
}

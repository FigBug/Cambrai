#pragma once

#include "Obstacle.h"
#include "../Renderer.h"
#include "../Random.h"

class Portal : public Obstacle
{
public:
    Portal (Vec2 position, float angle, int ownerIndex)
        : Obstacle (position, angle, ownerIndex)
    {
        health = 9999.0f;
    }

    ObstacleType getType() const override { return ObstacleType::Portal; }
    float getCollisionRadius() const override { return config.portalRadius; }

    void takeDamage (float) override
    {
        // Indestructible
    }

    void update (float dt, const std::vector<Tank*>&, float, float) override
    {
        if (!alive)
            return;

        animTimer += dt;
    }

    ShellHitResult checkShellCollision (const Shell&, Vec2&, Vec2&) const override
    {
        // Shells pass through portals
        return ShellHitResult::Miss;
    }

    bool checkTankCollision (const Tank& tank, Vec2& pushDirection, float& pushDistance) override
    {
        if (!alive)
            return false;

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

    bool handleTankCollision (Tank& tank, const std::vector<std::unique_ptr<Obstacle>>& allObstacles) override
    {
        if (!tank.canUseTeleporter())
            return false;

        // Find all other portals
        std::vector<Obstacle*> otherPortals;
        for (auto& other : allObstacles)
        {
            if (other.get() != this && other->getType() == ObstacleType::Portal && other->isAlive())
                otherPortals.push_back (other.get());
        }

        // Teleport to random portal if there are others
        if (!otherPortals.empty())
        {
            Obstacle* destPortal = otherPortals[randomInt ((int) otherPortals.size())];
            tank.teleportTo (destPortal->getPosition());
        }

        return false;  // No physics push
    }

    bool isValidPlacement (const std::vector<std::unique_ptr<Obstacle>>& obstacles, const std::vector<Tank*>& tanks, float arenaWidth, float arenaHeight) const override
    {
        return isValidCirclePlacement (config.portalRadius, obstacles, tanks, arenaWidth, arenaHeight);
    }

    void draw (Renderer& renderer) const override
    {
        // Swirling portal effect
        float pulse = 0.8f + 0.2f * std::sin (animTimer * 3.0f);

        // Outer glow
        Color glowColor = { 100, 50, 200, 100 };
        renderer.drawFilledCircle (position, config.portalRadius * 1.2f * pulse, glowColor);

        // Main portal
        renderer.drawFilledCircle (position, config.portalRadius, config.colorPortal);

        // Inner swirl effect - concentric rings
        for (int i = 0; i < 3; ++i)
        {
            float offset = std::fmod (animTimer * 2.0f + i * 0.33f, 1.0f);
            float ringRadius = config.portalRadius * (0.3f + offset * 0.6f);
            unsigned char alpha = (unsigned char) (200 * (1.0f - offset));
            Color ringColor = { 150, 100, 255, alpha };
            renderer.drawCircle (position, ringRadius, ringColor);
        }

        // Center bright spot
        Color centerColor = { 200, 180, 255, 255 };
        renderer.drawFilledCircle (position, config.portalRadius * 0.2f, centerColor);
    }

    void drawPreview (Renderer& renderer, bool valid) const override
    {
        Color color = valid ? config.colorPlacementValid : config.colorPlacementInvalid;
        renderer.drawFilledCircle (position, config.portalRadius, color);
    }

private:
    float animTimer = 0.0f;
};

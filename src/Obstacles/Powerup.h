#pragma once

#include "Obstacle.h"
#include "../Renderer.h"
#include "../Tank.h"

class Powerup : public Obstacle
{
public:
    Powerup (Vec2 position, float angle, int ownerIndex)
        : Obstacle (position, angle, ownerIndex)
    {
        // Randomly assign powerup type based on angle (use angle as seed)
        int typeIndex = static_cast<int> (std::abs (angle * 1000.0f)) % 3;
        powerupType = static_cast<PowerupType> (typeIndex);
    }

    ObstacleType getType() const override { return ObstacleType::Powerup; }
    float getCollisionRadius() const override { return config.powerupRadius; }

    PowerupType getPowerupType() const { return powerupType; }

    // Track who collected the powerup (-1 if not collected)
    int getCollectedBy() const { return collectedBy; }
    bool isCollected() const { return collectedBy >= 0; }

    void update (float dt, const std::vector<Tank*>& tanks, float, float) override
    {
        if (!alive || collectedBy >= 0)
            return;

        // Rotation animation
        rotationAngle += dt * 2.0f;
        if (rotationAngle > 2.0f * pi)
            rotationAngle -= 2.0f * pi;

        // Check if any tank touches the powerup
        for (Tank* tank : tanks)
        {
            if (!tank || !tank->isAlive())
                continue;

            float dist = (tank->getPosition() - position).length();
            if (dist < config.powerupRadius + tank->getSize())
            {
                collectedBy = tank->getPlayerIndex();
                alive = false;
                return;
            }
        }
    }

    ShellHitResult checkShellCollision (const Shell&, Vec2&, Vec2&) const override
    {
        // Shells pass through powerups
        return ShellHitResult::Miss;
    }

    bool checkTankCollision (const Tank&, Vec2&, float&) const override
    {
        // Tanks don't collide with powerups (they collect them)
        return false;
    }

    bool isValidPlacement (const std::vector<std::unique_ptr<Obstacle>>& obstacles, const std::vector<Tank*>& tanks, float arenaWidth, float arenaHeight) const override
    {
        return isValidCirclePlacement (config.powerupRadius, obstacles, tanks, arenaWidth, arenaHeight);
    }

    void draw (Renderer& renderer) const override
    {
        if (!alive)
            return;

        Color color = getColor();

        // Outer glow
        Color glowColor = { color.r, color.g, color.b, 100 };
        renderer.drawFilledCircle (position, config.powerupRadius * 1.3f, glowColor);

        // Main body
        renderer.drawFilledCircle (position, config.powerupRadius, color);
        renderer.drawCircle (position, config.powerupRadius, config.colorWhite);

        // Icon based on type
        drawIcon (renderer);
    }

    void drawPreview (Renderer& renderer, bool valid) const override
    {
        Color color = valid ? config.colorPlacementValid : config.colorPlacementInvalid;
        renderer.drawFilledCircle (position, config.powerupRadius, color);
    }

private:
    PowerupType powerupType;
    int collectedBy = -1;
    float rotationAngle = 0.0f;

    static constexpr float pi = 3.14159265358979323846f;

    Color getColor() const
    {
        switch (powerupType)
        {
            case PowerupType::Speed:  return config.colorPowerupSpeed;
            case PowerupType::Damage: return config.colorPowerupDamage;
            case PowerupType::Armor:  return config.colorPowerupArmor;
        }
        return config.colorWhite;
    }

    void drawIcon (Renderer& renderer) const
    {
        Color iconColor = config.colorWhite;
        float r = config.powerupRadius * 0.5f;

        switch (powerupType)
        {
            case PowerupType::Speed:
            {
                // Lightning bolt / arrow pointing right
                Vec2 p1 = position + Vec2 { -r * 0.5f, -r * 0.6f };
                Vec2 p2 = position + Vec2 { r * 0.3f, -r * 0.1f };
                Vec2 p3 = position + Vec2 { -r * 0.2f, r * 0.1f };
                Vec2 p4 = position + Vec2 { r * 0.6f, r * 0.6f };
                renderer.drawLineThick (p1, p2, 2.0f, iconColor);
                renderer.drawLineThick (p2, p3, 2.0f, iconColor);
                renderer.drawLineThick (p3, p4, 2.0f, iconColor);
                break;
            }
            case PowerupType::Damage:
            {
                // Star / explosion shape
                for (int i = 0; i < 4; ++i)
                {
                    float a = rotationAngle + i * pi * 0.5f;
                    Vec2 start = position;
                    Vec2 end = position + Vec2::fromAngle (a) * r;
                    renderer.drawLineThick (start, end, 2.0f, iconColor);
                }
                break;
            }
            case PowerupType::Armor:
            {
                // Shield shape (chevron)
                Vec2 top = position + Vec2 { 0, -r * 0.6f };
                Vec2 left = position + Vec2 { -r * 0.5f, 0 };
                Vec2 right = position + Vec2 { r * 0.5f, 0 };
                Vec2 bottom = position + Vec2 { 0, r * 0.6f };
                renderer.drawLineThick (top, left, 2.0f, iconColor);
                renderer.drawLineThick (top, right, 2.0f, iconColor);
                renderer.drawLineThick (left, bottom, 2.0f, iconColor);
                renderer.drawLineThick (right, bottom, 2.0f, iconColor);
                break;
            }
        }
    }
};

#pragma once

#include "Obstacle.h"
#include "../Renderer.h"

class BreakableWall : public Wall
{
public:
    BreakableWall (Vec2 position, float angle, int ownerIndex)
        : Wall (position, angle, ownerIndex)
    {
        health = config.breakableWallHealth;
    }

    ObstacleType getType() const override { return ObstacleType::BreakableWall; }
    float getMaxHealth() const override { return config.breakableWallHealth; }

    ShellHitResult checkShellCollision (const Shell& shell, Vec2& collisionPoint, Vec2& normal) const override
    {
        if (!alive)
            return ShellHitResult::Miss;

        if (checkWallShellCollision (shell, collisionPoint, normal))
            return ShellHitResult::Destroyed;

        return ShellHitResult::Miss;
    }

    void draw (Renderer& renderer) const override
    {
        float healthPct = health / getMaxHealth();
        Color color = {
            (unsigned char) (config.colorBreakableWall.r * healthPct),
            (unsigned char) (config.colorBreakableWall.g * healthPct),
            (unsigned char) (config.colorBreakableWall.b * healthPct),
            255
        };
        renderer.drawFilledRotatedRect (position, config.wallLength, config.wallThickness, angle, color);

        // Damage cracks when damaged
        if (healthPct < 0.7f)
        {
            Color crackColor = { 50, 30, 20, 200 };
            float cosA = std::cos (angle);
            float sinA = std::sin (angle);
            for (int i = 0; i < 3; ++i)
            {
                float offset = ((float) i - 1.0f) * config.wallLength * 0.25f;
                Vec2 crackStart = { position.x + offset * cosA, position.y + offset * sinA };
                Vec2 crackEnd = { crackStart.x - config.wallThickness * 0.4f * sinA, crackStart.y + config.wallThickness * 0.4f * cosA };
                renderer.drawLine (crackStart, crackEnd, crackColor);
            }
        }
    }

    void drawPreview (Renderer& renderer, bool valid) const override
    {
        drawWallPreview (renderer, valid);
    }
};

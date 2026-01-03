#pragma once

#include "Obstacle.h"
#include "../Renderer.h"

class RicochetWall : public Wall
{
public:
    RicochetWall (Vec2 position, float angle, int ownerIndex)
        : Wall (position, angle, ownerIndex)
    {
        health = 9999.0f;
    }

    ObstacleType getType() const override { return ObstacleType::RicochetWall; }

    void takeDamage (float) override
    {
        // Indestructible
    }

    ShellHitResult checkShellCollision (const Shell& shell, Vec2& collisionPoint, Vec2& normal) const override
    {
        if (!alive)
            return ShellHitResult::Miss;

        if (checkWallShellCollision (shell, collisionPoint, normal))
            return ShellHitResult::Ricochet;

        return ShellHitResult::Miss;
    }

    void draw (Renderer& renderer) const override
    {
        // Orange/red color to distinguish from reflective wall
        renderer.drawFilledRotatedRect (position, config.wallLength, config.wallThickness, angle, config.colorRicochetWall);

        // Multiple highlight lines to show "splitting" nature
        float cosA = std::cos (angle);
        float sinA = std::sin (angle);

        Color highlight = { 255, 200, 150, 120 };
        for (int i = -1; i <= 1; ++i)
        {
            float offset = i * config.wallThickness * 0.25f;
            Vec2 perpOffset = { -sinA * offset, cosA * offset };
            Vec2 start = { position.x - config.wallLength * 0.35f * cosA + perpOffset.x,
                           position.y - config.wallLength * 0.35f * sinA + perpOffset.y };
            Vec2 end = { position.x + config.wallLength * 0.35f * cosA + perpOffset.x,
                         position.y + config.wallLength * 0.35f * sinA + perpOffset.y };
            renderer.drawLine (start, end, highlight);
        }
    }

    void drawPreview (Renderer& renderer, bool valid) const override
    {
        drawWallPreview (renderer, valid);
    }
};

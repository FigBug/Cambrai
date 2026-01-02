#pragma once

#include "Obstacle.h"
#include "../Renderer.h"

class SolidWall : public Wall
{
public:
    SolidWall (Vec2 position, float angle, int ownerIndex)
        : Wall (position, angle, ownerIndex)
    {
        health = 9999.0f;
    }

    ObstacleType getType() const override { return ObstacleType::SolidWall; }

    void takeDamage (float) override
    {
        // Indestructible
    }

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
        renderer.drawFilledRotatedRect (position, config.wallLength, config.wallThickness, angle, config.colorSolidWall);
        Color outline = { 60, 60, 60, 255 };
        renderer.drawRotatedRect (position, config.wallLength, config.wallThickness, angle, outline);
    }

    void drawPreview (Renderer& renderer, bool valid) const override
    {
        drawWallPreview (renderer, valid);
    }
};

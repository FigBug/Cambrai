#pragma once

#include "Obstacle.h"
#include "../Renderer.h"

class ReflectiveWall : public Wall
{
public:
    ReflectiveWall (Vec2 position, float angle, int ownerIndex)
        : Wall (position, angle, ownerIndex)
    {
        health = 9999.0f;
    }

    ObstacleType getType() const override { return ObstacleType::ReflectiveWall; }

    void takeDamage (float) override
    {
        // Indestructible
    }

    ShellHitResult checkShellCollision (const Shell& shell, Vec2& collisionPoint, Vec2& normal) const override
    {
        if (!alive)
            return ShellHitResult::Miss;

        if (checkWallShellCollision (shell, collisionPoint, normal))
        {
            if (shell.canReflect())
                return ShellHitResult::Reflected;
            else
                return ShellHitResult::Destroyed;
        }

        return ShellHitResult::Miss;
    }

    void draw (Renderer& renderer) const override
    {
        renderer.drawFilledRotatedRect (position, config.wallLength, config.wallThickness, angle, config.colorReflectiveWall);

        // Shiny highlight
        Color highlight = { 220, 220, 255, 100 };
        float cosA = std::cos (angle);
        float sinA = std::sin (angle);
        Vec2 highlightStart = { position.x - config.wallLength * 0.4f * cosA, position.y - config.wallLength * 0.4f * sinA };
        Vec2 highlightEnd = { position.x + config.wallLength * 0.4f * cosA, position.y + config.wallLength * 0.4f * sinA };
        renderer.drawLineThick (highlightStart, highlightEnd, 2.0f, highlight);
    }

    void drawPreview (Renderer& renderer, bool valid) const override
    {
        drawWallPreview (renderer, valid);
    }
};

#pragma once

#include "Obstacle.h"
#include "../Renderer.h"

class Pit : public Obstacle
{
public:
    Pit (Vec2 position, float angle, int ownerIndex)
        : Obstacle (position, angle, ownerIndex)
    {
        health = 9999.0f;
    }

    ObstacleType getType() const override { return ObstacleType::Pit; }
    float getCollisionRadius() const override { return config.pitRadius; }

    void takeDamage (float) override
    {
        // Indestructible
    }

    ShellHitResult checkShellCollision (const Shell&, Vec2&, Vec2&) const override
    {
        // Shells pass over pits
        return ShellHitResult::Miss;
    }

    bool checkTankCollision (const Tank& tank, Vec2& pushDirection, float& pushDistance) override
    {
        if (!alive)
            return false;

        Vec2 diff = tank.getPosition() - position;
        float dist = diff.length();

        if (dist < config.pitRadius)
        {
            revealed = true;
            pushDirection = diff.normalized();
            pushDistance = 0.0f;  // No push, tank is trapped
            return true;
        }
        return false;
    }

    bool isValidPlacement (const std::vector<std::unique_ptr<Obstacle>>& obstacles, const std::vector<Tank*>& tanks, float arenaWidth, float arenaHeight) const override
    {
        return isValidCirclePlacement (config.pitRadius, obstacles, tanks, arenaWidth, arenaHeight);
    }

    void draw (Renderer& renderer) const override
    {
        unsigned char alpha = revealed ? 255 : 13;  // 0.05 * 255 ≈ 13

        // Dark pit with concentric rings for depth effect
        Color pitColor = config.colorPit;
        pitColor.a = alpha;
        renderer.drawFilledCircle (position, config.pitRadius, pitColor);

        // Inner darker ring
        Color innerColor = { 20, 15, 10, alpha };
        renderer.drawFilledCircle (position, config.pitRadius * 0.7f, innerColor);

        // Center darkest
        Color centerColor = { 10, 5, 0, alpha };
        renderer.drawFilledCircle (position, config.pitRadius * 0.4f, centerColor);

        // Outline
        Color outlineColor = { 60, 50, 40, alpha };
        renderer.drawCircle (position, config.pitRadius, outlineColor);
    }

    void drawPreview (Renderer& renderer, bool valid) const override
    {
        Color color = valid ? config.colorPlacementValid : config.colorPlacementInvalid;
        renderer.drawFilledCircle (position, config.pitRadius, color);
    }

private:
    bool revealed = false;
};

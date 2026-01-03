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
    }

    ObstacleType getType() const override { return ObstacleType::Powerup; }
    float getCollisionRadius() const override { return config.powerupRadius; }

    // Track who collected the health pack (-1 if not collected)
    int getCollectedBy() const { return collectedBy; }
    bool isCollected() const { return collectedBy >= 0; }

    // Returns true only once when health pack is first collected (consumes the collection event)
    bool consumeCollection()
    {
        if (collectedBy >= 0 && !effectApplied)
        {
            effectApplied = true;
            return true;
        }
        return false;
    }

    void update (float dt, const std::vector<Tank*>& tanks, float, float) override
    {
        if (!alive || collectedBy >= 0)
            return;

        // Pulse animation
        pulseTimer += dt * 3.0f;
        if (pulseTimer > 2.0f * pi)
            pulseTimer -= 2.0f * pi;

        // Check if any tank touches the health pack
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
        // Shells pass through health packs
        return ShellHitResult::Miss;
    }

    bool checkTankCollision (const Tank&, Vec2&, float&) override
    {
        // Tanks don't collide with health packs (they collect them)
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

        // Pulsing green color for health
        float pulse = 0.8f + 0.2f * std::sin (pulseTimer);
        Color color = { (unsigned char) (100 * pulse), (unsigned char) (220 * pulse), (unsigned char) (100 * pulse), 255 };

        // Outer glow
        Color glowColor = { color.r, color.g, color.b, 80 };
        renderer.drawFilledCircle (position, config.powerupRadius * 1.4f, glowColor);

        // Main body
        renderer.drawFilledCircle (position, config.powerupRadius, color);
        renderer.drawCircle (position, config.powerupRadius, config.colorWhite);

        // Draw cross/plus icon for health
        Color iconColor = config.colorWhite;
        float r = config.powerupRadius * 0.5f;
        float thickness = r * 0.4f;

        // Horizontal bar
        renderer.drawFilledRect ({ position.x - r, position.y - thickness / 2 }, r * 2, thickness, iconColor);
        // Vertical bar
        renderer.drawFilledRect ({ position.x - thickness / 2, position.y - r }, thickness, r * 2, iconColor);
    }

    void drawPreview (Renderer& renderer, bool valid) const override
    {
        Color color = valid ? config.colorPlacementValid : config.colorPlacementInvalid;
        renderer.drawFilledCircle (position, config.powerupRadius, color);

        // Show cross icon
        float r = config.powerupRadius * 0.5f;
        float thickness = r * 0.4f;
        renderer.drawFilledRect ({ position.x - r, position.y - thickness / 2 }, r * 2, thickness, color);
        renderer.drawFilledRect ({ position.x - thickness / 2, position.y - r }, thickness, r * 2, color);
    }

private:
    int collectedBy = -1;
    bool effectApplied = false;
    float pulseTimer = 0.0f;

    static constexpr float pi = 3.14159265358979323846f;
};

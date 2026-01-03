#pragma once

#include "Obstacle.h"
#include "../Renderer.h"
#include "../Tank.h"

class Flag : public Obstacle
{
public:
    Flag (Vec2 position, float angle, int ownerIndex)
        : Obstacle (position, angle, ownerIndex)
    {
    }

    ObstacleType getType() const override { return ObstacleType::Flag; }
    float getCollisionRadius() const override { return config.flagRadius; }

    // Override base class collection effect
    CollectionEffect consumeCollectionEffect() override
    {
        if (capturedBy >= 0 && !pointsAwarded)
        {
            pointsAwarded = true;
            return { capturedBy, config.flagPoints, 0 };
        }
        return {};
    }

    void update (float dt, const std::vector<Tank*>& tanks, float, float) override
    {
        if (!alive || capturedBy >= 0)
            return;

        // Check if any tank touches the flag
        for (Tank* tank : tanks)
        {
            if (!tank || !tank->isAlive())
                continue;

            float dist = (tank->getPosition() - position).length();
            if (dist < config.flagRadius + tank->getSize())
            {
                capturedBy = tank->getPlayerIndex();
                alive = false;
                return;
            }
        }
    }

    ShellHitResult checkShellCollision (const Shell&, Vec2&, Vec2&) const override
    {
        // Shells pass through flags
        return ShellHitResult::Miss;
    }

    bool checkTankCollision (const Tank&, Vec2&, float&) override
    {
        // Tanks don't collide with flags (they capture them)
        return false;
    }

    bool isValidPlacement (const std::vector<std::unique_ptr<Obstacle>>& obstacles, const std::vector<Tank*>& tanks, float arenaWidth, float arenaHeight) const override
    {
        return isValidCirclePlacement (config.flagRadius, obstacles, tanks, arenaWidth, arenaHeight);
    }

    void draw (Renderer& renderer) const override
    {
        if (!alive)
            return;

        // Flag pole
        Vec2 poleBase = position;
        Vec2 poleTop = { position.x, position.y - 25.0f };
        renderer.drawLineThick (poleBase, poleTop, 3.0f, config.colorFlagPole);

        // Flag (triangle waving to the right)
        Vec2 flagTop = poleTop;
        Vec2 flagBottom = { position.x, position.y - 10.0f };
        Vec2 flagTip = { position.x + 18.0f, position.y - 17.5f };

        // Draw as filled triangle using lines
        renderer.drawLineThick (flagTop, flagBottom, 2.0f, config.colorFlag);
        renderer.drawLineThick (flagTop, flagTip, 2.0f, config.colorFlag);
        renderer.drawLineThick (flagBottom, flagTip, 2.0f, config.colorFlag);

        // Fill effect - draw multiple horizontal lines
        for (float y = poleTop.y; y < flagBottom.y; y += 2.0f)
        {
            float t = (y - poleTop.y) / (flagBottom.y - poleTop.y);
            float xEnd = position.x + 18.0f * (1.0f - std::abs (t - 0.5f) * 2.0f);
            renderer.drawLine ({ position.x, y }, { xEnd, y }, config.colorFlag);
        }

        // Base circle
        renderer.drawFilledCircle (poleBase, 4.0f, config.colorFlagPole);
    }

    void drawPreview (Renderer& renderer, bool valid) const override
    {
        Color color = valid ? config.colorPlacementValid : config.colorPlacementInvalid;
        renderer.drawFilledCircle (position, config.flagRadius, color);
    }

private:
    int capturedBy = -1;  // Player index who captured, -1 if not captured
    bool pointsAwarded = false;
};

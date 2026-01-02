#pragma once

#include "Obstacle.h"
#include "../Renderer.h"

class Mine : public Obstacle
{
public:
    Mine (Vec2 position, float angle, int ownerIndex)
        : Obstacle (position, angle, ownerIndex)
    {
        health = 1.0f;
    }

    ObstacleType getType() const override { return ObstacleType::Mine; }
    float getCollisionRadius() const override { return config.mineRadius; }

    bool isArmed() const override { return armTimer >= config.mineArmTime; }
    float getArmProgress() const { return std::min (1.0f, armTimer / config.mineArmTime); }

    void update (float dt, const std::vector<Tank*>&, float, float) override
    {
        if (!alive)
            return;

        armTimer += dt;
    }

    ShellHitResult checkShellCollision (const Shell& shell, Vec2& collisionPoint, Vec2& normal) const override
    {
        if (!alive)
            return ShellHitResult::Miss;

        if (checkCircleCollision (shell, config.mineRadius, collisionPoint, normal))
            return ShellHitResult::Destroyed;

        return ShellHitResult::Miss;
    }

    bool checkTankCollision (const Tank& tank, Vec2& pushDirection, float& pushDistance) const override
    {
        if (!alive)
            return false;

        return checkCircleTankCollision (tank, config.mineRadius, pushDirection, pushDistance);
    }

    bool isValidPlacement (const std::vector<std::unique_ptr<Obstacle>>& obstacles, const std::vector<Tank*>& tanks, float arenaWidth, float arenaHeight) const override
    {
        return isValidCirclePlacement (config.mineRadius, obstacles, tanks, arenaWidth, arenaHeight);
    }

    void draw (Renderer& renderer) const override
    {
        float radius = config.mineRadius;
        Color color = isArmed() ? config.colorMineArmed : config.colorMine;

        renderer.drawFilledCircle (position, radius, color);
        renderer.drawCircle (position, radius, config.colorBlack);

        // Spikes
        int spikes = 8;
        for (int i = 0; i < spikes; ++i)
        {
            float spikeAngle = (2.0f * pi * i) / spikes;
            Vec2 spikeEnd = position + Vec2::fromAngle (spikeAngle) * (radius * 1.3f);
            renderer.drawLine (position + Vec2::fromAngle (spikeAngle) * radius, spikeEnd, config.colorBlack);
        }

        // Blinking light when armed
        if (isArmed())
        {
            float blink = std::fmod (armTimer * 4.0f, 1.0f);
            if (blink < 0.5f)
            {
                Color lightColor = { 255, 0, 0, 255 };
                renderer.drawFilledCircle (position, radius * 0.2f, lightColor);
            }
        }
        else
        {
            // Arming progress
            float progress = getArmProgress();
            Color progressColor = { 255, 200, 0, 255 };
            renderer.drawFilledCircle (position, radius * 0.3f * progress, progressColor);
        }
    }

    void drawPreview (Renderer& renderer, bool valid) const override
    {
        Color color = valid ? config.colorPlacementValid : config.colorPlacementInvalid;
        renderer.drawFilledCircle (position, config.mineRadius, color);
    }

private:
    float armTimer = 0.0f;
};

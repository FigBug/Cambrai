#pragma once

#include "Obstacle.h"
#include "../Renderer.h"
#include "../Tank.h"

class Electromagnet : public Obstacle
{
public:
    Electromagnet (Vec2 position, float angle, int ownerIndex)
        : Obstacle (position, angle, ownerIndex)
    {
        // Start with random phase based on position
        cycleTimer = std::fmod (position.x + position.y, config.electromagnetDutyCycle);
    }

    ObstacleType getType() const override { return ObstacleType::Electromagnet; }
    float getCollisionRadius() const override { return config.electromagnetRadius; }

    bool isActive() const { return active; }
    float getRange() const { return config.electromagnetRange; }
    float getForce() const { return config.electromagnetForce; }

    void update (float dt, const std::vector<Tank*>&, float, float) override
    {
        if (!alive)
            return;

        // Update duty cycle
        cycleTimer += dt;
        if (cycleTimer >= config.electromagnetDutyCycle)
            cycleTimer -= config.electromagnetDutyCycle;

        // On for first half, off for second half
        active = cycleTimer < config.electromagnetDutyCycle * 0.5f;

        // Update pulse animation
        if (active)
        {
            pulseTimer += dt * 3.0f;
            if (pulseTimer > 1.0f)
                pulseTimer = 0.0f;
        }
    }

    // Calculate pull force on a tank (to be applied by Game)
    Vec2 calculatePullForce (const Tank& tank) const
    {
        if (!alive || !active)
            return { 0, 0 };

        Vec2 toMagnet = position - tank.getPosition();
        float dist = toMagnet.length();

        if (dist < config.electromagnetRange && dist > config.electromagnetRadius)
        {
            // Force falls off with distance squared
            float strength = 1.0f - (dist / config.electromagnetRange);
            strength *= strength;  // Quadratic falloff
            return toMagnet.normalized() * config.electromagnetForce * strength;
        }

        return { 0, 0 };
    }

    ShellHitResult checkShellCollision (const Shell& shell, Vec2& collisionPoint, Vec2& normal) const override
    {
        if (!alive)
            return ShellHitResult::Miss;

        // Shells are destroyed when hitting the electromagnet
        if (checkCircleCollision (shell, config.electromagnetRadius, collisionPoint, normal))
            return ShellHitResult::Destroyed;

        return ShellHitResult::Miss;
    }

    bool checkTankCollision (const Tank& tank, Vec2& pushDirection, float& pushDistance) const override
    {
        if (!alive)
            return false;

        return checkCircleTankCollision (tank, config.electromagnetRadius, pushDirection, pushDistance);
    }

    bool isValidPlacement (const std::vector<std::unique_ptr<Obstacle>>& obstacles, const std::vector<Tank*>& tanks, float arenaWidth, float arenaHeight) const override
    {
        return isValidCirclePlacement (config.electromagnetRadius, obstacles, tanks, arenaWidth, arenaHeight);
    }

    void draw (Renderer& renderer) const override
    {
        if (!alive)
            return;

        Color baseColor = active ? config.colorElectromagnetOn : config.colorElectromagnetOff;

        // Draw range indicator when active (faint)
        if (active)
        {
            Color rangeColor = { baseColor.r, baseColor.g, baseColor.b, 30 };
            renderer.drawCircle (position, config.electromagnetRange, rangeColor);

            // Pulsing rings when active
            float pulseRadius = config.electromagnetRadius + (config.electromagnetRange - config.electromagnetRadius) * pulseTimer;
            Color pulseColor = { baseColor.r, baseColor.g, baseColor.b, (unsigned char) (100 * (1.0f - pulseTimer)) };
            renderer.drawCircle (position, pulseRadius, pulseColor);
        }

        // Main body
        renderer.drawFilledCircle (position, config.electromagnetRadius, baseColor);
        renderer.drawCircle (position, config.electromagnetRadius, config.colorBlack);

        // Inner core
        Color coreColor = active ? config.colorWhite : config.colorGreyDark;
        renderer.drawFilledCircle (position, config.electromagnetRadius * 0.4f, coreColor);

        // Magnetic field lines (decorative)
        if (active)
        {
            Color lineColor = { 255, 255, 255, 150 };
            for (int i = 0; i < 4; ++i)
            {
                float a = angle + i * pi * 0.5f;
                Vec2 inner = position + Vec2::fromAngle (a) * (config.electromagnetRadius * 0.5f);
                Vec2 outer = position + Vec2::fromAngle (a) * config.electromagnetRadius;
                renderer.drawLine (inner, outer, lineColor);
            }
        }
    }

    void drawPreview (Renderer& renderer, bool valid) const override
    {
        Color color = valid ? config.colorPlacementValid : config.colorPlacementInvalid;
        renderer.drawFilledCircle (position, config.electromagnetRadius, color);

        // Show range
        Color rangeColor = { color.r, color.g, color.b, 50 };
        renderer.drawCircle (position, config.electromagnetRange, rangeColor);
    }

private:
    bool active = true;
    float cycleTimer = 0.0f;
    float pulseTimer = 0.0f;

    static constexpr float pi = 3.14159265358979323846f;
};

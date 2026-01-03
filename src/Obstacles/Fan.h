#pragma once

#include "Obstacle.h"
#include "../Renderer.h"
#include "../Tank.h"

class Fan : public Obstacle
{
public:
    Fan (Vec2 position, float angle, int ownerIndex)
        : Obstacle (position, angle, ownerIndex)
    {
        health = 9999.0f;
    }

    ObstacleType getType() const override { return ObstacleType::Fan; }
    float getCollisionRadius() const override { return config.fanRadius; }

    void takeDamage (float) override
    {
        // Indestructible
    }

    void update (float dt, const std::vector<Tank*>&, float, float) override
    {
        if (!alive)
            return;

        // Animate fan blades
        bladeAngle += dt * 15.0f;
        if (bladeAngle > 2.0f * pi)
            bladeAngle -= 2.0f * pi;
    }

    // Override base class force methods
    Vec2 getTankForce (const Tank& tank) const override
    {
        return calculatePushForceAtPosition (tank.getPosition(), config.fanForce);
    }

    Vec2 getShellForce (Vec2 shellPos) const override
    {
        return calculatePushForceAtPosition (shellPos, config.fanForce * 3.0f);
    }

private:
    Vec2 calculatePushForceAtPosition (Vec2 targetPos, float force) const
    {
        if (!alive)
            return { 0, 0 };

        Vec2 fanDir = Vec2::fromAngle (angle);
        Vec2 toTarget = targetPos - position;
        float dist = toTarget.length();

        if (dist < config.fanRadius || dist > config.fanRange)
            return { 0, 0 };

        // Check if target is within the fan's cone
        float dotProduct = toTarget.normalized().dot (fanDir);
        if (dotProduct < 0.3f)  // ~72 degree cone
            return { 0, 0 };

        // Check lateral distance from fan centerline
        Vec2 projected = fanDir * toTarget.dot (fanDir);
        Vec2 perpendicular = toTarget - projected;
        float lateralDist = perpendicular.length();

        // Width of cone at this distance
        float coneWidth = (dist / config.fanRange) * config.fanWidth * 0.5f;
        if (lateralDist > coneWidth)
            return { 0, 0 };

        // Force falls off with distance
        float strength = 1.0f - (dist / config.fanRange);
        return fanDir * force * strength;
    }

public:

    ShellHitResult checkShellCollision (const Shell&, Vec2&, Vec2&) const override
    {
        // Shells pass through fans
        return ShellHitResult::Miss;
    }

    bool checkTankCollision (const Tank&, Vec2&, float&) override
    {
        // Tanks pass through fans
        return false;
    }

    bool isValidPlacement (const std::vector<std::unique_ptr<Obstacle>>& obstacles, const std::vector<Tank*>& tanks, float arenaWidth, float arenaHeight) const override
    {
        return isValidCirclePlacement (config.fanRadius, obstacles, tanks, arenaWidth, arenaHeight);
    }

    void draw (Renderer& renderer) const override
    {
        if (!alive)
            return;

        Vec2 fanDir = Vec2::fromAngle (angle);

        // Draw wind effect (faint lines in direction of blow)
        Color windColor = { 200, 200, 255, 40 };
        for (int i = 0; i < 5; ++i)
        {
            float offset = (float) i / 4.0f - 0.5f;
            Vec2 perpDir = { -fanDir.y, fanDir.x };
            Vec2 startPos = position + fanDir * config.fanRadius + perpDir * offset * 30.0f;
            Vec2 endPos = startPos + fanDir * config.fanRange * 0.8f;
            renderer.drawLine (startPos, endPos, windColor);
        }

        // Draw fan housing (circle)
        renderer.drawFilledCircle (position, config.fanRadius, config.colorFan);
        renderer.drawCircle (position, config.fanRadius, config.colorBlack);

        // Draw spinning blades
        for (int i = 0; i < 4; ++i)
        {
            float a = bladeAngle + i * pi * 0.5f;
            Vec2 bladeEnd = position + Vec2::fromAngle (a) * (config.fanRadius * 0.8f);
            renderer.drawLineThick (position, bladeEnd, 3.0f, config.colorFanBlade);
        }

        // Center hub
        renderer.drawFilledCircle (position, config.fanRadius * 0.2f, config.colorFanBlade);

        // Direction indicator
        Vec2 arrowTip = position + fanDir * (config.fanRadius + 8.0f);
        Vec2 arrowLeft = arrowTip - fanDir * 6.0f + Vec2 { -fanDir.y, fanDir.x } * 4.0f;
        Vec2 arrowRight = arrowTip - fanDir * 6.0f - Vec2 { -fanDir.y, fanDir.x } * 4.0f;
        renderer.drawLine (arrowTip, arrowLeft, config.colorBlack);
        renderer.drawLine (arrowTip, arrowRight, config.colorBlack);
    }

    void drawPreview (Renderer& renderer, bool valid) const override
    {
        Color color = valid ? config.colorPlacementValid : config.colorPlacementInvalid;
        renderer.drawFilledCircle (position, config.fanRadius, color);

        // Show direction
        Vec2 fanDir = Vec2::fromAngle (angle);
        Vec2 arrowEnd = position + fanDir * (config.fanRadius + 15.0f);
        renderer.drawLineThick (position, arrowEnd, 3.0f, color);
    }

private:
    float bladeAngle = 0.0f;

    static constexpr float pi = 3.14159265358979323846f;
};

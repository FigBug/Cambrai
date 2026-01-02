#pragma once

#include "Obstacle.h"
#include "../Renderer.h"
#include "../Tank.h"

class AutoTurret : public Obstacle
{
public:
    AutoTurret (Vec2 position, float angle, int ownerIndex)
        : Obstacle (position, angle, ownerIndex)
    {
        health = config.turretHealth;
        reloadTimer = config.turretFireInterval;  // Start loaded
    }

    ObstacleType getType() const override { return ObstacleType::AutoTurret; }
    float getCollisionRadius() const override { return 15.0f; }
    float getMaxHealth() const override { return config.turretHealth; }

    float getTurretAngle() const { return turretAngle; }
    float getReloadProgress() const { return reloadTimer / config.turretFireInterval; }

    void update (float dt, const std::vector<Tank*>& tanks, float, float) override
    {
        if (!alive)
            return;

        reloadTimer += dt;
        if (reloadTimer > config.turretFireInterval)
            reloadTimer = config.turretFireInterval;

        Tank* target = findNearestEnemy (tanks);
        if (!target)
            return;

        // Rotate turret toward target
        Vec2 toTarget = target->getPosition() - position;
        float targetAngle = std::atan2 (toTarget.y, toTarget.x);

        float angleDiff = targetAngle - turretAngle;
        while (angleDiff > pi)
            angleDiff -= 2.0f * pi;
        while (angleDiff < -pi)
            angleDiff += 2.0f * pi;

        float maxRotation = config.turretRotationSpeedAuto * dt;
        if (std::abs (angleDiff) <= maxRotation)
            turretAngle = targetAngle;
        else
            turretAngle += (angleDiff > 0 ? 1.0f : -1.0f) * maxRotation;

        // Normalize
        while (turretAngle > pi)
            turretAngle -= 2.0f * pi;
        while (turretAngle < -pi)
            turretAngle += 2.0f * pi;

        // Fire if on target and loaded
        float dist = toTarget.length();
        if (dist < config.turretRange && reloadTimer >= config.turretFireInterval)
        {
            float currentAngleDiff = std::abs (targetAngle - turretAngle);
            if (currentAngleDiff > pi)
                currentAngleDiff = 2.0f * pi - currentAngleDiff;

            if (currentAngleDiff < 0.1f)
            {
                Vec2 shellDir = Vec2::fromAngle (turretAngle);
                Vec2 shellPos = position + shellDir * 20.0f;
                Vec2 shellVel = shellDir * config.shellSpeed * 0.7f;

                pendingShells.push_back (Shell (shellPos, shellVel, ownerIndex, config.turretRange, config.turretDamage));
                reloadTimer = 0.0f;
            }
        }
    }

    ShellHitResult checkShellCollision (const Shell& shell, Vec2& collisionPoint, Vec2& normal) const override
    {
        if (!alive)
            return ShellHitResult::Miss;

        if (checkCircleCollision (shell, 15.0f, collisionPoint, normal))
            return ShellHitResult::Destroyed;

        return ShellHitResult::Miss;
    }

    bool checkTankCollision (const Tank& tank, Vec2& pushDirection, float& pushDistance) const override
    {
        if (!alive)
            return false;

        return checkCircleTankCollision (tank, 15.0f, pushDirection, pushDistance);
    }

    bool isValidPlacement (const std::vector<std::unique_ptr<Obstacle>>& obstacles, const std::vector<Tank*>& tanks, float arenaWidth, float arenaHeight) const override
    {
        return isValidCirclePlacement (15.0f, obstacles, tanks, arenaWidth, arenaHeight);
    }

    void draw (Renderer& renderer) const override
    {
        // Base
        renderer.drawFilledCircle (position, 15.0f, config.colorAutoTurret);
        renderer.drawCircle (position, 15.0f, config.colorBlack);

        // Barrel
        Vec2 barrelDir = Vec2::fromAngle (turretAngle);
        Vec2 barrelEnd = position + barrelDir * 25.0f;
        renderer.drawLineThick (position, barrelEnd, 4.0f, config.colorBarrel);

        // Reload indicator
        if (reloadTimer < config.turretFireInterval)
        {
            float progress = reloadTimer / config.turretFireInterval;
            Color reloadColor = { 255, (unsigned char) (255 * progress), 0, 200 };
            renderer.drawFilledCircle (position, 5.0f * progress, reloadColor);
        }
        else
        {
            renderer.drawFilledCircle (position, 5.0f, config.colorReloadReady);
        }
    }

    void drawPreview (Renderer& renderer, bool valid) const override
    {
        Color color = valid ? config.colorPlacementValid : config.colorPlacementInvalid;
        renderer.drawFilledCircle (position, 15.0f, color);
    }

private:
    float turretAngle = 0.0f;
    float reloadTimer = 0.0f;

    Tank* findNearestEnemy (const std::vector<Tank*>& tanks) const
    {
        Tank* nearest = nullptr;
        float nearestDist = config.turretRange;

        for (Tank* tank : tanks)
        {
            if (!tank || !tank->isAlive())
                continue;

            if (tank->getPlayerIndex() == ownerIndex)
                continue;

            float dist = (tank->getPosition() - position).length();
            if (dist < nearestDist)
            {
                nearestDist = dist;
                nearest = tank;
            }
        }

        return nearest;
    }
};

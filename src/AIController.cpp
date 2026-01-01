#include "AIController.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

AIController::AIController()
{
    // Random personality factor for variation between AI tanks
    personalityFactor = 0.9f + ((float) rand() / RAND_MAX) * 0.2f;
}

void AIController::update (float dt, const Tank& myTank, const std::vector<const Tank*>& enemies,
                           const std::vector<Shell>& shells, const std::vector<Obstacle>& obstacles,
                           float arenaWidth, float arenaHeight)
{
    moveInput = { 0, 0 };
    aimInput = { 0, 0 };
    fireInput = false;

    if (!myTank.isAlive())
        return;

    // Update wander timer
    wanderTimer -= dt;
    if (wanderTimer <= 0)
    {
        pickNewWanderTarget (arenaWidth, arenaHeight);
    }

    // Find best target
    const Tank* target = findBestTarget (myTank, enemies);

    // Calculate desired movement
    Vec2 desiredDirection = { 0, 0 };

    // Move toward wander target
    Vec2 toWander = wanderTarget - myTank.getPosition();
    float wanderDist = toWander.length();
    if (wanderDist > 20.0f)
    {
        desiredDirection = desiredDirection + toWander.normalized();
    }

    // Avoid obstacles
    Vec2 obstacleAvoid = avoidObstacles (myTank, obstacles);
    desiredDirection = desiredDirection + obstacleAvoid * 2.0f;

    // Avoid incoming shells
    Vec2 shellAvoid = avoidShells (myTank, shells);
    desiredDirection = desiredDirection + shellAvoid * 3.0f;

    // Avoid arena edges
    Vec2 pos = myTank.getPosition();
    float margin = config.aiWanderMargin;
    if (pos.x < margin)
        desiredDirection.x += (margin - pos.x) / margin;
    if (pos.x > arenaWidth - margin)
        desiredDirection.x -= (pos.x - (arenaWidth - margin)) / margin;
    if (pos.y < margin)
        desiredDirection.y += (margin - pos.y) / margin;
    if (pos.y > arenaHeight - margin)
        desiredDirection.y -= (pos.y - (arenaHeight - margin)) / margin;

    // Convert desired direction to tank controls
    if (desiredDirection.lengthSquared() > 0.01f)
    {
        desiredDirection = desiredDirection.normalized();

        // Calculate angle difference between tank heading and desired direction
        float desiredAngle = std::atan2 (desiredDirection.y, desiredDirection.x);
        float angleDiff = desiredAngle - myTank.getAngle();

        // Normalize to [-PI, PI]
        while (angleDiff > pi)
            angleDiff -= 2.0f * pi;
        while (angleDiff < -pi)
            angleDiff += 2.0f * pi;

        // Check if we should go forward or reverse
        bool goReverse = std::abs (angleDiff) > pi * 0.7f;

        if (goReverse)
        {
            // Adjust angle for reverse
            angleDiff = angleDiff > 0 ? angleDiff - pi : angleDiff + pi;
            moveInput.y = 0.5f * personalityFactor;  // Reverse
        }
        else
        {
            moveInput.y = -0.8f * personalityFactor;  // Forward
        }

        // Rotate toward desired direction
        if (std::abs (angleDiff) > 0.1f)
        {
            moveInput.x = std::clamp (angleDiff / 0.5f, -1.0f, 1.0f);
        }
    }

    // Aim and fire at target
    if (target)
    {
        Vec2 toTarget = target->getPosition() - myTank.getPosition();
        float targetDist = toTarget.length();

        // Set crosshair toward target with some prediction
        Vec2 targetVel = target->getVelocity();
        float shellTravelTime = targetDist / config.shellSpeed;
        Vec2 predictedPos = target->getPosition() + targetVel * shellTravelTime * 0.5f;

        Vec2 aimDir = (predictedPos - myTank.getPosition()).normalized();
        Vec2 targetCrosshair = myTank.getPosition() + aimDir * targetDist;
        Vec2 currentCrosshair = myTank.getCrosshairPosition();

        Vec2 crosshairDiff = targetCrosshair - currentCrosshair;
        if (crosshairDiff.length() > config.aiCrosshairTolerance)
        {
            aimInput = crosshairDiff.normalized();
        }

        // Fire if on target and in range
        if (targetDist < config.aiFireDistance * personalityFactor)
        {
            if (crosshairDiff.length() < config.aiCrosshairTolerance * 2.0f)
            {
                fireInput = true;
            }
        }
    }
    else
    {
        // No target - aim forward
        Vec2 forward = Vec2::fromAngle (myTank.getAngle());
        Vec2 targetCrosshair = myTank.getPosition() + forward * 200.0f;
        Vec2 currentCrosshair = myTank.getCrosshairPosition();
        Vec2 crosshairDiff = targetCrosshair - currentCrosshair;

        if (crosshairDiff.length() > 10.0f)
        {
            aimInput = crosshairDiff.normalized() * 0.5f;
        }
    }
}

void AIController::pickNewWanderTarget (float arenaWidth, float arenaHeight)
{
    float margin = config.aiWanderMargin;
    wanderTarget.x = margin + ((float) rand() / RAND_MAX) * (arenaWidth - 2.0f * margin);
    wanderTarget.y = margin + ((float) rand() / RAND_MAX) * (arenaHeight - 2.0f * margin);
    wanderTimer = config.aiWanderInterval * (0.8f + ((float) rand() / RAND_MAX) * 0.4f);
}

Vec2 AIController::avoidObstacles (const Tank& myTank, const std::vector<Obstacle>& obstacles) const
{
    Vec2 avoidance = { 0, 0 };
    Vec2 pos = myTank.getPosition();

    for (const auto& obstacle : obstacles)
    {
        if (!obstacle.isAlive())
            continue;

        Vec2 toMe = pos - obstacle.getPosition();
        float dist = toMe.length();

        float dangerDist = 100.0f;
        if (obstacle.getType() == ObstacleType::Mine)
            dangerDist = 80.0f;

        if (dist < dangerDist && dist > 0.1f)
        {
            float urgency = 1.0f - (dist / dangerDist);
            avoidance = avoidance + toMe.normalized() * urgency;
        }
    }

    return avoidance;
}

Vec2 AIController::avoidShells (const Tank& myTank, const std::vector<Shell>& shells) const
{
    Vec2 avoidance = { 0, 0 };
    Vec2 pos = myTank.getPosition();

    for (const auto& shell : shells)
    {
        if (!shell.isAlive())
            continue;

        // Don't dodge own shells
        if (shell.getOwnerIndex() == myTank.getPlayerIndex())
            continue;

        Vec2 shellPos = shell.getPosition();
        Vec2 shellVel = shell.getVelocity();

        // Predict where shell will be
        Vec2 toMe = pos - shellPos;
        float dist = toMe.length();

        // Check if shell is heading toward us
        float dot = toMe.dot (shellVel.normalized());
        if (dot > 0 && dist < 200.0f)
        {
            // Shell is coming toward us
            // Dodge perpendicular to shell direction
            Vec2 shellDir = shellVel.normalized();
            Vec2 perpendicular = { -shellDir.y, shellDir.x };

            // Choose which way to dodge based on our position
            float side = toMe.dot (perpendicular);
            if (side < 0)
                perpendicular = perpendicular * -1.0f;

            float urgency = 1.0f - (dist / 200.0f);
            avoidance = avoidance + perpendicular * urgency * 2.0f;
        }
    }

    return avoidance;
}

const Tank* AIController::findBestTarget (const Tank& myTank, const std::vector<const Tank*>& enemies) const
{
    const Tank* best = nullptr;
    float bestScore = -9999.0f;

    for (const Tank* enemy : enemies)
    {
        if (!enemy || !enemy->isAlive())
            continue;

        Vec2 toEnemy = enemy->getPosition() - myTank.getPosition();
        float dist = toEnemy.length();

        // Score based on distance (closer is better) and health (lower health is better)
        float distScore = 1.0f - std::min (1.0f, dist / 600.0f);
        float healthScore = 1.0f - (enemy->getHealth() / enemy->getMaxHealth());

        float score = distScore * 0.6f + healthScore * 0.4f;

        if (score > bestScore)
        {
            bestScore = score;
            best = enemy;
        }
    }

    return best;
}

Vec2 AIController::getPlacementPosition (float arenaWidth, float arenaHeight) const
{
    float margin = config.aiPlacementMargin;
    return {
        margin + ((float) rand() / RAND_MAX) * (arenaWidth - 2.0f * margin),
        margin + ((float) rand() / RAND_MAX) * (arenaHeight - 2.0f * margin)
    };
}

float AIController::getPlacementAngle() const
{
    return ((float) rand() / RAND_MAX) * 2.0f * pi;
}

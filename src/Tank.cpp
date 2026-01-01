#include "Tank.h"
#include "Config.h"
#include <algorithm>
#include <cmath>

Tank::Tank (int playerIndex_, Vec2 startPos, float startAngle, float tankSize)
    : playerIndex (playerIndex_), position (startPos), angle (startAngle),
      turretAngle (0.0f), size (tankSize)
{
    crosshairOffset = Vec2::fromAngle (angle) * config.crosshairStartDistance;
    reloadTimer = config.fireInterval; // Start loaded
}

void Tank::update (float dt, Vec2 moveInput, Vec2 aimInput, bool fireInput, float arenaWidth, float arenaHeight)
{
    // Handle destruction animation
    if (destroying)
    {
        destroyTimer += dt;
        if (destroyTimer > config.tankDestroyDuration)
            destroyTimer = config.tankDestroyDuration;

        // Slow down while being destroyed
        velocity *= 0.95f;
        position += velocity * dt;
        clampToArena (arenaWidth, arenaHeight);
        updateSmoke (dt);
        return;
    }

    // Calculate damage penalty (reduces speed and turn rate)
    float damagePercent = getDamagePercent();
    float damagePenalty = 1.0f - (damagePercent * config.tankDamagePenaltyMax);

    // Update reload timer
    reloadTimer += dt;
    if (reloadTimer > config.fireInterval)
        reloadTimer = config.fireInterval;

    // Fire if requested
    if (fireInput && isReadyToFire())
        fireShell();

    // TANK CONTROLS:
    // Left stick Y: Forward/Reverse
    // Left stick X: Rotate tank

    float driveInput = -moveInput.y;  // Negative because stick up is negative
    float rotateInput = moveInput.x;

    // Rotation: Tank can rotate while stationary or moving
    // Rotation is slightly slower while moving at speed
    float currentSpeed = velocity.length();
    float speedFactor = 1.0f - (currentSpeed / config.tankMaxSpeed) * (1.0f - config.tankRotateWhileMoving);
    float effectiveRotateSpeed = config.tankRotateSpeed * speedFactor * damagePenalty;

    if (std::abs (rotateInput) > 0.1f)
    {
        angle += rotateInput * effectiveRotateSpeed * dt;

        // Keep angle in [-PI, PI]
        while (angle > pi)
            angle -= 2.0f * pi;
        while (angle < -pi)
            angle += 2.0f * pi;
    }

    // Movement: Forward/Reverse based on drive input
    Vec2 forward = Vec2::fromAngle (angle);

    float effectiveMaxSpeed = (driveInput >= 0 ? config.tankMaxSpeed : config.tankReverseSpeed) * damagePenalty;
    float targetSpeed = driveInput * effectiveMaxSpeed;

    // Current speed along tank's forward direction
    float currentForwardSpeed = velocity.dot (forward);

    // Acceleration/braking
    float accelRate = config.tankMaxSpeed / config.tankAccelTime;
    float brakeRate = config.tankMaxSpeed / config.tankBrakeTime;
    float coastRate = config.tankMaxSpeed / config.tankCoastStopTime;

    float speedDiff = targetSpeed - currentForwardSpeed;

    if (std::abs (driveInput) > 0.1f)
    {
        // Player is giving input
        if ((driveInput > 0 && currentForwardSpeed < 0) || (driveInput < 0 && currentForwardSpeed > 0))
        {
            // Braking (going opposite direction) - fast stop
            float change = brakeRate * dt;
            if (currentForwardSpeed > 0)
                currentForwardSpeed = std::max (0.0f, currentForwardSpeed - change);
            else
                currentForwardSpeed = std::min (0.0f, currentForwardSpeed + change);
        }
        else
        {
            // Accelerating in desired direction
            float change = accelRate * dt;
            if (speedDiff > 0)
                currentForwardSpeed = std::min (currentForwardSpeed + change, targetSpeed);
            else
                currentForwardSpeed = std::max (currentForwardSpeed - change, targetSpeed);
        }
    }
    else
    {
        // No input - coast to stop
        float change = coastRate * dt;
        if (currentForwardSpeed > 0)
            currentForwardSpeed = std::max (0.0f, currentForwardSpeed - change);
        else if (currentForwardSpeed < 0)
            currentForwardSpeed = std::min (0.0f, currentForwardSpeed + change);
    }

    velocity = forward * currentForwardSpeed;
    throttle = currentForwardSpeed / config.tankMaxSpeed;  // For HUD display

    // Update position
    position += velocity * dt;

    // Clamp to arena
    clampToArena (arenaWidth, arenaHeight);

    // Update crosshair offset based on aim stick
    if (aimInput.lengthSquared() > 0.01f)
        crosshairOffset += aimInput * config.crosshairSpeed * dt;

    // Clamp crosshair to stay on screen
    Vec2 crosshairWorldPos = position + crosshairOffset;
    float margin = 10.0f;
    if (crosshairWorldPos.x < margin)
        crosshairOffset.x = margin - position.x;
    else if (crosshairWorldPos.x > arenaWidth - margin)
        crosshairOffset.x = arenaWidth - margin - position.x;
    if (crosshairWorldPos.y < margin)
        crosshairOffset.y = margin - position.y;
    else if (crosshairWorldPos.y > arenaHeight - margin)
        crosshairOffset.y = arenaHeight - margin - position.y;

    // Clamp crosshair to max distance
    float crosshairDist = crosshairOffset.length();
    if (crosshairDist > config.crosshairMaxDistance)
        crosshairOffset = crosshairOffset.normalized() * config.crosshairMaxDistance;

    // Update turret to aim at crosshair
    updateTurret (dt);

    // Update effects
    updateTrackMarks (dt);
    updateSmoke (dt);
}

void Tank::updateTurret (float dt)
{
    // Calculate target angle from tank to crosshair
    Vec2 toCrosshair = crosshairOffset;
    if (toCrosshair.lengthSquared() < 1.0f)
        return;

    // Target angle in world space
    float targetWorldAngle = std::atan2 (toCrosshair.y, toCrosshair.x);

    // Target angle relative to tank body
    float targetLocalAngle = targetWorldAngle - angle;

    // Normalize to [-PI, PI]
    while (targetLocalAngle > pi)
        targetLocalAngle -= 2.0f * pi;
    while (targetLocalAngle < -pi)
        targetLocalAngle += 2.0f * pi;

    // Rotate turret toward target
    float angleDiff = targetLocalAngle - turretAngle;

    // Normalize difference
    while (angleDiff > pi)
        angleDiff -= 2.0f * pi;
    while (angleDiff < -pi)
        angleDiff += 2.0f * pi;

    float maxRotation = config.turretRotationSpeed * dt;
    if (std::abs (angleDiff) <= maxRotation)
    {
        turretAngle = targetLocalAngle;
    }
    else
    {
        turretAngle += (angleDiff > 0 ? 1.0f : -1.0f) * maxRotation;
    }

    // Normalize turret angle
    while (turretAngle > pi)
        turretAngle -= 2.0f * pi;
    while (turretAngle < -pi)
        turretAngle += 2.0f * pi;
}

bool Tank::isTurretOnTarget() const
{
    Vec2 toCrosshair = crosshairOffset;
    if (toCrosshair.lengthSquared() < 1.0f)
        return true;

    float targetWorldAngle = std::atan2 (toCrosshair.y, toCrosshair.x);
    float targetLocalAngle = targetWorldAngle - angle;

    while (targetLocalAngle > pi)
        targetLocalAngle -= 2.0f * pi;
    while (targetLocalAngle < -pi)
        targetLocalAngle += 2.0f * pi;

    float angleDiff = std::abs (targetLocalAngle - turretAngle);
    if (angleDiff > pi)
        angleDiff = 2.0f * pi - angleDiff;

    return angleDiff <= config.turretOnTargetTolerance;
}

void Tank::clampToArena (float arenaWidth, float arenaHeight)
{
    auto corners = getCorners();

    float pushLeft = 0.0f, pushRight = 0.0f, pushUp = 0.0f, pushDown = 0.0f;

    for (const auto& corner : corners)
    {
        if (corner.x < 0)
            pushLeft = std::max (pushLeft, -corner.x);
        if (corner.x > arenaWidth)
            pushRight = std::max (pushRight, corner.x - arenaWidth);
        if (corner.y < 0)
            pushUp = std::max (pushUp, -corner.y);
        if (corner.y > arenaHeight)
            pushDown = std::max (pushDown, corner.y - arenaHeight);
    }

    if (pushLeft > 0)
    {
        position.x += pushLeft;
        velocity.x = std::abs (velocity.x) * config.wallBounceMultiplier;
    }
    else if (pushRight > 0)
    {
        position.x -= pushRight;
        velocity.x = -std::abs (velocity.x) * config.wallBounceMultiplier;
    }

    if (pushUp > 0)
    {
        position.y += pushUp;
        velocity.y = std::abs (velocity.y) * config.wallBounceMultiplier;
    }
    else if (pushDown > 0)
    {
        position.y -= pushDown;
        velocity.y = -std::abs (velocity.y) * config.wallBounceMultiplier;
    }
}

Color Tank::getColor() const
{
    switch (playerIndex)
    {
        case 0:
            return config.colorTankRed;
        case 1:
            return config.colorTankBlue;
        case 2:
            return config.colorTankGreen;
        case 3:
            return config.colorTankYellow;
        default:
            return config.colorGrey;
    }
}

void Tank::takeDamage (float damage, int attackerIndex)
{
    if (destroying)
        return;

    health -= damage;
    if (health <= 0)
    {
        health = 0;
        destroying = true;
        destroyTimer = 0.0f;
        killerIndex = attackerIndex;
    }
}

void Tank::applyCollision (Vec2 pushDirection, float pushDistance, Vec2 impulse)
{
    position += pushDirection * pushDistance;
    velocity += impulse;
}

std::array<Vec2, 4> Tank::getCorners() const
{
    float halfSize = size / 2.0f;

    float cosA = std::cos (angle);
    float sinA = std::sin (angle);

    // Tank is square-ish but slightly longer than wide
    float halfLength = halfSize * 1.2f;
    float halfWidth = halfSize * 0.8f;

    Vec2 corners[4] = {
        { -halfLength, -halfWidth },
        { halfLength, -halfWidth },
        { halfLength, halfWidth },
        { -halfLength, halfWidth }
    };

    std::array<Vec2, 4> worldCorners;
    for (int i = 0; i < 4; ++i)
    {
        worldCorners[i].x = position.x + corners[i].x * cosA - corners[i].y * sinA;
        worldCorners[i].y = position.y + corners[i].x * sinA + corners[i].y * cosA;
    }

    return worldCorners;
}

bool Tank::fireShell()
{
    if (reloadTimer < config.fireInterval || !isTurretOnTarget())
        return false;

    // Shell fires from turret
    float worldTurretAngle = angle + turretAngle;
    Vec2 turretDir = Vec2::fromAngle (worldTurretAngle);

    // Barrel tip position
    float barrelLength = size * 0.7f;
    Vec2 shellPos = position + turretDir * barrelLength;

    // Shell velocity
    Vec2 shellVel = turretDir * config.shellSpeed;

    pendingShells.push_back (Shell (shellPos, shellVel, playerIndex));
    reloadTimer = 0.0f;

    return true;
}

void Tank::setCrosshairPosition (Vec2 worldPos)
{
    crosshairOffset = worldPos - position;

    float dist = crosshairOffset.length();
    if (dist > config.crosshairMaxDistance)
    {
        crosshairOffset = crosshairOffset.normalized() * config.crosshairMaxDistance;
    }
}

void Tank::updateTrackMarks (float dt)
{
    float fadeRate = 1.0f / config.trackMarkFadeTime;

    // Fade existing track marks
    for (auto it = trackMarks.begin(); it != trackMarks.end();)
    {
        it->alpha -= fadeRate * dt;
        if (it->alpha <= 0.0f)
            it = trackMarks.erase (it);
        else
            ++it;
    }

    // Spawn new track marks when moving
    float speed = velocity.length();
    if (isAlive() && speed > 0.5f)
    {
        trackMarkTimer += dt;
        if (trackMarkTimer >= config.trackMarkSpawnInterval)
        {
            trackMarkTimer = 0.0f;
            trackMarks.push_back ({ position, angle, 1.0f });
        }
    }
}

void Tank::updateSmoke (float dt)
{
    // Fade existing smoke
    for (auto it = smoke.begin(); it != smoke.end();)
    {
        it->alpha -= it->fadeRate * dt;
        if (it->alpha <= 0.0f)
            it = smoke.erase (it);
        else
            ++it;
    }

    // Spawn smoke when damaged or destroying
    float damagePercent = getDamagePercent();
    float destroyFactor = destroying ? (1.0f - getDestroyProgress()) : 1.0f;

    if (destroyFactor <= 0.0f)
        return;

    if (damagePercent > 0.3f || destroying)
    {
        smokeSpawnTimer += dt;

        float spawnInterval = config.smokeBaseSpawnInterval / ((1.0f + damagePercent * config.smokeDamageMultiplier) * destroyFactor);

        while (smokeSpawnTimer >= spawnInterval)
        {
            smokeSpawnTimer -= spawnInterval;

            Vec2 spawnPos = position;

            // Random offset on damaged/destroying tanks
            if (damagePercent > 0.3f || destroying)
            {
                float randomX = ((float) rand() / RAND_MAX - 0.5f) * size * 0.6f;
                float randomY = ((float) rand() / RAND_MAX - 0.5f) * size * 0.6f;
                spawnPos.x += randomX;
                spawnPos.y += randomY;
            }

            float baseRadius = config.smokeBaseRadius + damagePercent * 3.0f;
            float smokeRadius = baseRadius + ((float) rand() / RAND_MAX) * 2.0f;

            float startAlpha = (config.smokeBaseAlpha + damagePercent * 0.4f) * destroyFactor;

            float lifetime = config.smokeFadeTimeMin + ((float) rand() / RAND_MAX) * (config.smokeFadeTimeMax - config.smokeFadeTimeMin);
            float fadeRate = 1.0f / lifetime;

            smoke.push_back ({ spawnPos, smokeRadius, startAlpha, fadeRate });
        }
    }
}

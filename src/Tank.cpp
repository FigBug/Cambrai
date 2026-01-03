#include "Tank.h"
#include "Config.h"
#include "Random.h"
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

    // Update trap timer
    if (trapTimer > 0.0f)
    {
        trapTimer -= dt;
        // When trap ends, start cooldown so tank can escape before being re-trapped
        if (trapTimer <= 0.0f)
        {
            trapTimer = 0.0f;
            startTeleportCooldown (config.portalCooldown);
        }
    }

    // Update teleport cooldown
    if (teleportCooldown > 0.0f)
        teleportCooldown -= dt;

    // Update powerup timers
    if (speedPowerupTimer > 0.0f)
        speedPowerupTimer -= dt;
    if (damagePowerupTimer > 0.0f)
        damagePowerupTimer -= dt;
    if (armorPowerupTimer > 0.0f)
        armorPowerupTimer -= dt;

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

    // If trapped in pit, can't move or rotate
    if (isTrapped())
    {
        velocity = { 0.0f, 0.0f };
        throttle = 0.0f;
        externalForce = { 0, 0 };  // Clear accumulated forces so tank doesn't shoot out when released
        updateTurret (dt);
        updateSmoke (dt);
        return;
    }

    // TANK CONTROLS:
    // Left stick Y: Adjust throttle (throttle stays when released)
    // Left stick X: Rotate tank

    float throttleInput = -moveInput.y;  // Negative because stick up is negative
    float rotateInput = moveInput.x;

    // Adjust throttle based on stick input
    if (std::abs (throttleInput) > 0.1f)
    {
        throttle += throttleInput * config.tankThrottleRate * dt;
        throttle = std::clamp (throttle, -1.0f, 1.0f);
    }

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

    // Movement: Based on throttle
    Vec2 forward = Vec2::fromAngle (angle);

    float speedMultiplier = getSpeedMultiplier();
    float effectiveMaxSpeed = (throttle >= 0 ? config.tankMaxSpeed : config.tankReverseSpeed) * damagePenalty * speedMultiplier;
    float targetSpeed = throttle * effectiveMaxSpeed;

    // Current speed along tank's forward direction
    float currentForwardSpeed = velocity.dot (forward);

    // Acceleration toward target speed
    float accelRate = config.tankMaxSpeed / config.tankAccelTime;

    float speedDiff = targetSpeed - currentForwardSpeed;
    float change = accelRate * dt;

    if (speedDiff > 0)
        currentForwardSpeed = std::min (currentForwardSpeed + change, targetSpeed);
    else
        currentForwardSpeed = std::max (currentForwardSpeed - change, targetSpeed);

    velocity = forward * currentForwardSpeed;

    // Apply external force (from electromagnet)
    velocity += externalForce * dt;
    externalForce = { 0, 0 };  // Reset for next frame

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

    // Apply armor powerup reduction
    float actualDamage = damage * getArmorMultiplier();

    health -= actualDamage;
    if (health <= 0)
    {
        health = 0;
        destroying = true;
        destroyTimer = 0.0f;
        killerIndex = attackerIndex;
    }
}

void Tank::heal (float percent)
{
    if (destroying || !isAlive())
        return;

    float healAmount = config.tankMaxHealth * percent;
    health = std::min (health + healAmount, config.tankMaxHealth);
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

    pendingShells.push_back (Shell (shellPos, shellVel, playerIndex, config.shellMaxRange, config.shellDamage));
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

    // Spawn new track marks based on distance traveled
    float speed = velocity.length();
    if (isAlive() && speed > 0.1f)
    {
        trackMarkDistance += speed * dt;
        if (trackMarkDistance >= config.trackMarkSpawnDistance)
        {
            trackMarkDistance = 0.0f;
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
                float randomX = randomFloat (-0.5f, 0.5f) * size * 0.6f;
                float randomY = randomFloat (-0.5f, 0.5f) * size * 0.6f;
                spawnPos.x += randomX;
                spawnPos.y += randomY;
            }

            float baseRadius = config.smokeBaseRadius + damagePercent * 3.0f;
            float smokeRadius = baseRadius + randomFloat() * 2.0f;

            float startAlpha = (config.smokeBaseAlpha + damagePercent * 0.4f) * destroyFactor;

            float lifetime = randomFloat (config.smokeFadeTimeMin, config.smokeFadeTimeMax);
            float fadeRate = 1.0f / lifetime;

            smoke.push_back ({ spawnPos, smokeRadius, startAlpha, fadeRate });
        }
    }
}

void Tank::trapInPit (float duration)
{
    if (!isTrapped() && canUseTeleporter())
    {
        trapTimer = duration;
        velocity = { 0.0f, 0.0f };
        throttle = 0.0f;
        startTeleportCooldown (config.portalCooldown);
    }
}

void Tank::startTeleportCooldown (float duration)
{
    teleportCooldown = duration;
}

void Tank::teleportTo (Vec2 newPosition)
{
    position = newPosition;
    velocity = { 0.0f, 0.0f };
    startTeleportCooldown (config.portalCooldown);
}

void Tank::applySpeedPowerup (float duration)
{
    speedPowerupTimer = duration;
}

void Tank::applyDamagePowerup (float duration)
{
    damagePowerupTimer = duration;
}

void Tank::applyArmorPowerup (float duration)
{
    armorPowerupTimer = duration;
}

float Tank::getSpeedMultiplier() const
{
    return hasSpeedPowerup() ? (1.0f + config.powerupSpeedBonus) : 1.0f;
}

float Tank::getDamageMultiplier() const
{
    return hasDamagePowerup() ? (1.0f + config.powerupDamageBonus) : 1.0f;
}

float Tank::getArmorMultiplier() const
{
    return hasArmorPowerup() ? (1.0f - config.powerupArmorBonus) : 1.0f;
}

void Tank::applyExternalForce (Vec2 force)
{
    externalForce += force;
}

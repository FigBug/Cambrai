#pragma once

#include "FileSystemWatcher.h"
#include <raylib.h>
#include <memory>
#include <string>

// =============================================================================
// Game Configuration
// All tweakable game constants in one place
// =============================================================================

class Config : public FileSystemWatcher::Listener
{
public:
    Config();
    ~Config();

    bool load();
    bool save() const;
    void startWatching();

    // FileSystemWatcher::Listener
    void fileChanged (const std::string& file, FileSystemWatcher::Event event) override;

    // -------------------------------------------------------------------------
    // Tank Physics
    // -------------------------------------------------------------------------
    float tankMaxSpeed                = 5.0f;       // Maximum forward speed
    float tankReverseSpeed            = 3.0f;       // Maximum reverse speed
    float tankAccelTime               = 1.5f;       // Seconds to reach full speed
    float tankBrakeTime               = 0.8f;       // Seconds to stop when braking
    float tankCoastStopTime           = 3.0f;       // Seconds to stop when coasting
    float tankRotateSpeed             = 2.5f;       // Radians per second at full turn
    float tankRotateWhileMoving       = 0.7f;       // Rotation multiplier while moving (harder to turn)
    float tankDamagePenaltyMax        = 0.3f;       // Max speed/turn reduction at 0% health
    float tankDestroyDuration         = 2.0f;       // Seconds for destruction animation

    // -------------------------------------------------------------------------
    // Tank Health
    // -------------------------------------------------------------------------
    float tankMaxHealth               = 100.0f;
    float shellDamage                 = 35.0f;
    float mineDamage                  = 100.0f;     // Mines are instant kill
    float turretDamage                = 15.0f;      // Auto turret damage

    // -------------------------------------------------------------------------
    // Tank Turret
    // -------------------------------------------------------------------------
    float turretRotationSpeed         = 3.0f;       // Radians per second
    float turretOnTargetTolerance     = 0.05f;      // Radians (~3 degrees)

    // -------------------------------------------------------------------------
    // Shells / Firing
    // -------------------------------------------------------------------------
    float fireInterval                = 1.5f;       // Seconds between shots
    float shellSpeed                  = 400.0f;     // Pixels per second
    float shellRadius                 = 4.0f;
    float shellDamageRadius           = 15.0f;      // Splash damage radius
    int maxShellBounces               = 3;          // Max reflections off reflective walls

    // -------------------------------------------------------------------------
    // Crosshair / Aiming
    // -------------------------------------------------------------------------
    float crosshairSpeed              = 200.0f;     // How fast crosshair moves with stick
    float crosshairStartDistance      = 150.0f;     // Initial crosshair distance
    float crosshairMaxDistance        = 500.0f;     // Max crosshair distance from tank

    // -------------------------------------------------------------------------
    // Obstacles
    // -------------------------------------------------------------------------
    float wallThickness               = 20.0f;
    float wallLength                  = 100.0f;
    float breakableWallHealth         = 50.0f;
    float mineRadius                  = 15.0f;
    float mineArmTime                 = 2.0f;       // Seconds before mine becomes active
    float turretFireInterval          = 2.0f;       // Auto turret fire rate
    float turretRange                 = 300.0f;     // Auto turret detection range
    float turretRotationSpeedAuto     = 2.0f;       // Auto turret rotation speed

    // -------------------------------------------------------------------------
    // Smoke / Effects
    // -------------------------------------------------------------------------
    float smokeFadeTimeMin            = 2.0f;
    float smokeFadeTimeMax            = 4.0f;
    float smokeBaseSpawnInterval      = 0.1f;
    float smokeDamageMultiplier       = 3.0f;
    float smokeBaseRadius             = 3.0f;
    float smokeBaseAlpha              = 0.5f;

    // -------------------------------------------------------------------------
    // Track Marks
    // -------------------------------------------------------------------------
    float trackMarkFadeTime           = 8.0f;
    float trackMarkSpawnInterval      = 0.05f;
    float trackMarkWidth              = 4.0f;

    // -------------------------------------------------------------------------
    // Explosions
    // -------------------------------------------------------------------------
    float explosionDuration           = 0.4f;
    float explosionMaxRadius          = 40.0f;
    float destroyExplosionDuration    = 0.8f;
    float destroyExplosionMaxRadius   = 60.0f;

    // -------------------------------------------------------------------------
    // Collision
    // -------------------------------------------------------------------------
    float collisionRestitution        = 0.3f;
    float collisionDamageScale        = 10.0f;
    float wallBounceMultiplier        = 0.2f;

    // -------------------------------------------------------------------------
    // AI
    // -------------------------------------------------------------------------
    float aiWanderInterval            = 2.0f;
    float aiWanderMargin              = 100.0f;
    float aiFireDistance              = 350.0f;
    float aiCrosshairTolerance        = 20.0f;
    float aiPlacementMargin           = 150.0f;     // How far from edges AI places objects

    // -------------------------------------------------------------------------
    // Audio
    // -------------------------------------------------------------------------
    float audioGunSilenceDuration     = 0.15f;
    float audioPitchVariation         = 0.1f;
    float audioGainVariation          = 0.1f;
    float audioEngineBaseVolume       = 0.2f;
    float audioEngineThrottleBoost    = 0.5f;
    float audioMinImpactForSound      = 1.0f;

    // -------------------------------------------------------------------------
    // Game Flow
    // -------------------------------------------------------------------------
    int roundsToWin                   = 10;         // Best of 10 rounds
    float roundStartDelay             = 0.5f;       // Delay before round starts
    float placementTime               = 10.0f;      // Seconds for placement phase
    float roundOverDelay              = 3.0f;       // Delay before next round
    float gameOverDelay               = 5.0f;       // Delay before reset after 10 rounds
    int pointsForSurviving            = 1;
    int pointsForKill                 = 1;

    // -------------------------------------------------------------------------
    // Colors - Environment
    // -------------------------------------------------------------------------
    Color colorDirt                   = { 139, 119, 101, 255 };
    Color colorDirtDark               = { 119, 99, 81, 255 };
    Color colorDirtLight              = { 159, 139, 121, 255 };

    // -------------------------------------------------------------------------
    // Colors - Tanks (FFA mode)
    // -------------------------------------------------------------------------
    Color colorTankRed                = { 200, 60, 60, 255 };
    Color colorTankBlue               = { 60, 80, 200, 255 };
    Color colorTankGreen              = { 60, 180, 60, 255 };
    Color colorTankYellow             = { 200, 180, 60, 255 };

    // -------------------------------------------------------------------------
    // Colors - Obstacles
    // -------------------------------------------------------------------------
    Color colorSolidWall              = { 100, 100, 100, 255 };
    Color colorBreakableWall          = { 139, 90, 43, 255 };
    Color colorReflectiveWall         = { 180, 180, 220, 255 };
    Color colorMine                   = { 80, 80, 80, 255 };
    Color colorMineArmed              = { 200, 50, 50, 255 };
    Color colorAutoTurret             = { 60, 60, 60, 255 };
    Color colorAutoTurretBarrel       = { 40, 40, 40, 255 };

    // -------------------------------------------------------------------------
    // Colors - UI
    // -------------------------------------------------------------------------
    Color colorWhite                  = { 255, 255, 255, 255 };
    Color colorBlack                  = { 0, 0, 0, 255 };
    Color colorGrey                   = { 200, 200, 200, 255 };
    Color colorGreyDark               = { 80, 80, 80, 255 };
    Color colorGreyMid                = { 100, 100, 100, 255 };
    Color colorGreyLight              = { 150, 150, 150, 255 };
    Color colorGreySubtle             = { 120, 120, 120, 255 };
    Color colorBarBackground          = { 60, 60, 60, 255 };
    Color colorHudBackground          = { 30, 30, 30, 200 };

    // -------------------------------------------------------------------------
    // Colors - Title Screen
    // -------------------------------------------------------------------------
    Color colorTitle                  = { 255, 255, 255, 255 };
    Color colorSubtitle               = { 200, 200, 200, 255 };
    Color colorInstruction            = { 150, 150, 150, 255 };

    // -------------------------------------------------------------------------
    // Colors - Gameplay
    // -------------------------------------------------------------------------
    Color colorShell                  = { 50, 50, 50, 255 };
    Color colorShellTracer            = { 255, 200, 100, 255 };
    float shellTrailLength            = 15.0f;
    int shellTrailSegments            = 3;
    Color colorBarrel                 = { 40, 40, 40, 255 };
    Color colorReloadReady            = { 100, 255, 100, 255 };
    Color colorReloadNotReady         = { 255, 100, 100, 255 };
    Color colorTrackMark              = { 90, 70, 50, 128 };

    // -------------------------------------------------------------------------
    // Colors - Explosions
    // -------------------------------------------------------------------------
    Color colorExplosionOuter         = { 255, 150, 50, 200 };
    Color colorExplosionMid           = { 255, 220, 100, 180 };
    Color colorExplosionCore          = { 255, 255, 200, 150 };

    // -------------------------------------------------------------------------
    // Colors - Placement Phase
    // -------------------------------------------------------------------------
    Color colorPlacementValid         = { 100, 255, 100, 150 };
    Color colorPlacementInvalid       = { 255, 100, 100, 150 };
    Color colorPlacementTimer         = { 255, 220, 100, 255 };

private:
    std::string getConfigPath() const;
    std::string getConfigDirectory() const;

    std::unique_ptr<FileSystemWatcher> watcher;
};

// Global config instance
extern Config config;

#include "Config.h"
#include "Platform.h"

#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Global config instance
Config config;

namespace
{
json colorToJson (Color c)
{
    char hex[10];
    snprintf (hex, sizeof (hex), "#%02X%02X%02X%02X", c.r, c.g, c.b, c.a);
    return std::string (hex);
}

Color jsonToColor (const json& j, Color defaultColor)
{
    if (! j.is_string())
        return defaultColor;

    std::string s = j.get<std::string>();
    if (s.empty() || s[0] != '#' || (s.length() != 7 && s.length() != 9))
        return defaultColor;

    unsigned int r, g, b, a = 255;
    if (s.length() == 7)
    {
        if (sscanf (s.c_str(), "#%02X%02X%02X", &r, &g, &b) != 3)
            return defaultColor;
    }
    else
    {
        if (sscanf (s.c_str(), "#%02X%02X%02X%02X", &r, &g, &b, &a) != 4)
            return defaultColor;
    }

    return {
        static_cast<unsigned char> (r),
        static_cast<unsigned char> (g),
        static_cast<unsigned char> (b),
        static_cast<unsigned char> (a)
    };
}

template <typename T>
void loadValue (const json& j, const char* key, T& value)
{
    if (j.contains (key))
        value = j[key].get<T>();
}

void loadColor (const json& j, const char* key, Color& value)
{
    if (j.contains (key))
        value = jsonToColor (j[key], value);
}
} // namespace

Config::Config() = default;

Config::~Config()
{
    if (watcher)
    {
        watcher->removeListener (this);
        watcher.reset();
    }
}

std::string Config::getConfigDirectory() const
{
    return Platform::getUserDataDirectory();
}

std::string Config::getConfigPath() const
{
    std::string dir = Platform::getUserDataDirectory();
    if (dir.empty())
        return "";
    return dir + "/config.json";
}

bool Config::load()
{
    std::string path = getConfigPath();
    if (path.empty())
        return false;

    std::ifstream file (path);
    if (! file.is_open())
        return false;

    json j;
    try
    {
        file >> j;
    }
    catch (...)
    {
        return false;
    }

    // Check version - if it doesn't match, use defaults (don't load old config)
    std::string fileVersion;
    if (j.contains ("version") && j["version"].is_string())
        fileVersion = j["version"].get<std::string>();

    if (fileVersion != CAMBRAI_VERSION)
    {
        // Version mismatch - save new defaults and return
        save();
        return true;
    }

    auto getSection = [&j] (const char* name) -> const json&
    {
        static const json empty = json::object();
        return j.contains (name) ? j[name] : empty;
    };

    // Tank Physics
    {
        const auto& s = getSection ("tankPhysics");
        loadValue (s, "maxSpeed", tankMaxSpeed);
        loadValue (s, "reverseSpeed", tankReverseSpeed);
        loadValue (s, "accelTime", tankAccelTime);
        loadValue (s, "throttleRate", tankThrottleRate);
        loadValue (s, "rotateSpeed", tankRotateSpeed);
        loadValue (s, "rotateWhileMoving", tankRotateWhileMoving);
        loadValue (s, "damagePenaltyMax", tankDamagePenaltyMax);
        loadValue (s, "destroyDuration", tankDestroyDuration);
    }

    // Tank Health
    {
        const auto& s = getSection ("tankHealth");
        loadValue (s, "maxHealth", tankMaxHealth);
        loadValue (s, "shellDamage", shellDamage);
        loadValue (s, "mineDamage", mineDamage);
        loadValue (s, "turretDamage", turretDamage);
    }

    // Turret
    {
        const auto& s = getSection ("turret");
        loadValue (s, "rotationSpeed", turretRotationSpeed);
        loadValue (s, "onTargetTolerance", turretOnTargetTolerance);
    }

    // Shells
    {
        const auto& s = getSection ("shells");
        loadValue (s, "fireIntervalCrosshair", fireIntervalCrosshair);
        loadValue (s, "fireIntervalRotation", fireIntervalRotation);
        loadValue (s, "speed", shellSpeed);
        loadValue (s, "radius", shellRadius);
        loadValue (s, "damageRadius", shellDamageRadius);
        loadValue (s, "maxRange", shellMaxRange);
        loadValue (s, "maxBounces", maxShellBounces);
    }

    // Crosshair
    {
        const auto& s = getSection ("crosshair");
        loadValue (s, "speed", crosshairSpeed);
        loadValue (s, "startDistance", crosshairStartDistance);
        loadValue (s, "maxDistance", crosshairMaxDistance);
    }

    // Obstacles
    {
        const auto& s = getSection ("obstacles");
        loadValue (s, "wallThickness", wallThickness);
        loadValue (s, "wallLength", wallLength);
        loadValue (s, "breakableWallHealth", breakableWallHealth);
        loadValue (s, "mineRadius", mineRadius);
        loadValue (s, "mineArmTime", mineArmTime);
        loadValue (s, "turretFireInterval", turretFireInterval);
        loadValue (s, "turretRange", turretRange);
        loadValue (s, "turretRotationSpeedAuto", turretRotationSpeedAuto);
        loadValue (s, "turretHealth", turretHealth);
        loadValue (s, "pitRadius", pitRadius);
        loadValue (s, "pitTrapDuration", pitTrapDuration);
        loadValue (s, "portalRadius", portalRadius);
        loadValue (s, "portalCooldown", portalCooldown);
        loadValue (s, "flagRadius", flagRadius);
        loadValue (s, "flagPoints", flagPoints);
        loadValue (s, "healthPackRadius", healthPackRadius);
        loadValue (s, "electromagnetRadius", electromagnetRadius);
        loadValue (s, "electromagnetRange", electromagnetRange);
        loadValue (s, "electromagnetForce", electromagnetForce);
        loadValue (s, "electromagnetDutyCycle", electromagnetDutyCycle);
        loadValue (s, "fanRadius", fanRadius);
        loadValue (s, "fanRange", fanRange);
        loadValue (s, "fanWidth", fanWidth);
        loadValue (s, "fanForce", fanForce);
    }

    // Effects
    {
        const auto& s = getSection ("effects");
        loadValue (s, "smokeFadeTimeMin", smokeFadeTimeMin);
        loadValue (s, "smokeFadeTimeMax", smokeFadeTimeMax);
        loadValue (s, "smokeBaseSpawnInterval", smokeBaseSpawnInterval);
        loadValue (s, "smokeDamageMultiplier", smokeDamageMultiplier);
        loadValue (s, "smokeBaseRadius", smokeBaseRadius);
        loadValue (s, "smokeBaseAlpha", smokeBaseAlpha);
        loadValue (s, "smokeWindSpeed", smokeWindSpeed);
        loadValue (s, "trackMarkFadeTime", trackMarkFadeTime);
        loadValue (s, "trackMarkSpawnDistance", trackMarkSpawnDistance);
        loadValue (s, "trackMarkWidth", trackMarkWidth);
        loadValue (s, "trackMarkLength", trackMarkLength);
        loadValue (s, "explosionDuration", explosionDuration);
        loadValue (s, "explosionMaxRadius", explosionMaxRadius);
        loadValue (s, "destroyExplosionDuration", destroyExplosionDuration);
        loadValue (s, "destroyExplosionMaxRadius", destroyExplosionMaxRadius);
    }

    // Collision
    {
        const auto& s = getSection ("collision");
        loadValue (s, "restitution", collisionRestitution);
        loadValue (s, "damageScale", collisionDamageScale);
        loadValue (s, "wallBounceMultiplier", wallBounceMultiplier);
    }

    // AI
    {
        const auto& s = getSection ("ai");
        loadValue (s, "wanderInterval", aiWanderInterval);
        loadValue (s, "wanderMargin", aiWanderMargin);
        loadValue (s, "fireDistance", aiFireDistance);
        loadValue (s, "crosshairTolerance", aiCrosshairTolerance);
        loadValue (s, "placementMargin", aiPlacementMargin);
    }

    // User Preferences
    {
        const auto& s = getSection ("userPreferences");
        loadValue (s, "numPlayers", numPlayers);
        loadValue (s, "aimMode", aimMode);
    }

    // Audio
    {
        const auto& s = getSection ("audio");
        loadValue (s, "masterVolume", audioMasterVolume);
        loadValue (s, "gunSilenceDuration", audioGunSilenceDuration);
        loadValue (s, "pitchVariation", audioPitchVariation);
        loadValue (s, "gainVariation", audioGainVariation);
        loadValue (s, "engineBaseVolume", audioEngineBaseVolume);
        loadValue (s, "engineThrottleBoost", audioEngineThrottleBoost);
        loadValue (s, "minImpactForSound", audioMinImpactForSound);
    }

    // Game Flow
    {
        const auto& s = getSection ("gameFlow");
        loadValue (s, "roundsToWin", roundsToWin);
        loadValue (s, "roundStartDelay", roundStartDelay);
        loadValue (s, "placementTime", placementTime);
        loadValue (s, "roundOverDelay", roundOverDelay);
        loadValue (s, "gameOverDelay", gameOverDelay);
        loadValue (s, "pointsForSurviving", pointsForSurviving);
        loadValue (s, "pointsForKill", pointsForKill);
        loadValue (s, "stalemateTimeout", stalemateTimeout);
    }

    // Selection Phase
    {
        const auto& s = getSection ("selectionPhase");
        loadValue (s, "selectionTime", selectionTime);
        loadValue (s, "aiSelectionMinDelay", aiSelectionMinDelay);
        loadValue (s, "aiSelectionMaxDelay", aiSelectionMaxDelay);
        loadValue (s, "aiSelectionMoveInterval", aiSelectionMoveInterval);
    }

    // Shell Trail (in gameplay colors section of header but not colors)
    {
        const auto& s = getSection ("shellTrail");
        loadValue (s, "length", shellTrailLength);
        loadValue (s, "segments", shellTrailSegments);
    }

    // Colors - Environment
    {
        const auto& s = getSection ("colorsEnvironment");
        loadColor (s, "dirt", colorDirt);
        loadColor (s, "dirtDark", colorDirtDark);
        loadColor (s, "dirtLight", colorDirtLight);
    }

    // Colors - Tanks
    {
        const auto& s = getSection ("colorsTanks");
        loadColor (s, "red", colorTankRed);
        loadColor (s, "blue", colorTankBlue);
        loadColor (s, "green", colorTankGreen);
        loadColor (s, "yellow", colorTankYellow);
    }

    // Colors - Obstacles
    {
        const auto& s = getSection ("colorsObstacles");
        loadColor (s, "solidWall", colorSolidWall);
        loadColor (s, "breakableWall", colorBreakableWall);
        loadColor (s, "reflectiveWall", colorReflectiveWall);
        loadColor (s, "ricochetWall", colorRicochetWall);
        loadColor (s, "mine", colorMine);
        loadColor (s, "mineArmed", colorMineArmed);
        loadColor (s, "autoTurret", colorAutoTurret);
        loadColor (s, "autoTurretBarrel", colorAutoTurretBarrel);
        loadColor (s, "pit", colorPit);
        loadColor (s, "portal", colorPortal);
        loadColor (s, "flag", colorFlag);
        loadColor (s, "flagPole", colorFlagPole);
        loadColor (s, "electromagnetOn", colorElectromagnetOn);
        loadColor (s, "electromagnetOff", colorElectromagnetOff);
        loadColor (s, "fan", colorFan);
        loadColor (s, "fanBlade", colorFanBlade);
    }

    // Colors - UI
    {
        const auto& s = getSection ("colorsUI");
        loadColor (s, "white", colorWhite);
        loadColor (s, "black", colorBlack);
        loadColor (s, "grey", colorGrey);
        loadColor (s, "greyDark", colorGreyDark);
        loadColor (s, "greyMid", colorGreyMid);
        loadColor (s, "greyLight", colorGreyLight);
        loadColor (s, "greySubtle", colorGreySubtle);
        loadColor (s, "barBackground", colorBarBackground);
        loadColor (s, "hudBackground", colorHudBackground);
    }

    // Colors - Title Screen
    {
        const auto& s = getSection ("colorsTitleScreen");
        loadColor (s, "title", colorTitle);
        loadColor (s, "subtitle", colorSubtitle);
        loadColor (s, "instruction", colorInstruction);
    }

    // Colors - Gameplay
    {
        const auto& s = getSection ("colorsGameplay");
        loadColor (s, "shell", colorShell);
        loadColor (s, "shellTracer", colorShellTracer);
        loadColor (s, "barrel", colorBarrel);
        loadColor (s, "reloadReady", colorReloadReady);
        loadColor (s, "reloadNotReady", colorReloadNotReady);
        loadColor (s, "trackMark", colorTrackMark);
    }

    // Colors - Explosions
    {
        const auto& s = getSection ("colorsExplosions");
        loadColor (s, "outer", colorExplosionOuter);
        loadColor (s, "mid", colorExplosionMid);
        loadColor (s, "core", colorExplosionCore);
    }

    // Colors - Placement Phase
    {
        const auto& s = getSection ("colorsPlacement");
        loadColor (s, "valid", colorPlacementValid);
        loadColor (s, "invalid", colorPlacementInvalid);
        loadColor (s, "timer", colorPlacementTimer);
    }

    // Colors - Selection Phase
    {
        const auto& s = getSection ("colorsSelection");
        loadColor (s, "grid", colorSelectionGrid);
        loadColor (s, "cell", colorSelectionCell);
        loadColor (s, "taken", colorSelectionTaken);
        loadColor (s, "text", colorSelectionText);
    }

    return true;
}

bool Config::save() const
{
    std::string path = getConfigPath();
    if (path.empty())
        return false;

    json j;

    j["version"] = CAMBRAI_VERSION;

    // Tank Physics
    j["tankPhysics"] = {
        { "maxSpeed", tankMaxSpeed },
        { "reverseSpeed", tankReverseSpeed },
        { "accelTime", tankAccelTime },
        { "throttleRate", tankThrottleRate },
        { "rotateSpeed", tankRotateSpeed },
        { "rotateWhileMoving", tankRotateWhileMoving },
        { "damagePenaltyMax", tankDamagePenaltyMax },
        { "destroyDuration", tankDestroyDuration }
    };

    // Tank Health
    j["tankHealth"] = {
        { "maxHealth", tankMaxHealth },
        { "shellDamage", shellDamage },
        { "mineDamage", mineDamage },
        { "turretDamage", turretDamage }
    };

    // Turret
    j["turret"] = {
        { "rotationSpeed", turretRotationSpeed },
        { "onTargetTolerance", turretOnTargetTolerance }
    };

    // Shells
    j["shells"] = {
        { "fireIntervalCrosshair", fireIntervalCrosshair },
        { "fireIntervalRotation", fireIntervalRotation },
        { "speed", shellSpeed },
        { "radius", shellRadius },
        { "damageRadius", shellDamageRadius },
        { "maxRange", shellMaxRange },
        { "maxBounces", maxShellBounces }
    };

    // Crosshair
    j["crosshair"] = {
        { "speed", crosshairSpeed },
        { "startDistance", crosshairStartDistance },
        { "maxDistance", crosshairMaxDistance }
    };

    // Obstacles
    j["obstacles"] = {
        { "wallThickness", wallThickness },
        { "wallLength", wallLength },
        { "breakableWallHealth", breakableWallHealth },
        { "mineRadius", mineRadius },
        { "mineArmTime", mineArmTime },
        { "turretFireInterval", turretFireInterval },
        { "turretRange", turretRange },
        { "turretRotationSpeedAuto", turretRotationSpeedAuto },
        { "turretHealth", turretHealth },
        { "pitRadius", pitRadius },
        { "pitTrapDuration", pitTrapDuration },
        { "portalRadius", portalRadius },
        { "portalCooldown", portalCooldown },
        { "flagRadius", flagRadius },
        { "flagPoints", flagPoints },
        { "healthPackRadius", healthPackRadius },
        { "electromagnetRadius", electromagnetRadius },
        { "electromagnetRange", electromagnetRange },
        { "electromagnetForce", electromagnetForce },
        { "electromagnetDutyCycle", electromagnetDutyCycle },
        { "fanRadius", fanRadius },
        { "fanRange", fanRange },
        { "fanWidth", fanWidth },
        { "fanForce", fanForce }
    };

    // Effects
    j["effects"] = {
        { "smokeFadeTimeMin", smokeFadeTimeMin },
        { "smokeFadeTimeMax", smokeFadeTimeMax },
        { "smokeBaseSpawnInterval", smokeBaseSpawnInterval },
        { "smokeDamageMultiplier", smokeDamageMultiplier },
        { "smokeBaseRadius", smokeBaseRadius },
        { "smokeBaseAlpha", smokeBaseAlpha },
        { "smokeWindSpeed", smokeWindSpeed },
        { "trackMarkFadeTime", trackMarkFadeTime },
        { "trackMarkSpawnDistance", trackMarkSpawnDistance },
        { "trackMarkWidth", trackMarkWidth },
        { "trackMarkLength", trackMarkLength },
        { "explosionDuration", explosionDuration },
        { "explosionMaxRadius", explosionMaxRadius },
        { "destroyExplosionDuration", destroyExplosionDuration },
        { "destroyExplosionMaxRadius", destroyExplosionMaxRadius }
    };

    // Collision
    j["collision"] = {
        { "restitution", collisionRestitution },
        { "damageScale", collisionDamageScale },
        { "wallBounceMultiplier", wallBounceMultiplier }
    };

    // AI
    j["ai"] = {
        { "wanderInterval", aiWanderInterval },
        { "wanderMargin", aiWanderMargin },
        { "fireDistance", aiFireDistance },
        { "crosshairTolerance", aiCrosshairTolerance },
        { "placementMargin", aiPlacementMargin }
    };

    // User Preferences
    j["userPreferences"] = {
        { "numPlayers", numPlayers },
        { "aimMode", aimMode }
    };

    // Audio
    j["audio"] = {
        { "masterVolume", audioMasterVolume },
        { "gunSilenceDuration", audioGunSilenceDuration },
        { "pitchVariation", audioPitchVariation },
        { "gainVariation", audioGainVariation },
        { "engineBaseVolume", audioEngineBaseVolume },
        { "engineThrottleBoost", audioEngineThrottleBoost },
        { "minImpactForSound", audioMinImpactForSound }
    };

    // Game Flow
    j["gameFlow"] = {
        { "roundsToWin", roundsToWin },
        { "roundStartDelay", roundStartDelay },
        { "placementTime", placementTime },
        { "roundOverDelay", roundOverDelay },
        { "gameOverDelay", gameOverDelay },
        { "pointsForSurviving", pointsForSurviving },
        { "pointsForKill", pointsForKill },
        { "stalemateTimeout", stalemateTimeout }
    };

    // Selection Phase
    j["selectionPhase"] = {
        { "selectionTime", selectionTime },
        { "aiSelectionMinDelay", aiSelectionMinDelay },
        { "aiSelectionMaxDelay", aiSelectionMaxDelay },
        { "aiSelectionMoveInterval", aiSelectionMoveInterval }
    };

    // Shell Trail
    j["shellTrail"] = {
        { "length", shellTrailLength },
        { "segments", shellTrailSegments }
    };

    // Colors - Environment
    j["colorsEnvironment"] = {
        { "dirt", colorToJson (colorDirt) },
        { "dirtDark", colorToJson (colorDirtDark) },
        { "dirtLight", colorToJson (colorDirtLight) }
    };

    // Colors - Tanks
    j["colorsTanks"] = {
        { "red", colorToJson (colorTankRed) },
        { "blue", colorToJson (colorTankBlue) },
        { "green", colorToJson (colorTankGreen) },
        { "yellow", colorToJson (colorTankYellow) }
    };

    // Colors - Obstacles
    j["colorsObstacles"] = {
        { "solidWall", colorToJson (colorSolidWall) },
        { "breakableWall", colorToJson (colorBreakableWall) },
        { "reflectiveWall", colorToJson (colorReflectiveWall) },
        { "ricochetWall", colorToJson (colorRicochetWall) },
        { "mine", colorToJson (colorMine) },
        { "mineArmed", colorToJson (colorMineArmed) },
        { "autoTurret", colorToJson (colorAutoTurret) },
        { "autoTurretBarrel", colorToJson (colorAutoTurretBarrel) },
        { "pit", colorToJson (colorPit) },
        { "portal", colorToJson (colorPortal) },
        { "flag", colorToJson (colorFlag) },
        { "flagPole", colorToJson (colorFlagPole) },
        { "electromagnetOn", colorToJson (colorElectromagnetOn) },
        { "electromagnetOff", colorToJson (colorElectromagnetOff) },
        { "fan", colorToJson (colorFan) },
        { "fanBlade", colorToJson (colorFanBlade) }
    };

    // Colors - UI
    j["colorsUI"] = {
        { "white", colorToJson (colorWhite) },
        { "black", colorToJson (colorBlack) },
        { "grey", colorToJson (colorGrey) },
        { "greyDark", colorToJson (colorGreyDark) },
        { "greyMid", colorToJson (colorGreyMid) },
        { "greyLight", colorToJson (colorGreyLight) },
        { "greySubtle", colorToJson (colorGreySubtle) },
        { "barBackground", colorToJson (colorBarBackground) },
        { "hudBackground", colorToJson (colorHudBackground) }
    };

    // Colors - Title Screen
    j["colorsTitleScreen"] = {
        { "title", colorToJson (colorTitle) },
        { "subtitle", colorToJson (colorSubtitle) },
        { "instruction", colorToJson (colorInstruction) }
    };

    // Colors - Gameplay
    j["colorsGameplay"] = {
        { "shell", colorToJson (colorShell) },
        { "shellTracer", colorToJson (colorShellTracer) },
        { "barrel", colorToJson (colorBarrel) },
        { "reloadReady", colorToJson (colorReloadReady) },
        { "reloadNotReady", colorToJson (colorReloadNotReady) },
        { "trackMark", colorToJson (colorTrackMark) }
    };

    // Colors - Explosions
    j["colorsExplosions"] = {
        { "outer", colorToJson (colorExplosionOuter) },
        { "mid", colorToJson (colorExplosionMid) },
        { "core", colorToJson (colorExplosionCore) }
    };

    // Colors - Placement Phase
    j["colorsPlacement"] = {
        { "valid", colorToJson (colorPlacementValid) },
        { "invalid", colorToJson (colorPlacementInvalid) },
        { "timer", colorToJson (colorPlacementTimer) }
    };

    // Colors - Selection Phase
    j["colorsSelection"] = {
        { "grid", colorToJson (colorSelectionGrid) },
        { "cell", colorToJson (colorSelectionCell) },
        { "taken", colorToJson (colorSelectionTaken) },
        { "text", colorToJson (colorSelectionText) }
    };

    std::ofstream file (path);
    if (! file.is_open())
        return false;

    file << j.dump (4);
    return true;
}

void Config::startWatching()
{
    std::string dir = getConfigDirectory();
    if (dir.empty())
        return;

    watcher = std::make_unique<FileSystemWatcher>();
    watcher->addListener (this);
    watcher->addFolder (dir);
}

void Config::fileChanged (const std::string& file, FileSystemWatcher::Event event)
{
    std::string configPath = getConfigPath();
    if (file == configPath && event == FileSystemWatcher::Event::fileModified)
    {
        load();
    }
}

#include "Config.h"
#include "Platform.h"

#include <nlohmann/json.hpp>
#include <fstream>

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
}

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

    auto getSection = [&j] (const char* name) -> const json& {
        static const json empty = json::object();
        return j.contains (name) ? j[name] : empty;
    };

    // Tank Physics
    {
        const auto& s = getSection ("tankPhysics");
        loadValue (s, "maxSpeed", tankMaxSpeed);
        loadValue (s, "reverseSpeed", tankReverseSpeed);
        loadValue (s, "accelTime", tankAccelTime);
        loadValue (s, "brakeTime", tankBrakeTime);
        loadValue (s, "coastStopTime", tankCoastStopTime);
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
        loadValue (s, "fireInterval", fireInterval);
        loadValue (s, "speed", shellSpeed);
        loadValue (s, "radius", shellRadius);
        loadValue (s, "damageRadius", shellDamageRadius);
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
    }

    return true;
}

bool Config::save() const
{
    std::string path = getConfigPath();
    if (path.empty())
        return false;

    json j;

    j["version"] = "1.0.0";

    // Tank Physics
    j["tankPhysics"] = {
        { "maxSpeed", tankMaxSpeed },
        { "reverseSpeed", tankReverseSpeed },
        { "accelTime", tankAccelTime },
        { "brakeTime", tankBrakeTime },
        { "coastStopTime", tankCoastStopTime },
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
        { "fireInterval", fireInterval },
        { "speed", shellSpeed },
        { "radius", shellRadius },
        { "damageRadius", shellDamageRadius },
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
        { "turretRotationSpeedAuto", turretRotationSpeedAuto }
    };

    // Game Flow
    j["gameFlow"] = {
        { "roundsToWin", roundsToWin },
        { "roundStartDelay", roundStartDelay },
        { "placementTime", placementTime },
        { "roundOverDelay", roundOverDelay },
        { "gameOverDelay", gameOverDelay },
        { "pointsForSurviving", pointsForSurviving },
        { "pointsForKill", pointsForKill }
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

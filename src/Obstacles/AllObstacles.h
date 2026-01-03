#pragma once

#include "Obstacle.h"
#include "SolidWall.h"
#include "BreakableWall.h"
#include "ReflectiveWall.h"
#include "RicochetWall.h"
#include "Mine.h"
#include "AutoTurret.h"
#include "Pit.h"
#include "Portal.h"
#include "Flag.h"
#include "HealthPack.h"
#include "Electromagnet.h"
#include "Fan.h"

inline std::unique_ptr<Obstacle> createObstacle (ObstacleType type, Vec2 position, float angle, int ownerIndex)
{
    switch (type)
    {
        case ObstacleType::SolidWall:
            return std::make_unique<SolidWall> (position, angle, ownerIndex);
        case ObstacleType::BreakableWall:
            return std::make_unique<BreakableWall> (position, angle, ownerIndex);
        case ObstacleType::ReflectiveWall:
            return std::make_unique<ReflectiveWall> (position, angle, ownerIndex);
        case ObstacleType::RicochetWall:
            return std::make_unique<RicochetWall> (position, angle, ownerIndex);
        case ObstacleType::Mine:
            return std::make_unique<Mine> (position, angle, ownerIndex);
        case ObstacleType::AutoTurret:
            return std::make_unique<AutoTurret> (position, angle, ownerIndex);
        case ObstacleType::Pit:
            return std::make_unique<Pit> (position, angle, ownerIndex);
        case ObstacleType::Portal:
            return std::make_unique<Portal> (position, angle, ownerIndex);
        case ObstacleType::Flag:
            return std::make_unique<Flag> (position, angle, ownerIndex);
        case ObstacleType::HealthPack:
            return std::make_unique<HealthPack> (position, angle, ownerIndex);
        case ObstacleType::Electromagnet:
            return std::make_unique<Electromagnet> (position, angle, ownerIndex);
        case ObstacleType::Fan:
            return std::make_unique<Fan> (position, angle, ownerIndex);
    }
    return nullptr;
}

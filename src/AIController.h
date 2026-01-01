#pragma once

#include "Config.h"
#include "Obstacle.h"
#include "Shell.h"
#include "Tank.h"
#include "Vec2.h"
#include <vector>

class AIController
{
public:
    AIController();

    void update (float dt, const Tank& myTank, const std::vector<const Tank*>& enemies,
                 const std::vector<Shell>& shells, const std::vector<Obstacle>& obstacles,
                 float arenaWidth, float arenaHeight);

    Vec2 getMoveInput() const { return moveInput; }
    Vec2 getAimInput() const { return aimInput; }
    bool getFireInput() const { return fireInput; }

    // For placement phase
    Vec2 getPlacementPosition (float arenaWidth, float arenaHeight) const;
    float getPlacementAngle() const;

private:
    Vec2 moveInput;
    Vec2 aimInput;
    bool fireInput = false;

    Vec2 wanderTarget;
    float wanderTimer = 0.0f;

    float personalityFactor;  // Slight variation in behavior

    void pickNewWanderTarget (float arenaWidth, float arenaHeight);
    Vec2 avoidObstacles (const Tank& myTank, const std::vector<Obstacle>& obstacles) const;
    Vec2 avoidShells (const Tank& myTank, const std::vector<Shell>& shells) const;
    const Tank* findBestTarget (const Tank& myTank, const std::vector<const Tank*>& enemies) const;
};

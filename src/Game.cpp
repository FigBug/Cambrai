#include "Game.h"
#include <raylib.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>

Game::Game() = default;

Game::~Game() = default;

bool Game::init()
{
    srand ((unsigned int) ::time (nullptr));

    SetConfigFlags (FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow (WINDOW_WIDTH, WINDOW_HEIGHT, "Cambrai");
    SetTargetFPS (60);
    HideCursor();

    renderer = std::make_unique<Renderer>();
    audio = std::make_unique<Audio>();

    if (! audio->init())
    {
        audio.reset();
    }

    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        players[i] = std::make_unique<Player> (i);
    }
    for (int i = 0; i < MAX_TANKS; ++i)
    {
        aiControllers[i] = std::make_unique<AIController>();
    }

    state = GameState::Title;
    running = true;
    lastFrameTime = GetTime();

    return true;
}

void Game::run()
{
    while (running && !WindowShouldClose())
    {
        double currentTime = GetTime();
        float dt = (float) (currentTime - lastFrameTime);
        lastFrameTime = currentTime;

        if (dt > 0.1f)
            dt = 0.1f;

        handleEvents();
        update (dt);
        render();
    }
}

void Game::shutdown()
{
    tanks = {};
    players = {};
    aiControllers = {};
    renderer.reset();
    if (audio)
        audio->shutdown();
    audio.reset();

    CloseWindow();
}

void Game::handleEvents()
{
    if (IsKeyPressed (KEY_ESCAPE))
    {
        if (state == GameState::Title)
            running = false;
        else
        {
            resetGame();
            state = GameState::Title;
        }
    }
}

void Game::update (float dt)
{
    time += dt;

    if (audio)
        audio->update (dt);

    for (int i = 0; i < MAX_PLAYERS; ++i)
        players[i]->update();

    switch (state)
    {
        case GameState::Title:
            updateTitle (dt);
            break;
        case GameState::Placement:
            updatePlacement (dt);
            break;
        case GameState::Playing:
            updatePlaying (dt);
            break;
        case GameState::RoundOver:
            updateRoundOver (dt);
            break;
        case GameState::GameOver:
            updateGameOver (dt);
            break;
    }
}

void Game::updateTitle (float dt)
{
    if (anyButtonPressed())
    {
        startPlacement();
    }
}

bool Game::anyButtonPressed()
{
    if (IsMouseButtonPressed (MOUSE_BUTTON_LEFT))
        return true;

    for (int i = 0; i < 4; ++i)
    {
        if (IsGamepadAvailable (i))
        {
            if (IsGamepadButtonPressed (i, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) ||
                IsGamepadButtonPressed (i, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) ||
                IsGamepadButtonPressed (i, GAMEPAD_BUTTON_RIGHT_FACE_LEFT) ||
                IsGamepadButtonPressed (i, GAMEPAD_BUTTON_RIGHT_FACE_UP))
            {
                return true;
            }
        }
    }
    return false;
}

void Game::startPlacement()
{
    currentRound++;

    // Shuffle starting positions
    for (int i = MAX_TANKS - 1; i > 0; --i)
    {
        int j = rand() % (i + 1);
        std::swap (startPositionOrder[i], startPositionOrder[j]);
    }

    // Create tanks at randomized starting positions
    float tankSize = renderer->getTankSize();
    for (int i = 0; i < MAX_TANKS; ++i)
    {
        int posIndex = startPositionOrder[i];
        tanks[i] = std::make_unique<Tank> (i, getTankStartPosition (posIndex), getTankStartAngle (posIndex), tankSize);
    }

    shells.clear();
    explosions.clear();

    // Clear all obstacles on round 1, otherwise just remove destroyed ones
    if (currentRound == 1)
        obstacles.clear();
    else
    {
        // Remove destroyed obstacles (mines that exploded, breakable walls that were destroyed)
        obstacles.erase (
            std::remove_if (obstacles.begin(), obstacles.end(), [] (const std::unique_ptr<Obstacle>& o)
                            { return !o->isAlive(); }),
            obstacles.end());
    }

    // Assign random obstacles for placement
    assignRandomObstacles();

    // Initialize placement
    float w, h;
    getWindowSize (w, h);
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        hasPlaced[i] = false;
        placementPositions[i] = { w / 2.0f, h / 2.0f };
        placementAngles[i] = 0.0f;
    }
    placementTimer = config.placementTime;

    state = GameState::Placement;
}

void Game::assignRandomObstacles()
{
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        assignedObstacles[i] = getRandomObstacleType();
    }
}

ObstacleType Game::getRandomObstacleType()
{
    int r = rand() % 7;
    switch (r)
    {
        case 0: return ObstacleType::SolidWall;
        case 1: return ObstacleType::BreakableWall;
        case 2: return ObstacleType::ReflectiveWall;
        case 3: return ObstacleType::Mine;
        case 4: return ObstacleType::AutoTurret;
        case 5: return ObstacleType::Pit;
        case 6: return ObstacleType::Portal;
        default: return ObstacleType::SolidWall;
    }
}

void Game::updatePlacement (float dt)
{
    float w, h;
    getWindowSize (w, h);

    placementTimer -= dt;

    // Update placement for each player
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        if (hasPlaced[i])
            continue;

        bool isHuman = players[i]->isConnected();

        if (isHuman)
        {
            // Move placement position with aim stick
            Vec2 aimInput = players[i]->getAimInput();
            if (aimInput.lengthSquared() > 0.01f)
            {
                placementPositions[i] += aimInput * config.crosshairSpeed * dt;

                // Clamp to screen
                float margin = 50.0f;
                placementPositions[i].x = std::clamp (placementPositions[i].x, margin, w - margin);
                placementPositions[i].y = std::clamp (placementPositions[i].y, margin, h - margin);
            }

            // Mouse position for player 0
            if (players[i]->isUsingMouse())
            {
                placementPositions[i] = players[i]->getMousePosition();
            }

            // Rotate obstacle
            if (players[i]->getRotateInput())
            {
                placementAngles[i] += 2.0f * dt;
            }

            // Place obstacle
            if (players[i]->getPlaceInput())
            {
                // Create temporary obstacle to check validity
                auto temp = createObstacle (assignedObstacles[i], placementPositions[i], placementAngles[i], i);

                std::vector<Tank*> tankPtrs;
                for (auto& tank : tanks)
                    if (tank) tankPtrs.push_back (tank.get());

                if (temp->isValidPlacement (obstacles, tankPtrs, w, h))
                {
                    obstacles.push_back (createObstacle (assignedObstacles[i], placementPositions[i], placementAngles[i], i));
                    hasPlaced[i] = true;
                }
            }
        }
        else
        {
            // AI placement - pick random valid position
            for (int attempt = 0; attempt < 10; ++attempt)
            {
                Vec2 pos = aiControllers[i]->getPlacementPosition (w, h);
                float angle = aiControllers[i]->getPlacementAngle();

                auto temp = createObstacle (assignedObstacles[i], pos, angle, i);

                std::vector<Tank*> tankPtrs;
                for (auto& tank : tanks)
                    if (tank) tankPtrs.push_back (tank.get());

                if (temp->isValidPlacement (obstacles, tankPtrs, w, h))
                {
                    obstacles.push_back (createObstacle (assignedObstacles[i], pos, angle, i));
                    hasPlaced[i] = true;
                    break;
                }
            }

            // If couldn't place after attempts, mark as placed anyway
            if (!hasPlaced[i])
                hasPlaced[i] = true;
        }
    }

    // Check if placement is done
    bool allPlaced = true;
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        if (!hasPlaced[i])
            allPlaced = false;
    }

    if (allPlaced || placementTimer <= 0)
    {
        startRound();
    }
}

void Game::startRound()
{
    stateTimer = 0.0f;

    // Reset kills for this round
    for (int i = 0; i < MAX_PLAYERS; ++i)
        kills[i] = 0;

    roundWinner = -1;
    state = GameState::Playing;
}

void Game::updatePlaying (float dt)
{
    float arenaWidth, arenaHeight;
    getWindowSize (arenaWidth, arenaHeight);

    stateTimer += dt;

    // Update tanks
    for (int tankIdx = 0; tankIdx < MAX_TANKS; ++tankIdx)
    {
        if (! tanks[tankIdx] || ! tanks[tankIdx]->isVisible())
            continue;

        Vec2 moveInput, aimInput;
        bool fireInput = false;

        bool isHumanControlled = players[tankIdx]->isConnected();

        if (isHumanControlled)
        {
            moveInput = players[tankIdx]->getMoveInput();
            aimInput = players[tankIdx]->getAimInput();
            fireInput = (stateTimer > config.roundStartDelay) && players[tankIdx]->getFireInput();
        }
        else
        {
            // AI control
            std::vector<const Tank*> enemies;
            for (int j = 0; j < MAX_TANKS; ++j)
                if (j != tankIdx && tanks[j] && tanks[j]->isAlive())
                    enemies.push_back (tanks[j].get());

            aiControllers[tankIdx]->update (dt, *tanks[tankIdx], enemies, shells, obstacles, arenaWidth, arenaHeight);
            moveInput = aiControllers[tankIdx]->getMoveInput();
            aimInput = aiControllers[tankIdx]->getAimInput();
            fireInput = aiControllers[tankIdx]->getFireInput();
        }

        tanks[tankIdx]->update (dt, moveInput, aimInput, fireInput, arenaWidth, arenaHeight);

        // Mouse aiming
        if (isHumanControlled && players[tankIdx]->isUsingMouse())
            tanks[tankIdx]->setCrosshairPosition (players[tankIdx]->getMousePosition());

        // Collect shells
        auto& pendingShells = tanks[tankIdx]->getPendingShells();
        if (! pendingShells.empty() && audio)
        {
            audio->playCannon (tanks[tankIdx]->getPosition().x, arenaWidth);
        }

        for (auto& shell : pendingShells)
            shells.push_back (std::move (shell));

        pendingShells.clear();
    }

    // Update obstacles (auto turrets)
    std::vector<Tank*> tankPtrs;
    for (auto& tank : tanks)
        if (tank && tank->isAlive()) tankPtrs.push_back (tank.get());

    for (auto& obstacle : obstacles)
    {
        obstacle->update (dt, tankPtrs, arenaWidth, arenaHeight);

        // Collect shells from auto turrets
        auto& pendingShells = obstacle->getPendingShells();
        for (auto& shell : pendingShells)
            shells.push_back (std::move (shell));
        pendingShells.clear();
    }

    // Update engine volume
    if (audio)
    {
        float totalThrottle = 0.0f;
        int aliveCount = 0;
        for (int i = 0; i < MAX_TANKS; ++i)
        {
            if (tanks[i] && tanks[i]->isAlive())
            {
                totalThrottle += std::abs (tanks[i]->getThrottle());
                aliveCount++;
            }
        }
        float avgThrottle = aliveCount > 0 ? totalThrottle / aliveCount : 0.0f;
        audio->setEngineVolume (config.audioEngineBaseVolume + avgThrottle * config.audioEngineThrottleBoost);
    }

    // Update shells
    updateShells (dt);

    // Check collisions
    checkCollisions();

    // Update explosions
    for (auto& explosion : explosions)
        explosion.timer += dt;

    explosions.erase (
        std::remove_if (explosions.begin(), explosions.end(), [] (const Explosion& e)
                        { return ! e.isAlive(); }),
        explosions.end());

    // Check round over
    checkRoundOver();
}

void Game::updateShells (float dt)
{
    float arenaWidth, arenaHeight;
    getWindowSize (arenaWidth, arenaHeight);

    for (auto& shell : shells)
    {
        if (!shell.isAlive())
            continue;

        shell.update (dt);

        Vec2 pos = shell.getPosition();

        // Check arena bounds - destroy shell
        if (pos.x < 0 || pos.x > arenaWidth || pos.y < 0 || pos.y > arenaHeight)
        {
            shell.kill();
        }
    }

    shells.erase (
        std::remove_if (shells.begin(), shells.end(), [] (const Shell& s)
                        { return ! s.isAlive(); }),
        shells.end());
}

void Game::checkCollisions()
{
    float arenaWidth, arenaHeight;
    getWindowSize (arenaWidth, arenaHeight);

    // Shell-to-obstacle collisions
    for (auto& shell : shells)
    {
        if (!shell.isAlive())
            continue;

        for (auto& obstacle : obstacles)
        {
            if (!obstacle->isAlive())
                continue;

            Vec2 collisionPoint, normal;
            ShellHitResult result = obstacle->checkShellCollision (shell, collisionPoint, normal);

            if (result != ShellHitResult::Miss)
            {
                if (result == ShellHitResult::Reflected)
                {
                    shell.reflect (normal);
                }
                else  // Destroyed
                {
                    // Damage destructible obstacles
                    if (obstacle->getType() == ObstacleType::BreakableWall)
                    {
                        obstacle->takeDamage (config.shellDamage);
                    }
                    else if (obstacle->getType() == ObstacleType::AutoTurret)
                    {
                        obstacle->takeDamage (shell.getDamage());

                        Explosion explosion;
                        explosion.position = collisionPoint;
                        explosion.duration = config.explosionDuration;
                        explosion.maxRadius = config.explosionMaxRadius;
                        explosions.push_back (explosion);

                        if (!obstacle->isAlive())
                        {
                            Explosion destroyExplosion;
                            destroyExplosion.position = obstacle->getPosition();
                            destroyExplosion.duration = config.destroyExplosionDuration;
                            destroyExplosion.maxRadius = config.destroyExplosionMaxRadius;
                            explosions.push_back (destroyExplosion);
                        }

                        if (audio)
                            audio->playExplosion (collisionPoint.x, arenaWidth);
                    }

                    shell.kill();
                }

                if (!shell.isAlive())
                    break;
            }
        }
    }

    // Shell-to-tank collisions (raycast along shell path)
    for (auto& shell : shells)
    {
        if (!shell.isAlive())
            continue;

        Vec2 shellPrev = shell.getPreviousPosition();
        Vec2 shellCur = shell.getPosition();

        for (auto& tank : tanks)
        {
            if (!tank || !tank->isVisible())
                continue;

            if (tank->getPlayerIndex() == shell.getOwnerIndex())
                continue;  // Don't hit own tank

            Vec2 hitPoint;
            if (renderer->checkTankHitLine (*tank, shellPrev, shellCur, hitPoint))
            {
                tank->takeDamage (shell.getDamage(), shell.getOwnerIndex());

                Explosion explosion;
                explosion.position = hitPoint;
                explosion.duration = config.explosionDuration;
                explosion.maxRadius = config.explosionMaxRadius;
                explosions.push_back (explosion);

                if (audio)
                    audio->playExplosion (hitPoint.x, arenaWidth);

                // Track kill
                if (!tank->isAlive() && shell.getOwnerIndex() >= 0 && shell.getOwnerIndex() < MAX_PLAYERS)
                {
                    kills[shell.getOwnerIndex()]++;
                    scores[shell.getOwnerIndex()] += config.pointsForKill;

                    // Big explosion for destruction
                    Explosion destroyExplosion;
                    destroyExplosion.position = tank->getPosition();
                    destroyExplosion.duration = config.destroyExplosionDuration;
                    destroyExplosion.maxRadius = config.destroyExplosionMaxRadius;
                    explosions.push_back (destroyExplosion);
                }

                shell.kill();
                break;
            }
        }
    }

    // Tank-to-obstacle collisions
    for (auto& tank : tanks)
    {
        if (!tank || !tank->isAlive())
            continue;

        for (auto& obstacle : obstacles)
        {
            if (!obstacle->isAlive())
                continue;

            Vec2 pushDir;
            float pushDist;

            if (obstacle->checkTankCollision (*tank, pushDir, pushDist))
            {
                if (obstacle->getType() == ObstacleType::Mine && obstacle->isArmed())
                {
                    // Mine explodes - instant kill
                    tank->takeDamage (config.mineDamage, obstacle->getOwnerIndex());
                    obstacle->takeDamage (9999.0f);

                    Explosion explosion;
                    explosion.position = obstacle->getPosition();
                    explosion.duration = config.destroyExplosionDuration;
                    explosion.maxRadius = config.destroyExplosionMaxRadius;
                    explosions.push_back (explosion);

                    if (audio)
                        audio->playExplosion (obstacle->getPosition().x, arenaWidth);

                    // Track kill
                    if (!tank->isAlive() && obstacle->getOwnerIndex() >= 0 && obstacle->getOwnerIndex() < MAX_PLAYERS)
                    {
                        kills[obstacle->getOwnerIndex()]++;
                        scores[obstacle->getOwnerIndex()] += config.pointsForKill;

                        Explosion destroyExplosion;
                        destroyExplosion.position = tank->getPosition();
                        destroyExplosion.duration = config.destroyExplosionDuration;
                        destroyExplosion.maxRadius = config.destroyExplosionMaxRadius;
                        explosions.push_back (destroyExplosion);
                    }
                }
                else if (obstacle->getType() == ObstacleType::Pit)
                {
                    // Tank falls into pit - trapped for duration (if not on cooldown)
                    if (tank->canUseTeleporter())
                        tank->trapInPit (config.pitTrapDuration);
                }
                else if (obstacle->getType() == ObstacleType::Portal)
                {
                    // Teleport to another random portal (if not on cooldown)
                    if (tank->canUseTeleporter())
                    {
                        // Find all other portals
                        std::vector<Obstacle*> otherPortals;
                        for (auto& other : obstacles)
                        {
                            if (other.get() != obstacle.get() && other->getType() == ObstacleType::Portal && other->isAlive())
                                otherPortals.push_back (other.get());
                        }

                        // Teleport to random portal if there are others
                        if (!otherPortals.empty())
                        {
                            Obstacle* destPortal = otherPortals[rand() % otherPortals.size()];
                            tank->teleportTo (destPortal->getPosition());
                        }
                    }
                }
                else
                {
                    // Push tank away from obstacle
                    tank->applyCollision (pushDir, pushDist, { 0, 0 });

                    if (audio)
                        audio->playCollision (tank->getPosition().x, arenaWidth);
                }
            }
        }
    }

    // Tank-to-tank collisions
    for (int i = 0; i < MAX_TANKS; ++i)
    {
        if (!tanks[i] || !tanks[i]->isAlive())
            continue;

        for (int j = i + 1; j < MAX_TANKS; ++j)
        {
            if (!tanks[j] || !tanks[j]->isAlive())
                continue;

            Vec2 collisionPoint;
            if (renderer->checkTankCollision (*tanks[i], *tanks[j], collisionPoint))
            {
                Vec2 diff = tanks[j]->getPosition() - tanks[i]->getPosition();
                Vec2 normal = diff.normalized();

                float pushDist = (tanks[i]->getSize() + tanks[j]->getSize()) * 0.3f;

                Vec2 velA = tanks[i]->getVelocity();
                Vec2 velB = tanks[j]->getVelocity();
                Vec2 relVel = velA - velB;
                float impulseStrength = relVel.dot (normal) * 0.5f * config.collisionRestitution;

                Vec2 impulse = normal * impulseStrength;

                tanks[i]->applyCollision (normal * -1.0f, pushDist, impulse * -1.0f);
                tanks[j]->applyCollision (normal, pushDist, impulse);

                // Collision damage
                float impactSpeed = relVel.length();
                float damage = impactSpeed * config.collisionDamageScale;
                tanks[i]->takeDamage (damage, j);
                tanks[j]->takeDamage (damage, i);

                if (audio && impactSpeed > config.audioMinImpactForSound)
                    audio->playCollision (collisionPoint.x, arenaWidth);
            }
        }
    }
}

void Game::checkRoundOver()
{
    int aliveCount = 0;
    int lastAlive = -1;

    for (int i = 0; i < MAX_TANKS; ++i)
    {
        if (tanks[i] && tanks[i]->isAlive() && !tanks[i]->isDestroying())
        {
            aliveCount++;
            lastAlive = i;
        }
    }

    if (aliveCount <= 1)
    {
        roundWinner = lastAlive;

        // Award survival point
        if (roundWinner >= 0)
            scores[roundWinner] += config.pointsForSurviving;

        stateTimer = 0.0f;
        state = GameState::RoundOver;
    }
}

void Game::updateRoundOver (float dt)
{
    stateTimer += dt;

    float arenaWidth, arenaHeight;
    getWindowSize (arenaWidth, arenaHeight);

    // Keep updating tanks for smoke effects
    for (auto& tank : tanks)
    {
        if (tank && tank->isVisible())
            tank->update (dt, { 0, 0 }, { 0, 0 }, false, arenaWidth, arenaHeight);
    }

    // Update explosions
    for (auto& explosion : explosions)
        explosion.timer += dt;

    explosions.erase (
        std::remove_if (explosions.begin(), explosions.end(), [] (const Explosion& e)
                        { return ! e.isAlive(); }),
        explosions.end());

    if (stateTimer >= config.roundOverDelay)
    {
        if (currentRound >= config.roundsToWin)
        {
            stateTimer = 0.0f;
            state = GameState::GameOver;
        }
        else
        {
            startPlacement();
        }
    }
}

void Game::updateGameOver (float dt)
{
    stateTimer += dt;

    if (stateTimer >= config.gameOverDelay)
    {
        if (anyButtonPressed())
        {
            resetGame();
            state = GameState::Title;
        }
    }
}

void Game::resetGame()
{
    for (auto& tank : tanks)
        tank.reset();

    shells.clear();
    explosions.clear();
    obstacles.clear();

    currentRound = 0;
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        scores[i] = 0;
        kills[i] = 0;
    }
}

void Game::render()
{
    BeginDrawing();

    float w, h;
    getWindowSize (w, h);
    renderer->drawDirt (time, w, h);

    switch (state)
    {
        case GameState::Title:
            renderTitle();
            break;
        case GameState::Placement:
            renderPlacement();
            break;
        case GameState::Playing:
            renderPlaying();
            break;
        case GameState::RoundOver:
            renderPlaying();
            renderRoundOver();
            break;
        case GameState::GameOver:
            renderPlaying();
            renderGameOver();
            break;
    }

    renderer->present();
    EndDrawing();
}

void Game::renderTitle()
{
    float w, h;
    getWindowSize (w, h);

    renderer->drawTextCentered ("CAMBRAI", { w / 2.0f, h / 3.0f }, 8.0f, config.colorTitle);

    int connectedCount = 0;
    for (auto& player : players)
        if (player->isConnected())
            connectedCount++;

    std::string playerText = std::to_string (connectedCount) + " PLAYERS CONNECTED";
    renderer->drawTextCentered (playerText, { w / 2.0f, h * 0.5f }, 3.0f, config.colorSubtitle);

    renderer->drawTextCentered ("FREE FOR ALL - BEST OF 10", { w / 2.0f, h * 0.6f }, 2.5f, config.colorSubtitle);

    // Player slots
    float slotSpacing = 80.0f;
    float startX = w / 2.0f - 1.5f * slotSpacing;
    float slotY = h * 0.72f;

    for (int i = 0; i < 4; ++i)
    {
        Vec2 slotPos = { startX + i * slotSpacing, slotY };
        Color slotColor;

        if (players[i]->isConnected())
        {
            switch (i)
            {
                case 0: slotColor = config.colorTankRed; break;
                case 1: slotColor = config.colorTankBlue; break;
                case 2: slotColor = config.colorTankGreen; break;
                case 3: slotColor = config.colorTankYellow; break;
            }
            renderer->drawFilledRect ({ slotPos.x - 25, slotPos.y - 25 }, 50, 50, slotColor);
            renderer->drawTextCentered ("P" + std::to_string (i + 1), slotPos, 3.0f, config.colorBlack);
        }
        else
        {
            slotColor = config.colorGreyDark;
            renderer->drawRect ({ slotPos.x - 25, slotPos.y - 25 }, 50, 50, slotColor);
            renderer->drawTextCentered ("AI", slotPos, 2.0f, slotColor);
        }
    }

    renderer->drawTextCentered ("CLICK OR PRESS ANY BUTTON TO START", { w / 2.0f, h * 0.9f }, 2.0f, config.colorInstruction);
}

void Game::renderPlacement()
{
    float w, h;
    getWindowSize (w, h);

    // Draw existing obstacles
    for (const auto& obstacle : obstacles)
    {
        obstacle->draw (*renderer);
    }

    // Draw tanks as grey ghosts during placement
    for (const auto& tank : tanks)
        if (tank)
            renderer->drawTankGhost (*tank);

    // Draw placement previews for human players
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        if (hasPlaced[i] || !players[i]->isConnected())
            continue;

        auto preview = createObstacle (assignedObstacles[i], placementPositions[i], placementAngles[i], i);

        std::vector<Tank*> tankPtrs;
        for (auto& tank : tanks)
            if (tank) tankPtrs.push_back (tank.get());

        bool valid = preview->isValidPlacement (obstacles, tankPtrs, w, h);
        preview->drawPreview (*renderer, valid);
    }

    // Draw timer
    int seconds = (int) std::ceil (placementTimer);
    std::string timerText = "PLACE YOUR OBSTACLE: " + std::to_string (seconds);
    renderer->drawTextCentered (timerText, { w / 2.0f, 40.0f }, 3.0f, config.colorPlacementTimer);

    // Draw obstacle type for each player
    float slotY = h - 50.0f;
    float slotSpacing = 200.0f;
    float startX = w / 2.0f - 1.5f * slotSpacing;

    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        Vec2 pos = { startX + i * slotSpacing, slotY };
        Color color = tanks[i] ? tanks[i]->getColor() : config.colorGrey;

        std::string typeText;
        switch (assignedObstacles[i])
        {
            case ObstacleType::SolidWall: typeText = "SOLID WALL"; break;
            case ObstacleType::BreakableWall: typeText = "BREAKABLE WALL"; break;
            case ObstacleType::ReflectiveWall: typeText = "MIRROR WALL"; break;
            case ObstacleType::Mine: typeText = "MINE"; break;
            case ObstacleType::AutoTurret: typeText = "AUTO TURRET"; break;
            case ObstacleType::Pit: typeText = "PIT"; break;
            case ObstacleType::Portal: typeText = "PORTAL"; break;
        }

        std::string statusText = hasPlaced[i] ? "PLACED" : typeText;
        renderer->drawTextCentered ("P" + std::to_string (i + 1), { pos.x, pos.y - 15 }, 2.0f, color);
        renderer->drawTextCentered (statusText, { pos.x, pos.y + 10 }, 1.5f, hasPlaced[i] ? config.colorGreySubtle : color);
    }
}

void Game::renderPlaying()
{
    float w, h;
    getWindowSize (w, h);

    // Draw track marks first
    for (const auto& tank : tanks)
        if (tank && tank->isVisible())
            renderer->drawTrackMarks (*tank);

    // Draw obstacles
    for (const auto& obstacle : obstacles)
        obstacle->draw (*renderer);

    // Draw tanks
    for (const auto& tank : tanks)
        if (tank && tank->isVisible())
            renderer->drawTank (*tank);

    // Draw smoke
    for (const auto& tank : tanks)
        if (tank && tank->isVisible())
            renderer->drawSmoke (*tank);

    // Draw shells
    for (const auto& shell : shells)
        renderer->drawShell (shell);

    // Draw explosions
    for (const auto& explosion : explosions)
        renderer->drawExplosion (explosion);

    // Draw crosshairs
    for (const auto& tank : tanks)
        if (tank && tank->isAlive())
            renderer->drawCrosshair (*tank);

    // Draw HUDs
    float hudWidth = 150.0f;
    for (int i = 0; i < MAX_TANKS; ++i)
    {
        if (tanks[i])
        {
            float alpha = tanks[i]->isAlive() ? 1.0f : 0.4f;
            renderer->drawTankHUD (*tanks[i], i, MAX_TANKS, w, hudWidth, alpha);
        }
    }

    // Draw round counter
    std::string roundText = "ROUND " + std::to_string (currentRound) + " OF " + std::to_string (config.roundsToWin);
    renderer->drawTextCentered (roundText, { w / 2.0f, h - 20.0f }, 1.5f, config.colorGreySubtle);

    // Draw scores on bottom
    float scoreY = h - 50.0f;
    float scoreSpacing = 100.0f;
    float scoreStartX = w / 2.0f - 1.5f * scoreSpacing;

    for (int i = 0; i < MAX_TANKS; ++i)
    {
        Vec2 pos = { scoreStartX + i * scoreSpacing, scoreY };
        Color color = tanks[i] ? tanks[i]->getColor() : config.colorGrey;

        std::string scoreStr = std::to_string (scores[i]);
        renderer->drawTextCentered (scoreStr, pos, 3.0f, color);
    }
}

void Game::renderRoundOver()
{
    float w, h;
    getWindowSize (w, h);

    if (roundWinner >= 0)
    {
        std::string winText = "PLAYER " + std::to_string (roundWinner + 1) + " WINS ROUND " + std::to_string (currentRound);
        renderer->drawTextCentered (winText, { w / 2.0f, h / 2.0f }, 4.0f, config.colorTitle);
    }
    else
    {
        renderer->drawTextCentered ("DRAW!", { w / 2.0f, h / 2.0f }, 4.0f, config.colorTitle);
    }
}

void Game::renderGameOver()
{
    float w, h;
    getWindowSize (w, h);

    // Find winner (highest score)
    int winner = 0;
    for (int i = 1; i < MAX_PLAYERS; ++i)
    {
        if (scores[i] > scores[winner])
            winner = i;
    }

    std::string winText = "PLAYER " + std::to_string (winner + 1) + " WINS!";
    renderer->drawTextCentered (winText, { w / 2.0f, h / 2.0f - 40.0f }, 5.0f, config.colorTitle);

    // Final scores
    std::string scoresText = "";
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        scoresText += "P" + std::to_string (i + 1) + ": " + std::to_string (scores[i]) + "  ";
    }
    renderer->drawTextCentered (scoresText, { w / 2.0f, h / 2.0f + 40.0f }, 2.5f, config.colorSubtitle);

    if (stateTimer >= config.gameOverDelay)
    {
        renderer->drawTextCentered ("PRESS ANY BUTTON TO CONTINUE", { w / 2.0f, h * 0.8f }, 2.0f, config.colorInstruction);
    }
}

Vec2 Game::getTankStartPosition (int index) const
{
    float w, h;
    getWindowSize (w, h);

    // Place tanks in corners
    float margin = 100.0f;

    switch (index)
    {
        case 0: return { margin, margin };
        case 1: return { w - margin, margin };
        case 2: return { margin, h - margin };
        case 3: return { w - margin, h - margin };
        default: return { w / 2.0f, h / 2.0f };
    }
}

float Game::getTankStartAngle (int index) const
{
    // Point tanks toward center
    switch (index)
    {
        case 0: return pi * 0.25f;   // Top-left, point toward center
        case 1: return pi * 0.75f;   // Top-right
        case 2: return -pi * 0.25f;  // Bottom-left
        case 3: return -pi * 0.75f;  // Bottom-right
        default: return 0.0f;
    }
}

void Game::getWindowSize (float& width, float& height) const
{
    width = (float) GetScreenWidth();
    height = (float) GetScreenHeight();
}

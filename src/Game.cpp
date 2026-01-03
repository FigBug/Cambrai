#include "Game.h"
#include "Random.h"
#include <raylib.h>
#include <algorithm>
#include <cmath>

Game::Game() = default;

Game::~Game() = default;

bool Game::init()
{
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
        case GameState::Selection:
            updateSelection (dt);
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
        startSelection();
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

// =============================================================================
// Selection Phase
// =============================================================================

ObstacleType Game::indexToObstacleType (int index) const
{
    switch (index)
    {
        case 0: return ObstacleType::SolidWall;
        case 1: return ObstacleType::BreakableWall;
        case 2: return ObstacleType::ReflectiveWall;
        case 3: return ObstacleType::Mine;
        case 4: return ObstacleType::AutoTurret;
        case 5: return ObstacleType::Pit;
        case 6: return ObstacleType::Portal;
        case 7: return ObstacleType::Flag;
        case 8: return ObstacleType::HealthPack;
        case 9: return ObstacleType::Electromagnet;
        case 10: return ObstacleType::Fan;
        case 11: return ObstacleType::RicochetWall;
        default: return ObstacleType::SolidWall;
    }
}

std::string Game::obstacleTypeName (ObstacleType type) const
{
    switch (type)
    {
        case ObstacleType::SolidWall: return "SOLID WALL";
        case ObstacleType::BreakableWall: return "BREAKABLE";
        case ObstacleType::ReflectiveWall: return "MIRROR";
        case ObstacleType::Mine: return "MINE";
        case ObstacleType::AutoTurret: return "TURRET";
        case ObstacleType::Pit: return "PIT";
        case ObstacleType::Portal: return "PORTAL";
        case ObstacleType::Flag: return "FLAG";
        case ObstacleType::HealthPack: return "HEALTH";
        case ObstacleType::Electromagnet: return "MAGNET";
        case ObstacleType::Fan: return "FAN";
        case ObstacleType::RicochetWall: return "RICOCHET";
        default: return "UNKNOWN";
    }
}

bool Game::isObstacleSelectedByOther (int obstacleIndex, int playerIndex) const
{
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        if (i != playerIndex && hasSelected[i] && selectedObstacleIndex[i] == obstacleIndex)
            return true;
    }
    return false;
}

int Game::findAvailableObstacle (int startIndex, int playerIndex) const
{
    // Find the first available obstacle starting from startIndex
    for (int offset = 0; offset < 12; ++offset)
    {
        int idx = (startIndex + offset) % 12;
        if (!isObstacleSelectedByOther (idx, playerIndex))
            return idx;
    }
    return startIndex;  // Shouldn't happen with 4 players and 12 obstacles
}

void Game::startSelection()
{
    currentRound++;
    selectionTimer = config.selectionTime;

    // Initialize cursor positions spread across the grid
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        hasSelected[i] = false;
        selectedObstacleIndex[i] = -1;
        selectionCursorIndex[i] = findAvailableObstacle (i * 3, i);  // Spread initial positions

        // AI selection timing
        aiSelectionMoveTimer[i] = config.aiSelectionMoveInterval;
        aiSelectionConfirmTimer[i] = config.aiSelectionMinDelay +
            randomFloat() * (config.aiSelectionMaxDelay - config.aiSelectionMinDelay);
    }

    state = GameState::Selection;
}

void Game::updateSelection (float dt)
{
    selectionTimer -= dt;

    // Update each player
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        if (hasSelected[i])
            continue;

        bool isHuman = players[i]->isConnected();

        if (isHuman)
        {
            // Human player navigation
            int navX = players[i]->getNavigationX();
            int navY = players[i]->getNavigationY();

            if (navX != 0 || navY != 0)
            {
                int currentIndex = selectionCursorIndex[i];
                int startIndex = currentIndex;

                // Keep moving until we find an available cell (or wrap back to start)
                do
                {
                    // Move to next cell with wrapping
                    if (navX != 0)
                    {
                        // Horizontal movement with row wrapping
                        currentIndex += navX;

                        // Wrap around horizontally and to adjacent rows
                        if (currentIndex < 0)
                            currentIndex = 11;  // Wrap to last cell
                        else if (currentIndex > 11)
                            currentIndex = 0;   // Wrap to first cell
                    }
                    else if (navY != 0)
                    {
                        // Vertical movement
                        int col = currentIndex % 4;
                        int row = currentIndex / 4;

                        row += navY;

                        // Wrap around vertically
                        if (row < 0)
                            row = 2;
                        else if (row > 2)
                            row = 0;

                        currentIndex = row * 4 + col;
                    }

                    // If this cell is available, use it
                    if (!isObstacleSelectedByOther (currentIndex, i))
                    {
                        selectionCursorIndex[i] = currentIndex;
                        break;
                    }

                    // Safety: if we've looped all the way around, stop
                    if (currentIndex == startIndex)
                        break;

                } while (true);
            }

            // Confirm selection
            if (players[i]->getConfirmInput())
            {
                int idx = selectionCursorIndex[i];
                if (!isObstacleSelectedByOther (idx, i))
                {
                    hasSelected[i] = true;
                    selectedObstacleIndex[i] = idx;
                }
            }
        }
        else
        {
            // AI player - simulated browsing
            aiSelectionMoveTimer[i] -= dt;
            aiSelectionConfirmTimer[i] -= dt;

            // Occasionally move cursor
            if (aiSelectionMoveTimer[i] <= 0)
            {
                aiSelectionMoveTimer[i] = config.aiSelectionMoveInterval;

                // Random move direction
                int dir = randomInt (4);  // 0=left, 1=right, 2=up, 3=down
                int col = selectionCursorIndex[i] % 4;
                int row = selectionCursorIndex[i] / 4;

                switch (dir)
                {
                    case 0: col = std::max (0, col - 1); break;
                    case 1: col = std::min (3, col + 1); break;
                    case 2: row = std::max (0, row - 1); break;
                    case 3: row = std::min (2, row + 1); break;
                }

                int newIndex = row * 4 + col;

                if (!isObstacleSelectedByOther (newIndex, i))
                    selectionCursorIndex[i] = newIndex;
            }

            // Confirm after random delay
            if (aiSelectionConfirmTimer[i] <= 0)
            {
                int idx = selectionCursorIndex[i];
                if (!isObstacleSelectedByOther (idx, i))
                {
                    hasSelected[i] = true;
                    selectedObstacleIndex[i] = idx;
                }
                else
                {
                    // Find available and select immediately
                    idx = findAvailableObstacle (idx, i);
                    hasSelected[i] = true;
                    selectedObstacleIndex[i] = idx;
                }
            }
        }
    }

    // Check if selection phase is done
    bool allSelected = true;
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        if (!hasSelected[i])
            allSelected = false;
    }

    if (allSelected || selectionTimer <= 0)
    {
        // Auto-confirm anyone who hasn't selected
        for (int i = 0; i < MAX_PLAYERS; ++i)
        {
            if (!hasSelected[i])
            {
                int idx = selectionCursorIndex[i];
                if (isObstacleSelectedByOther (idx, i))
                    idx = findAvailableObstacle (idx, i);

                hasSelected[i] = true;
                selectedObstacleIndex[i] = idx;
            }
        }

        startPlacement();
    }
}

void Game::renderSelection()
{
    float w, h;
    getWindowSize (w, h);

    // Draw timer
    int seconds = (int) std::ceil (selectionTimer);
    std::string timerText = "SELECT YOUR OBSTACLE: " + std::to_string (seconds);
    renderer->drawTextCentered (timerText, { w / 2.0f, 40.0f }, 3.0f, config.colorPlacementTimer);

    // Grid layout: 4 columns x 3 rows
    const int cols = 4;
    const int rows = 3;
    const float cellWidth = 150.0f;
    const float cellHeight = 100.0f;
    const float cellSpacing = 10.0f;

    float gridWidth = cols * cellWidth + (cols - 1) * cellSpacing;
    float gridHeight = rows * cellHeight + (rows - 1) * cellSpacing;
    float gridStartX = (w - gridWidth) / 2.0f;
    float gridStartY = (h - gridHeight) / 2.0f - 20.0f;

    // Draw grid cells
    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            int idx = row * cols + col;

            float cellX = gridStartX + col * (cellWidth + cellSpacing);
            float cellY = gridStartY + row * (cellHeight + cellSpacing);

            // Determine cell state
            bool isTaken = false;
            int takenByPlayer = -1;
            for (int p = 0; p < MAX_PLAYERS; ++p)
            {
                if (hasSelected[p] && selectedObstacleIndex[p] == idx)
                {
                    isTaken = true;
                    takenByPlayer = p;
                    break;
                }
            }

            // Draw cell background
            Color cellColor = isTaken ? config.colorSelectionTaken : config.colorSelectionCell;
            renderer->drawFilledRect ({ cellX, cellY }, cellWidth, cellHeight, cellColor);

            // Draw obstacle preview (clipped to cell)
            Vec2 previewPos = { cellX + cellWidth / 2.0f, cellY + cellHeight / 2.0f - 10.0f };
            ObstacleType obstacleType = indexToObstacleType (idx);

            // Create temporary obstacle for drawing with clipping
            BeginScissorMode ((int) cellX, (int) cellY, (int) cellWidth, (int) cellHeight);
            auto preview = createObstacle (obstacleType, previewPos, 0.0f, -1);
            preview->draw (*renderer);
            EndScissorMode();

            // Draw obstacle name
            std::string name = obstacleTypeName (obstacleType);
            renderer->drawTextCentered (name, { cellX + cellWidth / 2.0f, cellY + cellHeight - 15.0f },
                                       1.5f, config.colorSelectionText);

            // If taken, show which player
            if (isTaken && takenByPlayer >= 0)
            {
                Color playerColor;
                switch (takenByPlayer)
                {
                    case 0: playerColor = config.colorTankRed; break;
                    case 1: playerColor = config.colorTankBlue; break;
                    case 2: playerColor = config.colorTankGreen; break;
                    default: playerColor = config.colorTankYellow; break;
                }
                std::string playerLabel = "P" + std::to_string (takenByPlayer + 1);
                renderer->drawTextCentered (playerLabel, { cellX + cellWidth / 2.0f, cellY + 15.0f },
                                           2.0f, playerColor);
            }

            // Draw player cursor outlines (for both selecting and selected players)
            for (int p = 0; p < MAX_PLAYERS; ++p)
            {
                // Show outline if player is hovering here OR has selected this cell
                bool showOutline = false;
                if (!hasSelected[p] && selectionCursorIndex[p] == idx)
                    showOutline = true;
                if (hasSelected[p] && selectedObstacleIndex[p] == idx)
                    showOutline = true;

                if (showOutline)
                {
                    Color cursorColor;
                    switch (p)
                    {
                        case 0: cursorColor = config.colorTankRed; break;
                        case 1: cursorColor = config.colorTankBlue; break;
                        case 2: cursorColor = config.colorTankGreen; break;
                        default: cursorColor = config.colorTankYellow; break;
                    }

                    // Draw thick outline (multiple rectangles for thickness)
                    float thickness = 6.0f;
                    for (float t = 0; t < thickness; t += 1.0f)
                    {
                        renderer->drawRect ({ cellX - thickness + t, cellY - thickness + t },
                                           cellWidth + (thickness - t) * 2, cellHeight + (thickness - t) * 2, cursorColor);
                    }
                }
            }
        }
    }

    // Draw player status at bottom
    float slotY = h - 60.0f;
    float slotSpacing = 200.0f;
    float startX = w / 2.0f - 1.5f * slotSpacing;

    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        Vec2 pos = { startX + i * slotSpacing, slotY };
        Color color;
        switch (i)
        {
            case 0: color = config.colorTankRed; break;
            case 1: color = config.colorTankBlue; break;
            case 2: color = config.colorTankGreen; break;
            default: color = config.colorTankYellow; break;
        }

        std::string statusText;
        if (hasSelected[i])
            statusText = obstacleTypeName (indexToObstacleType (selectedObstacleIndex[i]));
        else
            statusText = "SELECTING...";

        renderer->drawTextCentered ("P" + std::to_string (i + 1), { pos.x, pos.y - 15 }, 2.0f, color);
        renderer->drawTextCentered (statusText, { pos.x, pos.y + 10 }, 1.5f, hasSelected[i] ? color : config.colorGreySubtle);
    }

    // Draw instructions
    renderer->drawTextCentered ("ARROWS TO MOVE - ENTER TO SELECT", { w / 2.0f, h - 20.0f }, 1.5f, config.colorInstruction);
}

void Game::startPlacement()
{
    // Shuffle starting positions
    for (int i = MAX_TANKS - 1; i > 0; --i)
    {
        int j = randomInt (i + 1);
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

    // Use selected obstacles from selection phase
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        assignedObstacles[i] = indexToObstacleType (selectedObstacleIndex[i]);
    }

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

    // Auto-place for players who haven't placed when timer expires
    if (placementTimer <= 0 && !allPlaced)
    {
        std::vector<Tank*> tankPtrs;
        for (auto& tank : tanks)
            if (tank) tankPtrs.push_back (tank.get());

        for (int i = 0; i < MAX_PLAYERS; ++i)
        {
            if (hasPlaced[i])
                continue;

            // Try current position first
            auto temp = createObstacle (assignedObstacles[i], placementPositions[i], placementAngles[i], i);
            if (temp->isValidPlacement (obstacles, tankPtrs, w, h))
            {
                obstacles.push_back (createObstacle (assignedObstacles[i], placementPositions[i], placementAngles[i], i));
                hasPlaced[i] = true;
            }
            else
            {
                // Try random valid positions
                for (int attempt = 0; attempt < 20; ++attempt)
                {
                    Vec2 pos = aiControllers[i]->getPlacementPosition (w, h);
                    float angle = aiControllers[i]->getPlacementAngle();

                    auto randTemp = createObstacle (assignedObstacles[i], pos, angle, i);
                    if (randTemp->isValidPlacement (obstacles, tankPtrs, w, h))
                    {
                        obstacles.push_back (createObstacle (assignedObstacles[i], pos, angle, i));
                        hasPlaced[i] = true;
                        break;
                    }
                }

                // If still couldn't place, mark as placed anyway (no obstacle placed)
                if (!hasPlaced[i])
                    hasPlaced[i] = true;
            }
        }
        allPlaced = true;
    }

    if (allPlaced)
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

    // Reset stalemate detection
    noDamageTimer = 0.0f;
    for (int i = 0; i < MAX_TANKS; ++i)
        lastTankHealth[i] = tanks[i] ? tanks[i]->getHealth() : 0.0f;

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

        // Apply obstacle forces to tanks (electromagnet, fan)
        if (obstacle->isAlive())
        {
            for (auto& tank : tanks)
            {
                if (tank && tank->isAlive())
                {
                    Vec2 force = obstacle->getTankForce (*tank);
                    tank->applyExternalForce (force);
                }
            }
        }

        // Handle collection effects (flag capture, health pack pickup)
        auto effect = obstacle->consumeCollectionEffect();
        if (effect.playerIndex >= 0 && effect.playerIndex < MAX_PLAYERS)
        {
            scores[effect.playerIndex] += effect.scoreToAdd;
            if (effect.healthPercent > 0 && tanks[effect.playerIndex])
                tanks[effect.playerIndex]->heal (effect.healthPercent);
        }
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

    // Update stalemate timer
    noDamageTimer += dt;

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

        // Apply forces from obstacles (fans, electromagnets)
        for (auto& obstacle : obstacles)
        {
            if (!obstacle->isAlive())
                continue;

            Vec2 force = obstacle->getShellForce (shell.getPosition());
            shell.applyForce (force, dt);
        }

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
                    break;  // Only one reflection per frame
                }
                else if (result == ShellHitResult::Ricochet)
                {
                    // Create 5 shells with spread angles
                    Vec2 shellVel = shell.getVelocity();
                    float speed = shellVel.length();

                    // Reflect base velocity
                    float dot = shellVel.dot (normal);
                    Vec2 reflectedVel = shellVel - normal * (2.0f * dot);
                    float baseAngle = std::atan2 (reflectedVel.y, reflectedVel.x);

                    // Spawn 5 shells with angles spread around the reflected direction
                    float spreadAngles[5] = { -0.3f, -0.15f, 0.0f, 0.15f, 0.3f };
                    for (int i = 0; i < 5; ++i)
                    {
                        float angle = baseAngle + spreadAngles[i];
                        Vec2 newVel = { std::cos (angle) * speed, std::sin (angle) * speed };
                        Vec2 spawnPos = collisionPoint + normal * 5.0f;
                        shells.push_back (Shell (spawnPos, newVel, shell.getOwnerIndex(),
                                                 shell.getMaxRange() * 0.5f, shell.getDamage() * 0.4f));
                    }

                    shell.kill();
                    break;
                }
                else  // Destroyed
                {
                    // Damage destructible obstacles (breakable walls, turrets)
                    obstacle->takeDamage (shell.getDamage());

                    if (obstacle->createsExplosionOnHit())
                    {
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

                // Track kill (no points for self-kills)
                if (!tank->isAlive() && shell.getOwnerIndex() >= 0 && shell.getOwnerIndex() < MAX_PLAYERS
                    && tank->getPlayerIndex() != shell.getOwnerIndex())
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

                    // Track kill (no points for self-kills)
                    if (!tank->isAlive() && obstacle->getOwnerIndex() >= 0 && obstacle->getOwnerIndex() < MAX_PLAYERS
                        && tank->getPlayerIndex() != obstacle->getOwnerIndex())
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
                else
                {
                    // Let obstacle handle collision (pit traps, portal teleports, etc.)
                    bool applyPush = obstacle->handleTankCollision (*tank, obstacles);

                    if (applyPush)
                    {
                        tank->applyCollision (pushDir, pushDist, { 0, 0 });

                        if (audio)
                            audio->playCollision (tank->getPosition().x, arenaWidth);
                    }
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

                // Track kills from ramming
                if (!tanks[i]->isAlive())
                {
                    kills[j]++;
                    scores[j] += config.pointsForKill;

                    Explosion destroyExplosion;
                    destroyExplosion.position = tanks[i]->getPosition();
                    destroyExplosion.duration = config.destroyExplosionDuration;
                    destroyExplosion.maxRadius = config.destroyExplosionMaxRadius;
                    explosions.push_back (destroyExplosion);
                }
                if (!tanks[j]->isAlive())
                {
                    kills[i]++;
                    scores[i] += config.pointsForKill;

                    Explosion destroyExplosion;
                    destroyExplosion.position = tanks[j]->getPosition();
                    destroyExplosion.duration = config.destroyExplosionDuration;
                    destroyExplosion.maxRadius = config.destroyExplosionMaxRadius;
                    explosions.push_back (destroyExplosion);
                }

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

    // Check for damage taken (stalemate detection)
    bool damageTaken = false;
    for (int i = 0; i < MAX_TANKS; ++i)
    {
        if (tanks[i] && tanks[i]->isAlive() && !tanks[i]->isDestroying())
        {
            aliveCount++;
            lastAlive = i;

            float currentHealth = tanks[i]->getHealth();
            if (currentHealth < lastTankHealth[i])
                damageTaken = true;
            lastTankHealth[i] = currentHealth;
        }
    }

    if (damageTaken)
        noDamageTimer = 0.0f;

    if (aliveCount <= 1)
    {
        roundWinner = lastAlive;

        // Award survival point
        if (roundWinner >= 0)
            scores[roundWinner] += config.pointsForSurviving;

        stateTimer = 0.0f;
        state = GameState::RoundOver;
    }
    // Stalemate - no damage for too long
    else if (aliveCount > 1 && noDamageTimer >= config.stalemateTimeout)
    {
        roundWinner = -1;  // Draw
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
            startSelection();
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
        case GameState::Selection:
            renderSelection();
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
            case ObstacleType::Flag: typeText = "FLAG"; break;
            case ObstacleType::HealthPack: typeText = "HEALTH PACK"; break;
            case ObstacleType::Electromagnet: typeText = "ELECTROMAGNET"; break;
            case ObstacleType::Fan: typeText = "FAN"; break;
            case ObstacleType::RicochetWall: typeText = "RICOCHET WALL"; break;
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

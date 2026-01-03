#pragma once

#include "AIController.h"
#include "Audio.h"
#include "Config.h"
#include "Obstacles/AllObstacles.h"
#include "Player.h"
#include "Renderer.h"
#include "Shell.h"
#include "Tank.h"
#include <array>
#include <memory>
#include <vector>

enum class GameState
{
    Title,
    Selection,    // Players select their obstacles from grid
    Placement,    // Players place their obstacles
    Playing,
    RoundOver,
    GameOver      // After 10 rounds
};

struct Explosion
{
    Vec2 position;
    float timer = 0.0f;
    float duration = 0.0f;
    float maxRadius = 0.0f;

    float getProgress() const { return timer / duration; }
    bool isAlive() const { return timer < duration; }
};

class Game
{
public:
    Game();
    ~Game();

    bool init();
    void run();
    void shutdown();

private:
    static constexpr int WINDOW_WIDTH = 1280;
    static constexpr int WINDOW_HEIGHT = 720;
    static constexpr int MAX_TANKS = 4;
    static constexpr int MAX_PLAYERS = 4;

    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<Audio> audio;

    bool running = false;
    GameState state = GameState::Title;
    int currentRound = 0;
    float stateTimer = 0.0f;
    float time = 0.0f;
    double lastFrameTime = 0.0;

    std::array<std::unique_ptr<Tank>, MAX_TANKS> tanks;
    std::array<std::unique_ptr<Player>, MAX_PLAYERS> players;
    std::array<std::unique_ptr<AIController>, MAX_TANKS> aiControllers;
    std::vector<Shell> shells;
    std::vector<Explosion> explosions;
    std::vector<std::unique_ptr<Obstacle>> obstacles;

    // Selection phase
    std::array<int, MAX_PLAYERS> selectionCursorIndex = {};   // Grid position (0-10)
    std::array<bool, MAX_PLAYERS> hasSelected = {};
    std::array<int, MAX_PLAYERS> selectedObstacleIndex = {};  // Which obstacle type selected
    float selectionTimer = 0.0f;

    // AI selection behavior
    std::array<float, MAX_PLAYERS> aiSelectionMoveTimer = {};
    std::array<float, MAX_PLAYERS> aiSelectionConfirmTimer = {};

    // Placement phase
    std::array<ObstacleType, MAX_PLAYERS> assignedObstacles;
    std::array<bool, MAX_PLAYERS> hasPlaced;
    std::array<Vec2, MAX_PLAYERS> placementPositions;
    std::array<float, MAX_PLAYERS> placementAngles;
    float placementTimer = 0.0f;

    // Scoring
    std::array<int, MAX_PLAYERS> scores = {};  // Total points
    std::array<int, MAX_PLAYERS> kills = {};   // Kills this round
    int roundWinner = -1;

    // Stalemate detection
    float noDamageTimer = 0.0f;
    std::array<float, MAX_TANKS> lastTankHealth = {};

    // Random starting positions (shuffled each round)
    std::array<int, MAX_TANKS> startPositionOrder = { 0, 1, 2, 3 };

    void handleEvents();
    void update (float dt);
    void render();

    // Title screen
    void updateTitle (float dt);
    void renderTitle();
    bool anyButtonPressed();

    // Selection phase
    void startSelection();
    void updateSelection (float dt);
    void renderSelection();
    ObstacleType indexToObstacleType (int index) const;
    std::string obstacleTypeName (ObstacleType type) const;
    bool isObstacleSelectedByOther (int obstacleIndex, int playerIndex) const;
    int findAvailableObstacle (int startIndex, int playerIndex) const;

    // Placement phase
    void startPlacement();
    void updatePlacement (float dt);
    void renderPlacement();

    // Gameplay
    void startRound();
    void updatePlaying (float dt);
    void renderPlaying();
    void updateShells (float dt);
    void checkCollisions();
    void checkRoundOver();

    // Round over
    void updateRoundOver (float dt);
    void renderRoundOver();

    // Game over (after 10 rounds)
    void updateGameOver (float dt);
    void renderGameOver();
    void resetGame();

    Vec2 getTankStartPosition (int index) const;
    float getTankStartAngle (int index) const;
    void getWindowSize (float& width, float& height) const;
    int getNumTanks() const { return MAX_TANKS; }
};

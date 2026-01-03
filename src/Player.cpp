#include "Player.h"
#include <raylib.h>
#include <algorithm>
#include <cmath>

Player::Player (int playerIndex_)
    : playerIndex (playerIndex_)
{
    tryOpenGamepad();
}

Player::~Player()
{
}

void Player::update()
{
    // Check if gamepad is still available, or try to find one
    if (gamepadId < 0 || !IsGamepadAvailable (gamepadId))
    {
        tryOpenGamepad();
    }

    // Player 0 can use keyboard/mouse if no gamepad
    if (gamepadId < 0 && playerIndex == 0)
    {
        usingKeyboard = true;
        updateKeyboardMouse();
        return;
    }

    usingKeyboard = false;

    if (gamepadId < 0)
    {
        moveInput = { 0, 0 };
        aimInput = { 0, 0 };
        fireInput = false;
        placeInput = false;
        rotateInput = false;
        navX = 0;
        navY = 0;
        confirmInput = false;
        return;
    }

    // Left stick for movement
    // Y-axis: Forward/Reverse
    // X-axis: Rotate tank
    float leftY = GetGamepadAxisMovement (gamepadId, GAMEPAD_AXIS_LEFT_Y);
    float leftX = GetGamepadAxisMovement (gamepadId, GAMEPAD_AXIS_LEFT_X);

    moveInput.y = applyDeadzone (leftY);
    moveInput.x = applyDeadzone (leftX);

    // Right stick for aiming crosshair
    float rightX = GetGamepadAxisMovement (gamepadId, GAMEPAD_AXIS_RIGHT_X);
    float rightY = GetGamepadAxisMovement (gamepadId, GAMEPAD_AXIS_RIGHT_Y);
    aimInput.x = applyDeadzone (rightX);
    aimInput.y = applyDeadzone (rightY);

    // Any face button or trigger fires
    fireInput = IsGamepadButtonDown (gamepadId, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) ||  // A / Cross
                IsGamepadButtonDown (gamepadId, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) || // B / Circle
                IsGamepadButtonDown (gamepadId, GAMEPAD_BUTTON_RIGHT_FACE_LEFT) ||  // X / Square
                IsGamepadButtonDown (gamepadId, GAMEPAD_BUTTON_RIGHT_FACE_UP) ||    // Y / Triangle
                IsGamepadButtonDown (gamepadId, GAMEPAD_BUTTON_LEFT_TRIGGER_1) ||
                IsGamepadButtonDown (gamepadId, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);

    // A button places obstacle during placement phase
    placeInput = IsGamepadButtonPressed (gamepadId, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);

    // Bumpers rotate obstacle during placement
    rotateInput = IsGamepadButtonDown (gamepadId, GAMEPAD_BUTTON_LEFT_TRIGGER_1) ||
                  IsGamepadButtonDown (gamepadId, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);

    // D-pad navigation for selection grid
    navX = 0;
    navY = 0;
    if (IsGamepadButtonPressed (gamepadId, GAMEPAD_BUTTON_LEFT_FACE_LEFT))
        navX = -1;
    if (IsGamepadButtonPressed (gamepadId, GAMEPAD_BUTTON_LEFT_FACE_RIGHT))
        navX = 1;
    if (IsGamepadButtonPressed (gamepadId, GAMEPAD_BUTTON_LEFT_FACE_UP))
        navY = -1;
    if (IsGamepadButtonPressed (gamepadId, GAMEPAD_BUTTON_LEFT_FACE_DOWN))
        navY = 1;

    confirmInput = IsGamepadButtonPressed (gamepadId, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
}

void Player::updateKeyboardMouse()
{
    // WASD or Arrow keys for movement
    moveInput = { 0, 0 };

    if (IsKeyDown (KEY_W) || IsKeyDown (KEY_UP))
        moveInput.y = -1.0f;  // Forward (negative Y)
    else if (IsKeyDown (KEY_S) || IsKeyDown (KEY_DOWN))
        moveInput.y = 1.0f;   // Reverse

    if (IsKeyDown (KEY_A) || IsKeyDown (KEY_LEFT))
        moveInput.x = -1.0f;  // Rotate left
    else if (IsKeyDown (KEY_D) || IsKeyDown (KEY_RIGHT))
        moveInput.x = 1.0f;   // Rotate right

    // Mouse position for aiming
    Vector2 mouse = GetMousePosition();
    mousePosition = { mouse.x, mouse.y };

    // aimInput not used for mouse aiming - Game will set crosshair directly
    aimInput = { 0, 0 };

    // Left click to fire
    fireInput = IsMouseButtonDown (MOUSE_BUTTON_LEFT);

    // Left click or Enter to place obstacle
    placeInput = IsMouseButtonPressed (MOUSE_BUTTON_LEFT) || IsKeyPressed (KEY_ENTER);

    // Arrow keys, Q/E to rotate obstacle during placement
    rotateInput = IsKeyDown (KEY_Q) || IsKeyDown (KEY_E);

    // Arrow keys / WASD for grid navigation in selection phase
    navX = 0;
    navY = 0;
    if (IsKeyPressed (KEY_LEFT) || IsKeyPressed (KEY_A))
        navX = -1;
    if (IsKeyPressed (KEY_RIGHT) || IsKeyPressed (KEY_D))
        navX = 1;
    if (IsKeyPressed (KEY_UP) || IsKeyPressed (KEY_W))
        navY = -1;
    if (IsKeyPressed (KEY_DOWN) || IsKeyPressed (KEY_S))
        navY = 1;

    confirmInput = IsKeyPressed (KEY_ENTER) || IsKeyPressed (KEY_SPACE);
}

float Player::applyDeadzone (float value) const
{
    if (std::abs (value) < deadzone)
    {
        return 0.0f;
    }
    // Rescale from deadzone to 1.0
    float sign = value > 0 ? 1.0f : -1.0f;
    return sign * (std::abs (value) - deadzone) / (1.0f - deadzone);
}

void Player::tryOpenGamepad()
{
    // Find an available gamepad for this player index
    int gamepadCount = 0;
    for (int i = 0; i < 4; ++i)
    {
        if (IsGamepadAvailable (i))
        {
            if (gamepadCount == playerIndex)
            {
                gamepadId = i;
                TraceLog (LOG_INFO, "Player %d connected to gamepad %d: %s",
                          playerIndex, i, GetGamepadName (i));
                return;
            }
            gamepadCount++;
        }
    }

    // No gamepad found for this player
    gamepadId = -1;
}

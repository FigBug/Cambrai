#pragma once

#include "Vec2.h"

class Player
{
public:
    Player (int playerIndex);
    ~Player();

    void update();

    Vec2 getMoveInput() const { return moveInput; }
    Vec2 getAimInput() const { return aimInput; }
    bool getFireInput() const { return fireInput; }
    bool getPlaceInput() const { return placeInput; }      // For placement phase
    bool getRotateInput() const { return rotateInput; }    // For rotating obstacle during placement
    bool isConnected() const { return gamepadId >= 0 || usingKeyboard; }
    bool isUsingMouse() const { return usingKeyboard; }
    Vec2 getMousePosition() const { return mousePosition; }
    int getPlayerIndex() const { return playerIndex; }

    // Grid navigation (selection phase)
    int getNavigationX() const { return navX; }  // -1, 0, +1
    int getNavigationY() const { return navY; }  // -1, 0, +1
    bool getConfirmInput() const { return confirmInput; }

private:
    int playerIndex;
    int gamepadId = -1;
    bool usingKeyboard = false;

    Vec2 moveInput;
    Vec2 aimInput;
    Vec2 mousePosition;
    bool fireInput = false;
    bool placeInput = false;
    bool rotateInput = false;

    // Grid navigation
    int navX = 0;
    int navY = 0;
    bool confirmInput = false;

    float deadzone = 0.15f;

    float applyDeadzone (float value) const;
    void tryOpenGamepad();
    void updateKeyboardMouse();
};

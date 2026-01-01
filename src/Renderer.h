#pragma once

#include "Vec2.h"
#include <raylib.h>
#include <string>

class Tank;
class Shell;
class Obstacle;
struct Explosion;

class Renderer
{
public:
    Renderer();
    ~Renderer();

    void clear();
    void drawDirt (float time, float screenWidth, float screenHeight);
    void present();

    void drawTank (const Tank& tank);
    void drawTrackMarks (const Tank& tank);
    void drawSmoke (const Tank& tank);
    void drawShell (const Shell& shell);
    void drawExplosion (const Explosion& explosion);
    void drawCrosshair (const Tank& tank);
    void drawObstacle (const Obstacle& obstacle);
    void drawObstaclePreview (const Obstacle& obstacle, bool valid);
    void drawTankHUD (const Tank& tank, int slot, int totalSlots, float screenWidth, float hudWidth, float alpha = 1.0f);

    void drawOval (Vec2 center, float width, float height, float angle, Color color);
    void drawCircle (Vec2 center, float radius, Color color);
    void drawFilledCircle (Vec2 center, float radius, Color color);
    void drawLine (Vec2 start, Vec2 end, Color color);
    void drawLineThick (Vec2 start, Vec2 end, float thickness, Color color);
    void drawRect (Vec2 topLeft, float width, float height, Color color);
    void drawFilledRect (Vec2 topLeft, float width, float height, Color color);
    void drawRotatedRect (Vec2 center, float width, float height, float angle, Color color);
    void drawFilledRotatedRect (Vec2 center, float width, float height, float angle, Color color);

    void drawText (const std::string& text, Vec2 position, float scale, Color color);
    void drawTextCentered (const std::string& text, Vec2 center, float scale, Color color);

    float getTankSize() const { return 40.0f; }

    bool checkTankHit (const Tank& tank, Vec2 worldPos) const;
    bool checkTankCollision (const Tank& tankA, const Tank& tankB, Vec2& collisionPoint) const;

private:
    void createNoiseTexture();
    void drawFilledOval (Vec2 center, float width, float height, float angle, Color color);
    void drawChar (char c, Vec2 position, float scale, Color color);

    Texture2D noiseTexture1 = { 0 };
    Texture2D noiseTexture2 = { 0 };
    static constexpr int noiseTextureSize = 128;
};

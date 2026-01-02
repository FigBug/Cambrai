#include "Renderer.h"
#include "Config.h"
#include "Obstacles/Obstacle.h"
#include "Shell.h"
#include "Tank.h"
#include <algorithm>
#include <cmath>
#include <vector>

// Forward declare explosion struct
struct Explosion
{
    Vec2 position;
    float timer = 0.0f;
    float duration = 0.0f;
    float maxRadius = 0.0f;

    float getProgress() const { return timer / duration; }
    bool isAlive() const { return timer < duration; }
};

Renderer::Renderer()
{
    createNoiseTexture();
}

Renderer::~Renderer()
{
    if (noiseTexture1.id != 0)
        UnloadTexture (noiseTexture1);
    if (noiseTexture2.id != 0)
        UnloadTexture (noiseTexture2);
}

void Renderer::clear()
{
    ClearBackground (config.colorDirt);
}

void Renderer::drawDirt (float time, float screenWidth, float screenHeight)
{
    // Base dirt color
    ClearBackground (config.colorDirt);

    // Subtle noise texture overlay for terrain variation
    float tileSize = noiseTextureSize * 2.0f;

    int tilesX = (int) (screenWidth / tileSize) + 1;
    int tilesY = (int) (screenHeight / tileSize) + 1;

    for (int ty = 0; ty <= tilesY; ++ty)
    {
        for (int tx = 0; tx <= tilesX; ++tx)
        {
            float tileX = tx * tileSize;
            float tileY = ty * tileSize;

            Rectangle source = { 0, 0, (float) noiseTextureSize, (float) noiseTextureSize };
            Rectangle dest = { tileX, tileY, tileSize, tileSize };
            DrawTexturePro (noiseTexture1, source, dest, { 0, 0 }, 0.0f, WHITE);
        }
    }
}

void Renderer::present()
{
}

void Renderer::drawTank (const Tank& tank)
{
    Vec2 pos = tank.getPosition();
    float angle = tank.getAngle();
    float size = tank.getSize();

    // Calculate alpha for destroyed tanks
    float alpha = 1.0f;
    if (tank.isDestroying())
        alpha = 1.0f - tank.getDestroyProgress();

    Color tankColor = tank.getColor();
    tankColor.a = (unsigned char) (255 * alpha);

    // Tank body - slightly longer than wide
    float bodyLength = size * 1.2f;
    float bodyWidth = size * 0.8f;

    // Draw tank body
    drawFilledRotatedRect (pos, bodyLength, bodyWidth, angle, tankColor);

    // Tank tracks (darker strips on sides)
    Color trackColor = {
        (unsigned char) (tankColor.r * 0.6f),
        (unsigned char) (tankColor.g * 0.6f),
        (unsigned char) (tankColor.b * 0.6f),
        tankColor.a
    };

    float cosA = std::cos (angle);
    float sinA = std::sin (angle);

    float trackOffset = bodyWidth * 0.4f;
    float trackWidth = bodyWidth * 0.2f;

    // Left track
    Vec2 leftTrackPos = {
        pos.x - trackOffset * sinA,
        pos.y + trackOffset * cosA
    };
    drawFilledRotatedRect (leftTrackPos, bodyLength, trackWidth, angle, trackColor);

    // Right track
    Vec2 rightTrackPos = {
        pos.x + trackOffset * sinA,
        pos.y - trackOffset * cosA
    };
    drawFilledRotatedRect (rightTrackPos, bodyLength, trackWidth, angle, trackColor);

    // Turret base (circular)
    float turretBaseRadius = size * 0.3f;
    Color turretBaseColor = {
        (unsigned char) (tankColor.r * 0.8f),
        (unsigned char) (tankColor.g * 0.8f),
        (unsigned char) (tankColor.b * 0.8f),
        tankColor.a
    };
    drawFilledCircle (pos, turretBaseRadius, turretBaseColor);

    // Turret barrel
    float worldTurretAngle = angle + tank.getTurretAngle();
    float barrelLength = size * 0.7f;
    float barrelWidth = size * 0.12f;

    Vec2 barrelDir = Vec2::fromAngle (worldTurretAngle);
    Vec2 barrelCenter = pos + barrelDir * (barrelLength * 0.5f);

    Color barrelColor = { config.colorBarrel.r, config.colorBarrel.g, config.colorBarrel.b, tankColor.a };
    drawFilledRotatedRect (barrelCenter, barrelLength, barrelWidth, worldTurretAngle, barrelColor);

    // Outline
    Color outlineColor = {
        (unsigned char) (tankColor.r * 0.4f),
        (unsigned char) (tankColor.g * 0.4f),
        (unsigned char) (tankColor.b * 0.4f),
        tankColor.a
    };
    drawRotatedRect (pos, bodyLength, bodyWidth, angle, outlineColor);
}

void Renderer::drawTankGhost (const Tank& tank)
{
    Vec2 pos = tank.getPosition();
    float angle = tank.getAngle();
    float size = tank.getSize();

    // Semi-transparent grey color
    Color ghostColor = { 100, 100, 100, 100 };

    // Tank body - slightly longer than wide
    float bodyLength = size * 1.2f;
    float bodyWidth = size * 0.8f;

    // Draw tank body
    drawFilledRotatedRect (pos, bodyLength, bodyWidth, angle, ghostColor);

    // Tank tracks (darker strips on sides)
    Color trackColor = { 70, 70, 70, 100 };

    float cosA = std::cos (angle);
    float sinA = std::sin (angle);

    float trackOffset = bodyWidth * 0.4f;
    float trackWidth = bodyWidth * 0.2f;

    // Left track
    Vec2 leftTrackPos = {
        pos.x - trackOffset * sinA,
        pos.y + trackOffset * cosA
    };
    drawFilledRotatedRect (leftTrackPos, bodyLength, trackWidth, angle, trackColor);

    // Right track
    Vec2 rightTrackPos = {
        pos.x + trackOffset * sinA,
        pos.y - trackOffset * cosA
    };
    drawFilledRotatedRect (rightTrackPos, bodyLength, trackWidth, angle, trackColor);

    // Turret base (circular)
    float turretBaseRadius = size * 0.3f;
    Color turretBaseColor = { 80, 80, 80, 100 };
    drawFilledCircle (pos, turretBaseRadius, turretBaseColor);

    // Turret barrel
    float worldTurretAngle = angle + tank.getTurretAngle();
    float barrelLength = size * 0.7f;
    float barrelWidth = size * 0.12f;

    Vec2 barrelDir = Vec2::fromAngle (worldTurretAngle);
    Vec2 barrelCenter = pos + barrelDir * (barrelLength * 0.5f);

    Color barrelColor = { 60, 60, 60, 100 };
    drawFilledRotatedRect (barrelCenter, barrelLength, barrelWidth, worldTurretAngle, barrelColor);

    // Outline
    Color outlineColor = { 50, 50, 50, 100 };
    drawRotatedRect (pos, bodyLength, bodyWidth, angle, outlineColor);
}

void Renderer::drawTrackMarks (const Tank& tank)
{
    for (const auto& mark : tank.getTrackMarks())
    {
        unsigned char alpha = (unsigned char) (mark.alpha * config.colorTrackMark.a);
        Color color = { config.colorTrackMark.r, config.colorTrackMark.g, config.colorTrackMark.b, alpha };

        float size = tank.getSize();
        float trackOffset = size * 0.35f;
        float cosA = std::cos (mark.angle);
        float sinA = std::sin (mark.angle);

        // Perpendicular direction (for horizontal tread lines relative to tank)
        float perpX = -sinA;
        float perpY = cosA;

        float halfWidth = config.trackMarkWidth / 2.0f;

        // Draw two horizontal tread lines for left and right tracks
        Vec2 leftCenter = {
            mark.position.x - trackOffset * sinA,
            mark.position.y + trackOffset * cosA
        };
        Vec2 rightCenter = {
            mark.position.x + trackOffset * sinA,
            mark.position.y - trackOffset * cosA
        };

        // Left track tread mark (horizontal line perpendicular to tank direction)
        Vec2 leftStart = { leftCenter.x - perpX * halfWidth, leftCenter.y - perpY * halfWidth };
        Vec2 leftEnd = { leftCenter.x + perpX * halfWidth, leftCenter.y + perpY * halfWidth };
        drawLineThick (leftStart, leftEnd, config.trackMarkLength, color);

        // Right track tread mark
        Vec2 rightStart = { rightCenter.x - perpX * halfWidth, rightCenter.y - perpY * halfWidth };
        Vec2 rightEnd = { rightCenter.x + perpX * halfWidth, rightCenter.y + perpY * halfWidth };
        drawLineThick (rightStart, rightEnd, config.trackMarkLength, color);
    }
}

void Renderer::drawSmoke (const Tank& tank)
{
    for (const auto& s : tank.getSmoke())
    {
        unsigned char alpha = (unsigned char) (s.alpha * 180);
        Color color = { 80, 80, 80, alpha };
        drawFilledCircle (s.position, s.radius, color);
    }
}

void Renderer::drawShell (const Shell& shell)
{
    Vec2 pos = shell.getPosition();
    Vec2 vel = shell.getVelocity();
    float radius = shell.getRadius();

    // Draw trail behind shell
    if (vel.length() > 0.1f)
    {
        Vec2 trailDir = vel.normalized() * -1.0f;

        for (int i = config.shellTrailSegments; i >= 1; --i)
        {
            float t = (float) i / config.shellTrailSegments;
            Vec2 trailPos = pos + trailDir * (config.shellTrailLength * t);

            float alpha = (1.0f - t) * 0.8f;
            float trailRadius = radius * (1.0f - t * 0.3f);

            Color trailColor = {
                config.colorShellTracer.r,
                config.colorShellTracer.g,
                config.colorShellTracer.b,
                (unsigned char) (255 * alpha)
            };
            drawFilledCircle (trailPos, trailRadius, trailColor);
        }
    }

    // Draw shell
    drawFilledCircle (pos, radius, config.colorShell);
}

void Renderer::drawExplosion (const Explosion& explosion)
{
    float progress = explosion.getProgress();
    float radius = explosion.maxRadius * std::sqrt (progress);
    float alpha = 1.0f - progress;

    Color outerColor = { config.colorExplosionOuter.r, config.colorExplosionOuter.g, config.colorExplosionOuter.b, (unsigned char) (alpha * config.colorExplosionOuter.a) };
    drawCircle (explosion.position, radius, outerColor);

    if (radius > 5.0f)
    {
        Color midColor = { config.colorExplosionMid.r, config.colorExplosionMid.g, config.colorExplosionMid.b, (unsigned char) (alpha * config.colorExplosionMid.a) };
        drawCircle (explosion.position, radius * 0.7f, midColor);
    }

    if (radius > 10.0f)
    {
        Color coreColor = { config.colorExplosionCore.r, config.colorExplosionCore.g, config.colorExplosionCore.b, (unsigned char) (alpha * config.colorExplosionCore.a) };
        drawFilledCircle (explosion.position, radius * 0.3f, coreColor);
    }
}

void Renderer::drawCrosshair (const Tank& tank)
{
    Vec2 position = tank.getCrosshairPosition();
    Color tankColor = tank.getColor();

    Color crosshairColor = tank.isReadyToFire() ? tankColor : config.colorGreyMid;

    float size = 12.0f;

    // Draw crosshair lines
    drawLine ({ position.x - size, position.y }, { position.x + size, position.y }, crosshairColor);
    drawLine ({ position.x, position.y - size }, { position.x, position.y + size }, crosshairColor);

    // Center circle
    drawCircle (position, 4.0f, crosshairColor);

    // Reload bar
    float barWidth = 30.0f;
    float barHeight = 3.0f;
    float barY = position.y + size + 6.0f;
    drawFilledRect ({ position.x - barWidth / 2.0f, barY }, barWidth, barHeight, config.colorBarBackground);

    float reloadPct = tank.getReloadProgress();
    Color reloadColor = reloadPct >= 1.0f ? config.colorReloadReady : config.colorReloadNotReady;
    drawFilledRect ({ position.x - barWidth / 2.0f, barY }, barWidth * reloadPct, barHeight, reloadColor);
}

void Renderer::drawObstacle (const Obstacle& obstacle)
{
    obstacle.draw (*this);
}

void Renderer::drawObstaclePreview (const Obstacle& obstacle, bool valid)
{
    obstacle.drawPreview (*this, valid);
}

void Renderer::drawPit (const Obstacle& pit)
{
    if (pit.getType() != ObstacleType::Pit || !pit.isAlive())
        return;

    Vec2 pos = pit.getPosition();

    // Dark pit with concentric rings for depth effect
    drawFilledCircle (pos, config.pitRadius, config.colorPit);

    // Inner darker ring
    Color innerColor = { 20, 15, 10, 255 };
    drawFilledCircle (pos, config.pitRadius * 0.7f, innerColor);

    // Center darkest
    Color centerColor = { 10, 5, 0, 255 };
    drawFilledCircle (pos, config.pitRadius * 0.4f, centerColor);

    // Outline
    Color outlineColor = { 60, 50, 40, 255 };
    drawCircle (pos, config.pitRadius, outlineColor);
}

void Renderer::drawTankHUD (const Tank& tank, int slot, int totalSlots, float screenWidth, float hudWidth, float alpha)
{
    float hudHeight = 40.0f;
    float spacing = 10.0f;

    float totalWidth = totalSlots * hudWidth + (totalSlots - 1) * spacing;
    float startX = (screenWidth - totalWidth) / 2.0f;
    float x = startX + slot * (hudWidth + spacing);
    float y = 10.0f;

    unsigned char a = (unsigned char) (alpha * 255);

    Color tankColor = { tank.getColor().r, tank.getColor().g, tank.getColor().b, a };
    Color bgColor = { config.colorHudBackground.r, config.colorHudBackground.g, config.colorHudBackground.b, (unsigned char) (alpha * config.colorHudBackground.a) };
    Color barBg = { config.colorBarBackground.r, config.colorBarBackground.g, config.colorBarBackground.b, a };

    // Background
    drawFilledRect ({ x, y }, hudWidth, hudHeight, bgColor);
    drawRect ({ x, y }, hudWidth, hudHeight, tankColor);

    // Player number
    int playerNum = tank.getPlayerIndex() + 1;
    std::string label = "P" + std::to_string (playerNum);
    drawText (label, { x + 3, y + 3 }, 1.5f, tankColor);

    // Health bar
    float barX = x + 25.0f;
    float barWidth = hudWidth - 30.0f;
    float barHeight = 10.0f;
    float healthY = y + 5;

    drawFilledRect ({ barX, healthY }, barWidth, barHeight, barBg);
    float healthPct = tank.getHealth() / tank.getMaxHealth();
    Color healthColor = { (unsigned char) (255 * (1 - healthPct)), (unsigned char) (255 * healthPct), 0, a };
    drawFilledRect ({ barX, healthY }, barWidth * healthPct, barHeight, healthColor);

    // Throttle bar
    float throttleY = y + 22;
    drawFilledRect ({ barX, throttleY }, barWidth, barHeight, barBg);
    float throttle = tank.getThrottle();  // -1 to 1
    float throttlePct = std::abs (throttle);
    Color throttleColor = throttle >= 0 ? Color { 100, 200, 100, a } : Color { 200, 150, 100, a };
    float throttleBarWidth = barWidth * throttlePct;
    float throttleBarX = throttle >= 0 ? barX : barX + barWidth - throttleBarWidth;
    drawFilledRect ({ throttleBarX, throttleY }, throttleBarWidth, barHeight, throttleColor);
}

void Renderer::drawOval (Vec2 center, float width, float height, float angle, Color color)
{
    float cosA = std::cos (angle);
    float sinA = std::sin (angle);

    int segments = 32;
    for (int i = 0; i < segments; ++i)
    {
        float theta1 = (2.0f * pi * i) / segments;
        float theta2 = (2.0f * pi * (i + 1)) / segments;

        float x1 = (width / 2.0f) * std::cos (theta1);
        float y1 = (height / 2.0f) * std::sin (theta1);
        float x2 = (width / 2.0f) * std::cos (theta2);
        float y2 = (height / 2.0f) * std::sin (theta2);

        float rx1 = x1 * cosA - y1 * sinA;
        float ry1 = x1 * sinA + y1 * cosA;
        float rx2 = x2 * cosA - y2 * sinA;
        float ry2 = x2 * sinA + y2 * cosA;

        DrawLine ((int) (center.x + rx1), (int) (center.y + ry1),
                  (int) (center.x + rx2), (int) (center.y + ry2), color);
    }
}

void Renderer::drawFilledOval (Vec2 center, float width, float height, float angle, Color color)
{
    float cosA = std::cos (angle);
    float sinA = std::sin (angle);

    int numLines = (int) (std::max (width, height) / 2.0f);

    for (int i = -numLines; i <= numLines; ++i)
    {
        float t = (float) i / numLines;
        float localY = t * (height / 2.0f);

        float localHalfWidth = (width / 2.0f) * std::sqrt (1.0f - t * t);

        if (localHalfWidth > 0.0f)
        {
            float x1 = -localHalfWidth;
            float x2 = localHalfWidth;

            float wx1 = x1 * cosA - localY * sinA + center.x;
            float wy1 = x1 * sinA + localY * cosA + center.y;
            float wx2 = x2 * cosA - localY * sinA + center.x;
            float wy2 = x2 * sinA + localY * cosA + center.y;

            DrawLine ((int) wx1, (int) wy1, (int) wx2, (int) wy2, color);
        }
    }
}

void Renderer::drawCircle (Vec2 center, float radius, Color color)
{
    DrawCircleLinesV ({ center.x, center.y }, radius, color);
}

void Renderer::drawFilledCircle (Vec2 center, float radius, Color color)
{
    DrawCircleV ({ center.x, center.y }, radius, color);
}

void Renderer::drawLine (Vec2 start, Vec2 end, Color color)
{
    DrawLineV ({ start.x, start.y }, { end.x, end.y }, color);
}

void Renderer::drawLineThick (Vec2 start, Vec2 end, float thickness, Color color)
{
    DrawLineEx ({ start.x, start.y }, { end.x, end.y }, thickness, color);
}

void Renderer::drawRect (Vec2 topLeft, float width, float height, Color color)
{
    DrawRectangleLinesEx ({ topLeft.x, topLeft.y, width, height }, 1.0f, color);
}

void Renderer::drawFilledRect (Vec2 topLeft, float width, float height, Color color)
{
    DrawRectangleV ({ topLeft.x, topLeft.y }, { width, height }, color);
}

void Renderer::drawRotatedRect (Vec2 center, float width, float height, float angle, Color color)
{
    float cosA = std::cos (angle);
    float sinA = std::sin (angle);

    float hw = width / 2.0f;
    float hh = height / 2.0f;

    Vec2 corners[4] = {
        { -hw, -hh },
        { hw, -hh },
        { hw, hh },
        { -hw, hh }
    };

    for (int i = 0; i < 4; ++i)
    {
        Vec2& c = corners[i];
        float rx = c.x * cosA - c.y * sinA;
        float ry = c.x * sinA + c.y * cosA;
        c.x = center.x + rx;
        c.y = center.y + ry;
    }

    for (int i = 0; i < 4; ++i)
    {
        drawLine (corners[i], corners[(i + 1) % 4], color);
    }
}

void Renderer::drawFilledRotatedRect (Vec2 center, float width, float height, float angle, Color color)
{
    Rectangle rect = { center.x, center.y, width, height };
    Vector2 origin = { width / 2.0f, height / 2.0f };
    float angleDeg = angle * (180.0f / pi);
    DrawRectanglePro (rect, origin, angleDeg, color);
}

// Simple 5x7 bitmap font patterns
static const uint8_t* getGlyph (unsigned char c)
{
    static const uint8_t empty[7] = { 0, 0, 0, 0, 0, 0, 0 };

    static const uint8_t g0[7] = { 0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110 };
    static const uint8_t g1[7] = { 0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110 };
    static const uint8_t g2[7] = { 0b01110, 0b10001, 0b00001, 0b00110, 0b01000, 0b10000, 0b11111 };
    static const uint8_t g3[7] = { 0b01110, 0b10001, 0b00001, 0b00110, 0b00001, 0b10001, 0b01110 };
    static const uint8_t g4[7] = { 0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010 };
    static const uint8_t g5[7] = { 0b11111, 0b10000, 0b11110, 0b00001, 0b00001, 0b10001, 0b01110 };
    static const uint8_t g6[7] = { 0b00110, 0b01000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110 };
    static const uint8_t g7[7] = { 0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000 };
    static const uint8_t g8[7] = { 0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110 };
    static const uint8_t g9[7] = { 0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00010, 0b01100 };

    static const uint8_t gA[7] = { 0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001 };
    static const uint8_t gB[7] = { 0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110 };
    static const uint8_t gC[7] = { 0b01110, 0b10001, 0b10000, 0b10000, 0b10000, 0b10001, 0b01110 };
    static const uint8_t gD[7] = { 0b11110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11110 };
    static const uint8_t gE[7] = { 0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111 };
    static const uint8_t gF[7] = { 0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000 };
    static const uint8_t gG[7] = { 0b01110, 0b10001, 0b10000, 0b10111, 0b10001, 0b10001, 0b01110 };
    static const uint8_t gH[7] = { 0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001 };
    static const uint8_t gI[7] = { 0b01110, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110 };
    static const uint8_t gJ[7] = { 0b00111, 0b00010, 0b00010, 0b00010, 0b00010, 0b10010, 0b01100 };
    static const uint8_t gK[7] = { 0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001 };
    static const uint8_t gL[7] = { 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111 };
    static const uint8_t gM[7] = { 0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001 };
    static const uint8_t gN[7] = { 0b10001, 0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001 };
    static const uint8_t gO[7] = { 0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110 };
    static const uint8_t gP[7] = { 0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000 };
    static const uint8_t gQ[7] = { 0b01110, 0b10001, 0b10001, 0b10001, 0b10101, 0b10010, 0b01101 };
    static const uint8_t gR[7] = { 0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001 };
    static const uint8_t gS[7] = { 0b01110, 0b10001, 0b10000, 0b01110, 0b00001, 0b10001, 0b01110 };
    static const uint8_t gT[7] = { 0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100 };
    static const uint8_t gU[7] = { 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110 };
    static const uint8_t gV[7] = { 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100 };
    static const uint8_t gW[7] = { 0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b10101, 0b01010 };
    static const uint8_t gX[7] = { 0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001 };
    static const uint8_t gY[7] = { 0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100 };
    static const uint8_t gZ[7] = { 0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b11111 };

    static const uint8_t gExclaim[7] = { 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00000, 0b00100 };
    static const uint8_t gColon[7] = { 0b00000, 0b00100, 0b00100, 0b00000, 0b00100, 0b00100, 0b00000 };
    static const uint8_t gDash[7] = { 0b00000, 0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000 };

    switch (c)
    {
        case '0': return g0;
        case '1': return g1;
        case '2': return g2;
        case '3': return g3;
        case '4': return g4;
        case '5': return g5;
        case '6': return g6;
        case '7': return g7;
        case '8': return g8;
        case '9': return g9;
        case 'A': return gA;
        case 'B': return gB;
        case 'C': return gC;
        case 'D': return gD;
        case 'E': return gE;
        case 'F': return gF;
        case 'G': return gG;
        case 'H': return gH;
        case 'I': return gI;
        case 'J': return gJ;
        case 'K': return gK;
        case 'L': return gL;
        case 'M': return gM;
        case 'N': return gN;
        case 'O': return gO;
        case 'P': return gP;
        case 'Q': return gQ;
        case 'R': return gR;
        case 'S': return gS;
        case 'T': return gT;
        case 'U': return gU;
        case 'V': return gV;
        case 'W': return gW;
        case 'X': return gX;
        case 'Y': return gY;
        case 'Z': return gZ;
        case '!': return gExclaim;
        case ':': return gColon;
        case '-': return gDash;
        default: return empty;
    }
}

void Renderer::drawChar (char c, Vec2 position, float scale, Color color)
{
    unsigned char uc = (unsigned char) c;
    const uint8_t* glyph = getGlyph (uc);

    float pixelSize = std::ceil (scale);

    for (int row = 0; row < 7; ++row)
    {
        uint8_t rowBits = glyph[row];
        for (int col = 0; col < 5; ++col)
        {
            if (rowBits & (1 << (4 - col)))
            {
                Rectangle rect = {
                    position.x + col * scale,
                    position.y + row * scale,
                    pixelSize,
                    pixelSize
                };
                DrawRectangleRec (rect, color);
            }
        }
    }
}

void Renderer::drawText (const std::string& text, Vec2 position, float scale, Color color)
{
    float charWidth = 6 * scale;
    Vec2 pos = position;

    for (char c : text)
    {
        char upper = (c >= 'a' && c <= 'z') ? (c - 32) : c;
        drawChar (upper, pos, scale, color);
        pos.x += charWidth;
    }
}

void Renderer::drawTextCentered (const std::string& text, Vec2 center, float scale, Color color)
{
    float charWidth = 6 * scale;
    float charHeight = 7 * scale;
    float textWidth = text.length() * charWidth;

    Vec2 topLeft = {
        center.x - textWidth / 2.0f,
        center.y - charHeight / 2.0f
    };

    drawText (text, topLeft, scale, color);
}

void Renderer::createNoiseTexture()
{
    auto generateNoiseImage = [] (unsigned int seed) -> Image
    {
        Image noiseImage = GenImageColor (noiseTextureSize, noiseTextureSize, { 0, 0, 0, 0 });

        auto nextRandom = [&seed]() -> unsigned int
        {
            seed = seed * 1103515245 + 12345;
            return (seed >> 16) & 0x7FFF;
        };

        for (int y = 0; y < noiseTextureSize; ++y)
        {
            for (int x = 0; x < noiseTextureSize; ++x)
            {
                unsigned int r = nextRandom() % 100;
                Color pixelColor = { 0, 0, 0, 0 };

                if (r < 15)
                {
                    // Dark spot
                    pixelColor = { config.colorDirtDark.r, config.colorDirtDark.g, config.colorDirtDark.b, 40 };
                }
                else if (r < 30)
                {
                    // Light spot
                    pixelColor = { config.colorDirtLight.r, config.colorDirtLight.g, config.colorDirtLight.b, 30 };
                }

                ImageDrawPixel (&noiseImage, x, y, pixelColor);
            }
        }

        return noiseImage;
    };

    Image noiseImage1 = generateNoiseImage (12345);
    noiseTexture1 = LoadTextureFromImage (noiseImage1);
    UnloadImage (noiseImage1);
    SetTextureFilter (noiseTexture1, TEXTURE_FILTER_BILINEAR);

    Image noiseImage2 = generateNoiseImage (67890);
    noiseTexture2 = LoadTextureFromImage (noiseImage2);
    UnloadImage (noiseImage2);
    SetTextureFilter (noiseTexture2, TEXTURE_FILTER_BILINEAR);
}

bool Renderer::checkTankHit (const Tank& tank, Vec2 worldPos) const
{
    // Simple circular hit test
    Vec2 diff = worldPos - tank.getPosition();
    float dist = diff.length();
    return dist < tank.getSize() * 0.6f;
}

bool Renderer::checkTankHitLine (const Tank& tank, Vec2 lineStart, Vec2 lineEnd, Vec2& hitPoint) const
{
    // Line-circle intersection test
    // Tank is approximated as a circle with radius = size * 0.6
    Vec2 center = tank.getPosition();
    float radius = tank.getSize() * 0.6f;

    Vec2 d = lineEnd - lineStart;
    Vec2 f = lineStart - center;

    float a = d.dot (d);
    float b = 2.0f * f.dot (d);
    float c = f.dot (f) - radius * radius;

    float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0)
        return false;

    discriminant = std::sqrt (discriminant);

    // Find the nearest intersection point along the line segment
    float t1 = (-b - discriminant) / (2.0f * a);
    float t2 = (-b + discriminant) / (2.0f * a);

    // Check if either intersection is within the line segment [0, 1]
    if (t1 >= 0.0f && t1 <= 1.0f)
    {
        hitPoint = lineStart + d * t1;
        return true;
    }
    if (t2 >= 0.0f && t2 <= 1.0f)
    {
        hitPoint = lineStart + d * t2;
        return true;
    }

    return false;
}

bool Renderer::checkTankCollision (const Tank& tankA, const Tank& tankB, Vec2& collisionPoint) const
{
    Vec2 diff = tankB.getPosition() - tankA.getPosition();
    float dist = diff.length();
    float combinedRadius = (tankA.getSize() + tankB.getSize()) * 0.5f;

    if (dist < combinedRadius)
    {
        collisionPoint = tankA.getPosition() + diff * 0.5f;
        return true;
    }

    return false;
}

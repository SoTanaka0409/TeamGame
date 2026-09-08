#define NOMINMAX
#include "Player.h"
#include "Stage.h"
#include "DxLib.h"
#include <cmath>
#include <algorithm>

Player::Player()
    : m_x(3.5f), m_y(13.5f), m_speed(0.20f), m_radius(0.55f), m_lightAngle(0.0f), m_isLightOn(true), m_isInBush(false)
{
}

void Player::Initialize(float startX, float startY)
{
    m_x = startX;
    m_y = startY;
    m_lightAngle = 0.0f;
    m_isLightOn = true;
    m_isInBush = false;
    m_prevMouseRight = false;
}

void Player::Update(const Stage& stage, int screenWidth, int screenHeight, float startDrawX, float startDrawY, float cellSize)
{
    if (stage.GetWidth() <= 0 || stage.GetHeight() <= 0 || cellSize <= 0.0f) return;

    // 1. 右クリックで懐中電灯 ON / OFF トグル切り替え
    bool currMouseRight = ((GetMouseInput() & MOUSE_INPUT_RIGHT) != 0);
    if (currMouseRight && !m_prevMouseRight)
    {
        m_isLightOn = !m_isLightOn;
    }
    m_prevMouseRight = currMouseRight;

    // 2. 移動入力の計算
    float moveX = 0.0f;
    float moveY = 0.0f;

    if (CheckHitKey(KEY_INPUT_LEFT) || CheckHitKey(KEY_INPUT_A))  moveX -= m_speed;
    if (CheckHitKey(KEY_INPUT_RIGHT) || CheckHitKey(KEY_INPUT_D)) moveX += m_speed;
    if (CheckHitKey(KEY_INPUT_UP) || CheckHitKey(KEY_INPUT_W))    moveY -= m_speed;
    if (CheckHitKey(KEY_INPUT_DOWN) || CheckHitKey(KEY_INPUT_S))  moveY += m_speed;

    if (moveX != 0.0f && moveY != 0.0f)
    {
        moveX *= 0.7071f;
        moveY *= 0.7071f;
    }

    // X軸移動 & 衝突判定 (境界外ガード)
    float newX = m_x + moveX;
    int checkX = static_cast<int>(newX);
    int currentY = static_cast<int>(m_y);

    if (checkX >= 0 && checkX < stage.GetWidth() && !stage.IsSolidWall(checkX, currentY))
    {
        m_x = newX;
    }

    // Y軸移動 & 衝突判定 (境界外ガード)
    float newY = m_y + moveY;
    int currentX = static_cast<int>(m_x);
    int checkY = static_cast<int>(newY);

    if (checkY >= 0 && checkY < stage.GetHeight() && !stage.IsSolidWall(currentX, checkY))
    {
        m_y = newY;
    }

    // 3. 草むら隠れ状態（ステルス）判定
    int pGridX = static_cast<int>(m_x);
    int pGridY = static_cast<int>(m_y);
    if (pGridX >= 0 && pGridX < stage.GetWidth() && pGridY >= 0 && pGridY < stage.GetHeight())
    {
        m_isInBush = (stage.GetCell(pGridX, pGridY) == CellType::BUSH);
    }
    else
    {
        m_isInBush = false;
    }

    // 4. マウス照準角度の更新
    int mouseX = 0, mouseY = 0;
    GetMousePoint(&mouseX, &mouseY);

    float playerPixelX = startDrawX + m_x * cellSize;
    float playerPixelY = startDrawY + m_y * cellSize;

    m_lightAngle = std::atan2(mouseY - playerPixelY, mouseX - playerPixelX);
}

void Player::RenderLightMask(const Stage& stage, int rectX, int rectY, int rectW, int rectH, float startDrawX, float startDrawY, float cellSize) const
{
    if (stage.GetWidth() <= 0 || stage.GetHeight() <= 0 || cellSize <= 0.0f) return;

    float playerPixelX = startDrawX + m_x * cellSize;
    float playerPixelY = startDrawY + m_y * cellSize;

    float maxSpotDist = cellSize * m_maxSpotDistCells;
    float closeRadius = cellSize * m_closeRadiusCells;

    int stageWidth = stage.GetWidth();
    int stageHeight = stage.GetHeight();

    const int resolutionStep = 4; // 高速4pxステップ

    for (int py = rectY; py < rectY + rectH; py += resolutionStep)
    {
        for (int px = rectX; px < rectX + rectW; px += resolutionStep)
        {
            float dx = px - playerPixelX;
            float dy = py - playerPixelY;
            float dist = std::sqrt(dx * dx + dy * dy);

            float lightVal = 0.0f;

            // A. プレイヤー周囲の足元明かり
            if (dist < closeRadius)
            {
                float ambientLight = 0.38f * (1.0f - (dist / closeRadius) * 0.6f);
                lightVal = std::max(lightVal, ambientLight);
            }

            // B. 前方60°扇形スポットライト (ライトスイッチがONの時のみ照射)
            if (m_isLightOn && dist < maxSpotDist)
            {
                float cellAngle = std::atan2(dy, dx);
                float angleDiff = std::abs(cellAngle - m_lightAngle);
                while (angleDiff > 3.14159265f) angleDiff = std::abs(angleDiff - 2.0f * 3.14159265f);

                if (angleDiff < m_fanAngleHalf)
                {
                    bool isBlocked = false;

                    // 水場WATERと草むらBUSHは光が透過し、壁・木箱・サボテンのみが光を遮断
                    if (dist > cellSize * 0.8f)
                    {
                        int raySteps = static_cast<int>(dist / (cellSize * 0.60f));
                        if (raySteps < 2) raySteps = 2;

                        float stepX = dx / raySteps;
                        float stepY = dy / raySteps;

                        float currPx = playerPixelX + stepX;
                        float currPy = playerPixelY + stepY;

                        for (int s = 1; s < raySteps; ++s)
                        {
                            int gX = static_cast<int>((currPx - startDrawX) / cellSize);
                            int gY = static_cast<int>((currPy - startDrawY) / cellSize);

                            if (gX < 0 || gX >= stageWidth || gY < 0 || gY >= stageHeight || stage.IsLightBlockingWall(gX, gY))
                            {
                                isBlocked = true;
                                break;
                            }
                            currPx += stepX;
                            currPy += stepY;
                        }
                    }

                    if (!isBlocked)
                    {
                        float distFade = 1.0f - (dist / maxSpotDist);
                        distFade = distFade * distFade;
                        float angleFade = 1.0f - (angleDiff / m_fanAngleHalf);
                        float spotLight = distFade * angleFade * 0.95f;

                        lightVal = std::max(lightVal, spotLight);
                    }
                }
            }

            if (lightVal < 0.98f)
            {
                int alpha = static_cast<int>((1.0f - std::min(1.0f, lightVal)) * 248);
                if (alpha > 8)
                {
                    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
                    DrawBox(px, py, px + resolutionStep, py + resolutionStep, GetColor(4, 5, 10), TRUE);
                }
            }
        }
    }

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void Player::Draw(float startDrawX, float startDrawY, float cellSize, bool isDebugMode) const
{
    if (cellSize <= 0.0f) return;

    float px = startDrawX + m_x * cellSize;
    float py = startDrawY + m_y * cellSize;

    int pr = static_cast<int>(cellSize * m_radius);
    if (pr < 4) pr = 4;

    if (m_isInBush && !isDebugMode)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 140);
        DrawCircle(static_cast<int>(px), static_cast<int>(py), pr, GetColor(30, 180, 80), TRUE);
        DrawCircle(static_cast<int>(px), static_cast<int>(py), pr, GetColor(255, 255, 255), FALSE);
        DrawString(static_cast<int>(px) - 4, static_cast<int>(py) - 6, "P", GetColor(255, 255, 255));
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
    else
    {
        DrawCircle(static_cast<int>(px), static_cast<int>(py), pr + 1, GetColor(255, 255, 255), FALSE);
        DrawCircle(static_cast<int>(px), static_cast<int>(py), pr, GetColor(0, 220, 100), TRUE);
        DrawCircle(static_cast<int>(px), static_cast<int>(py), pr, GetColor(0, 120, 50), FALSE);
        DrawString(static_cast<int>(px) - 4, static_cast<int>(py) - 6, "P", GetColor(0, 0, 0));
    }
}

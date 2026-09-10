#include "Camera.h"
#define NOMINMAX
#include "Player.h"
#include "Stage.h"
#include "Bullet.h"
#include "DxLib.h"
#include "Enemy.h"
#include "Handgun.h"
#include "InputManager.h"
#include "Shotgun.h"
#include "SceneManager.h"
#include "Scene.h"
#include <cmath>
#include <algorithm>

Player::Player(float startX, float startY)
    : Character(ObjectTag::Player, startX, startY, 35.0f), damageColorTimer(0),
      facingDir(0.0f, -1.0f)
{
    speed = 5.0f;
    collider->SetTag("Player");
    weapons.push_back(new Handgun());
    weapons.push_back(new Shotgun());
    currentWeaponIndex = 0;
}

Player::~Player()
{
    for (auto w : weapons)
        delete w;
    weapons.clear();
}

void Player::Update()
{
    if (!isRemote)
    {
        // 右クリックで懐中電灯 ON / OFF トグル切り替え
        bool currMouseRight = ((GetMouseInput() & MOUSE_INPUT_RIGHT) != 0);
        if (currMouseRight && !m_prevMouseRight)
        {
            m_isLightOn = !m_isLightOn;
        }
        m_prevMouseRight = currMouseRight;
    }

    // 草むら隠れ状態判定
    if (currentStage)
    {
        int pGridX = static_cast<int>(position.x / cellSize);
        int pGridY = static_cast<int>(position.y / cellSize);
        if (pGridX >= 0 && pGridX < currentStage->GetWidth() && pGridY >= 0 && pGridY < currentStage->GetHeight())
        {
            m_isInBush = (currentStage->GetCell(pGridX, pGridY) == CellType::BUSH);
        }
        else
        {
            m_isInBush = false;
        }
    }
    bool isMoving = false;
    Vector2 moveDir(0.0f, 0.0f);

    if (!isRemote)
    {
        if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_LEFT) || InputManager::GetInstance().IsKeyHeld(KEY_INPUT_A))
        {
            moveDir.x -= 1.0f;
            isMoving = true;
        }
        if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_RIGHT) || InputManager::GetInstance().IsKeyHeld(KEY_INPUT_D))
        {
            moveDir.x += 1.0f;
            isMoving = true;
        }
        if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_UP) || InputManager::GetInstance().IsKeyHeld(KEY_INPUT_W))
        {
            moveDir.y -= 1.0f;
            isMoving = true;
        }
        if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_DOWN) || InputManager::GetInstance().IsKeyHeld(KEY_INPUT_S))
        {
            moveDir.y += 1.0f;
            isMoving = true;
        }
    }

    if (isMoving)
    {
        float length = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
        if (length > 0.0001f)
        {
            float velX = (moveDir.x / length) * speed;
            float velY = (moveDir.y / length) * speed;
            
            // X軸の移動と衝突判定
            if (currentStage)
            {
                float nextX = position.x + velX;
                int gridX = static_cast<int>(nextX / cellSize);
                int gridY = static_cast<int>(position.y / cellSize);
                if (!currentStage->IsSolidWall(gridX, gridY))
                {
                    position.x = nextX;
                }
                
                // Y軸の移動と衝突判定
                float nextY = position.y + velY;
                gridX = static_cast<int>(position.x / cellSize);
                gridY = static_cast<int>(nextY / cellSize);
                if (!currentStage->IsSolidWall(gridX, gridY))
                {
                    position.y = nextY;
                }
            }
            else
            {
                position.x += velX;
                position.y += velY;
            }
        }
    }

    if (damageColorTimer > 0)
    {
        damageColorTimer--;
    }

    if (!weapons.empty())
    {
        weapons[currentWeaponIndex]->Update();
    }

    if (!isRemote)
    {
        int mouseX, mouseY;
        GetMousePoint(&mouseX, &mouseY);
        float dx = mouseX - Camera::WorldToScreenX(position.x);
        float dy = mouseY - Camera::WorldToScreenY(position.y);
        float dirLen = std::sqrt(dx * dx + dy * dy);
        if (dirLen > 0.0001f)
        {
            facingDir.x = dx / dirLen;
            facingDir.y = dy / dirLen;
        }

        // Qキーで武器チェンジ
        if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_Q))
        {
            currentWeaponIndex = (currentWeaponIndex + 1) % weapons.size();
        }

        // 左クリックまたはZキーで発砲
        if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_Z) || (GetMouseInput() & MOUSE_INPUT_LEFT))
        {
            if (!weapons.empty())
            {
                weapons[currentWeaponIndex]->Fire(position, facingDir);
            }
        }
    }

    // Spaceキーでナイフ暗殺 (敵が未発覚時のみ実行可能)
    if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_SPACE))
    {
        auto scene = SceneManager::GetInstance().GetCurrentScene();
        if (scene && scene->GetObjectManager())
        {
            Enemy* targetEnemy = nullptr;
            float minDistSq = 95.0f * 95.0f; // 暗殺可能距離

            for (auto obj : scene->GetObjectManager()->GetObjects())
            {
                Enemy *enemy = dynamic_cast<Enemy *>(obj);
                if (enemy && enemy->IsActive())
                {
                    // 敵がこちらに気づいていない(非ALERT状態)場合のみ暗殺可能
                    if (!enemy->IsAlerted())
                    {
                        Vector2 ePos = enemy->GetPosition();
                        float dx = ePos.x - position.x;
                        float dy = ePos.y - position.y;
                        float distSq = dx * dx + dy * dy;
                        if (distSq < minDistSq)
                        {
                            minDistSq = distSq;
                            targetEnemy = enemy;
                        }
                    }
                }
            }

            if (targetEnemy)
            {
                Vector2 ePos = targetEnemy->GetPosition();
                float dx = ePos.x - position.x;
                float dy = ePos.y - position.y;
                float dirAngle = std::atan2(dy, dx);

                facingDir = Vector2(dx, dy);
                float len = std::sqrt(dx * dx + dy * dy);
                if (len > 0.0001f)
                {
                    facingDir.x /= len;
                    facingDir.y /= len;
                }

                if (scene->GetEffectManager())
                {
                    scene->GetEffectManager()->AddKnifeSlashEffect(ePos.x, ePos.y, dirAngle);
                }

                targetEnemy->StealthKill();
            }
        }
    }
}

void Player::Draw()
{
    // 画面中央 (960, 540) に固定描画
    float screenX = Camera::WorldToScreenX(position.x);
    float screenY = Camera::WorldToScreenY(position.y);

    if (m_isInBush)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 140);
    }

    unsigned int color =
        (damageColorTimer > 0) ? GetColor(255, 255, 0) : GetColor(0, 255, 0);
    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
               static_cast<int>(radius), color, TRUE);

    // 向いている方角を線で描画
    float lineLen = 50.0f; // 線の長さ
    float len =
        std::sqrt(facingDir.x * facingDir.x + facingDir.y * facingDir.y);
    float nx = facingDir.x;
    float ny = facingDir.y;

    if (len > 0.0001f)
    {
        nx /= len;
        ny /= len;
    }

    int x1 = static_cast<int>(screenX);
    int y1 = static_cast<int>(screenY);
    int x2 = static_cast<int>(screenX + nx * lineLen);
    int y2 = static_cast<int>(screenY + ny * lineLen);

    // 白い線を描画（太さ2）
    DrawLine(x1, y1, x2, y2, GetColor(255, 255, 255), 2);

    // 【赤色でやや透明な弾道予測線】
    if (currentStage && cellSize > 0.0f)
    {
        float zoomCellSize = 75.0f;
        float maxRange = cellSize * 12.0f;
        float stepDist = cellSize * 0.4f;
        float currDist = radius + 5.0f;
        Vector2 hitPos = Vector2(position.x + nx * maxRange, position.y + ny * maxRange);

        while (currDist < maxRange)
        {
            float testX = position.x + nx * currDist;
            float testY = position.y + ny * currDist;
            int gX = static_cast<int>(testX / cellSize);
            int gY = static_cast<int>(testY / cellSize);

            if (currentStage->IsOutOfBounds(gX, gY) || currentStage->IsSolidWall(gX, gY))
            {
                hitPos = Vector2(testX, testY);
                break;
            }
            currDist += stepDist;
        }

        float zoomScale = zoomCellSize / cellSize;
        float hitScreenX = Camera::WorldToScreenX(hitPos.x);
        float hitScreenY = Camera::WorldToScreenY(hitPos.y);

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
        DrawLine(static_cast<int>(screenX), static_cast<int>(screenY),
                 static_cast<int>(hitScreenX), static_cast<int>(hitScreenY),
                 GetColor(255, 60, 60), 2);
        DrawCircle(static_cast<int>(hitScreenX), static_cast<int>(hitScreenY), 4, GetColor(255, 100, 100), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    if (!weapons.empty())
    {
        DrawString(static_cast<int>(screenX) - 20,
                   static_cast<int>(screenY) - 30,
                   weapons[currentWeaponIndex]->GetName().c_str(),
                   GetColor(255, 255, 255));
    }

    // 未発覚敵の近接ナイフ暗殺案内UI表示
    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (scene && scene->GetObjectManager())
    {
        Enemy* canKillEnemy = nullptr;
        float minDistSq = 95.0f * 95.0f;

        for (auto obj : scene->GetObjectManager()->GetObjects())
        {
            Enemy *enemy = dynamic_cast<Enemy *>(obj);
            if (enemy && enemy->IsActive() && !enemy->IsAlerted())
            {
                Vector2 ePos = enemy->GetPosition();
                float dx = ePos.x - position.x;
                float dy = ePos.y - position.y;
                float distSq = dx * dx + dy * dy;
                if (distSq < minDistSq)
                {
                    minDistSq = distSq;
                    canKillEnemy = enemy;
                }
            }
        }

        if (canKillEnemy)
        {
            float zoomScale = 75.0f / (cellSize > 0.0f ? cellSize : 40.0f);
            Vector2 ePos = canKillEnemy->GetPosition();
            float eScreenX = Camera::WorldToScreenX(ePos.x);
            float eScreenY = Camera::WorldToScreenY(ePos.y);

            int boxX1 = static_cast<int>(eScreenX - 75);
            int boxY1 = static_cast<int>(eScreenY - 55);
            int boxX2 = static_cast<int>(eScreenX + 75);
            int boxY2 = static_cast<int>(eScreenY - 30);

            DrawBox(boxX1, boxY1, boxX2, boxY2, GetColor(20, 20, 20), TRUE);
            DrawBox(boxX1, boxY1, boxX2, boxY2, GetColor(255, 200, 0), FALSE);
            DrawStringF(eScreenX - 65.0f, eScreenY - 50.0f, "[SPACE] KNIFE KILL", GetColor(255, 230, 0));
        }
    }

    if (m_isInBush)
    {
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}

void Player::TakeDamage()
{
    damageColorTimer = 30;
}

void Player::OnCollisionEnter(Collider *otherCollider)
{
    // 接触によるダメージは0
}

void Player::OnCollisionStay(Collider *otherCollider)
{
}

void Player::OnCollisionExit(Collider *otherCollider)
{
}

void Player::RenderLightMask(int rectX, int rectY, int rectW, int rectH, float startDrawX, float startDrawY) const
{
    if (!currentStage || cellSize <= 0.0f) return;

    // プレイヤーの画面上での中心位置 (960, 540)
    float playerPixelX = Camera::WorldToScreenX(position.x);
    float playerPixelY = Camera::WorldToScreenY(position.y);

    float zoomCellSize = 75.0f;
    float zoomScale = zoomCellSize / cellSize;

    float maxSpotDist = zoomCellSize * m_maxSpotDistCells;
    float closeRadius = zoomCellSize * m_closeRadiusCells;

    int stageWidth = currentStage->GetWidth();
    int stageHeight = currentStage->GetHeight();

    const int resolutionStep = 4; // 高速4pxステップ

    float lightAngle = std::atan2(facingDir.y, facingDir.x);

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
                lightVal = (std::max)(lightVal, ambientLight);
            }

            // B. 前方60°扇形スポットライト
            if (m_isLightOn && dist < maxSpotDist)
            {
                float cellAngle = std::atan2(dy, dx);
                float angleDiff = std::abs(cellAngle - lightAngle);
                while (angleDiff > 3.14159265f) angleDiff = std::abs(angleDiff - 2.0f * 3.14159265f);

                if (angleDiff < m_fanAngleHalf)
                {
                    bool isBlocked = false;

                    if (dist > zoomCellSize * 0.8f)
                    {
                        int raySteps = static_cast<int>(dist / (zoomCellSize * 0.60f));
                        if (raySteps < 2) raySteps = 2;

                        float stepX = dx / raySteps;
                        float stepY = dy / raySteps;

                        float currPx = playerPixelX + stepX;
                        float currPy = playerPixelY + stepY;

                        for (int s = 1; s < raySteps; ++s)
                        {
                            float worldX = Camera::ScreenToWorldX(currPx);
                            float worldY = Camera::ScreenToWorldY(currPy);

                            int gX = static_cast<int>(worldX / cellSize);
                            int gY = static_cast<int>(worldY / cellSize);

                            if (gX < 0 || gX >= stageWidth || gY < 0 || gY >= stageHeight || currentStage->IsLightBlockingWall(gX, gY))
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

                        lightVal = (std::max)(lightVal, spotLight);
                    }
                }
            }

            if (lightVal < 0.98f)
            {
                int alpha = static_cast<int>((1.0f - (std::min)(1.0f, lightVal)) * 248);
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

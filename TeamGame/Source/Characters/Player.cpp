#define NOMINMAX
#include "Player.h"
#include "Stage.h"
#include "Bullet.h"
#include "DxLib.h"
#include "Enemy.h"
#include "Handgun.h"
#include "InputManager.h"
#include "Shotgun.h"
#include "GameSettings.h"
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
    if (m_isRemote)
    {
        // リモートプレイヤーの場合は移動や入力を処理しない
        // （アニメーションなどの更新が必要ならここに書く）
        return;
    }

    // 右クリックで懐中電灯 ON / OFF トグル切り替え
    bool currMouseRight = ((GetMouseInput() & MOUSE_INPUT_RIGHT) != 0);
    if (currMouseRight && !m_prevMouseRight)
    {
        m_isLightOn = !m_isLightOn;
    }
    m_prevMouseRight = currMouseRight;

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

    // GameSettingsの取得
    bool currE = (CheckHitKey(KEY_INPUT_E) != 0);

    if (GameSettings::GetInstance().isAimLockHoldMode)
    {
        // ONの場合: Eキーを押している間は視点固定
        m_isAimLocked = currE;
    }
    else
    {
        // OFFの場合: Eキーが押された時に視点固定をトグル（eeが押されるまで）
        if (currE && !m_prevE)
        {
            m_isAimLocked = !m_isAimLocked;
        }
    }
    m_prevE = currE;

    // マウスで視点移動（照準を合わせる） - 視点固定されていない時のみ
    if (!m_isAimLocked)
    {
        int mouseX, mouseY;
        GetMousePoint(&mouseX, &mouseY);
        float dx = mouseX - position.x;
        float dy = mouseY - position.y;
        float dirLen = std::sqrt(dx * dx + dy * dy);
        if (dirLen > 0.0001f)
        {
            facingDir.x = dx / dirLen;
            facingDir.y = dy / dirLen;
        }
    }

    // Qキーで武器チェンジ
    if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_Q))
    {
        currentWeaponIndex = (currentWeaponIndex + 1) % weapons.size();
    }

    // 左クリックまたはZキーで発射
    if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_Z) || (GetMouseInput() & MOUSE_INPUT_LEFT))
    {
        if (!weapons.empty())
        {
            weapons[currentWeaponIndex]->Fire(position, facingDir);
        }
    }
}

void Player::Draw()
{
    if (m_isInBush)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 140);
    }

    unsigned int color =
        (damageColorTimer > 0) ? GetColor(255, 255, 0) : GetColor(0, 255, 0);
    DrawCircle(static_cast<int>(position.x), static_cast<int>(position.y),
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

    int x1 = static_cast<int>(position.x);
    int y1 = static_cast<int>(position.y);
    int x2 = static_cast<int>(position.x + nx * lineLen);
    int y2 = static_cast<int>(position.y + ny * lineLen);

    // 白い線を描画（太さ2）
    DrawLine(x1, y1, x2, y2, GetColor(255, 255, 255), 2);

    if (!weapons.empty())
    {
        DrawString(static_cast<int>(position.x) - 20,
                   static_cast<int>(position.y) - 30,
                   weapons[currentWeaponIndex]->GetName().c_str(),
                   GetColor(255, 255, 255));
    }

    if (m_isInBush)
    {
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}

void Player::OnCollisionEnter(Collider *otherCollider)
{
    Enemy *enemy = dynamic_cast<Enemy *>(otherCollider->GetOwner());
    if (enemy)
    {
        enemy->Damage();
        damageColorTimer = 30;
    }
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

    float playerPixelX = position.x;
    float playerPixelY = position.y;

    float maxSpotDist = cellSize * m_maxSpotDistCells;
    float closeRadius = cellSize * m_closeRadiusCells;

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

            // B. 前方60°扇形スポットライト (ライトスイッチがONの時のみ照射)
            if (m_isLightOn && dist < maxSpotDist)
            {
                float cellAngle = std::atan2(dy, dx);
                float angleDiff = std::abs(cellAngle - lightAngle);
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

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
#include "GameSettings.h"
#include <cmath>
#include <algorithm>

Player::Player(float startX, float startY)
    : Character(ObjectTag::Player, startX, startY, 35.0f), damageColorTimer(0),
      facingDir(0.0f, -1.0f)
{
    teamId = 0; // プレイヤーはTeam 0
    status.Init(10, 5.0f, 1);
    collider->SetTag("Player");
    collider->SetRadius(20.0f); // 重なった時のみ判定されるタイトなコライダー半径
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
        if (m_inputType == PlayerInputType::KEYBOARD_MOUSE)
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
        else if (m_inputType == PlayerInputType::GAMEPAD_1)
        {
            int padState = GetJoypadInputState(DX_INPUT_PAD1);
            int ax = 0, ay = 0;
            GetJoypadAnalogInput(&ax, &ay, DX_INPUT_PAD1);
            if (ax < -200) moveDir.x -= 1.0f;
            if (ax > 200) moveDir.x += 1.0f;
            if (ay < -200) moveDir.y -= 1.0f;
            if (ay > 200) moveDir.y += 1.0f;
            
            if (padState & PAD_INPUT_LEFT) moveDir.x -= 1.0f;
            if (padState & PAD_INPUT_RIGHT) moveDir.x += 1.0f;
            if (padState & PAD_INPUT_UP) moveDir.y -= 1.0f;
            if (padState & PAD_INPUT_DOWN) moveDir.y += 1.0f;
            
            if (moveDir.x != 0.0f || moveDir.y != 0.0f) isMoving = true;
        }
    }

    if (isMoving)
    {
        float length = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
        if (length > 0.0001f)
        {
            // HPが3以下の負傷状態では移動速度が低下(60%に低下)
            float moveSpeed = status.GetSpeed();
            if (status.GetCurrentHp() <= 3)
            {
                moveSpeed *= 0.60f;
            }

            float velX = (moveDir.x / length) * moveSpeed;
            float velY = (moveDir.y / length) * moveSpeed;
            
            // X軸の移動と衝突判定
            if (currentStage)
            {
                position.x += velX;
                position.y += velY;
                currentStage->ResolveCollision(position, 20.0f, cellSize);
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
        if (m_inputType == PlayerInputType::KEYBOARD_MOUSE)
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

            if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_Q))
            {
                currentWeaponIndex = (currentWeaponIndex + 1) % weapons.size();
            }

            if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_Z) || (GetMouseInput() & MOUSE_INPUT_LEFT))
            {
                if (!weapons.empty()) weapons[currentWeaponIndex]->Fire(position, facingDir, teamId);
            }
        }
        else if (m_inputType == PlayerInputType::GAMEPAD_1)
        {
            int rx = 0, ry = 0;
            GetJoypadAnalogInputRight(&rx, &ry, DX_INPUT_PAD1);
            if (rx < -200 || rx > 200 || ry < -200 || ry > 200)
            {
                float dx = (float)rx;
                float dy = (float)ry;
                float dirLen = std::sqrt(dx * dx + dy * dy);
                if (dirLen > 0.0001f)
                {
                    facingDir.x = dx / dirLen;
                    facingDir.y = dy / dirLen;
                }
            }
            
            int padState = GetJoypadInputState(DX_INPUT_PAD1);
            static int prevPadState = 0;
            
            if ((padState & PAD_INPUT_3) && !(prevPadState & PAD_INPUT_3)) // Button X
            {
                currentWeaponIndex = (currentWeaponIndex + 1) % weapons.size();
            }
            if ((padState & PAD_INPUT_4) && !(prevPadState & PAD_INPUT_4)) // Button Y
            {
                ToggleLight();
            }
            
            if (padState & PAD_INPUT_1) // Button A (or R1)
            {
                if (!weapons.empty()) weapons[currentWeaponIndex]->Fire(position, facingDir, teamId);
            }
            
            prevPadState = padState;
        }
    }

    bool isKnifePressed = false;
    bool isReloadPressed = false;
    if (!isRemote)
    {
        if (m_inputType == PlayerInputType::KEYBOARD_MOUSE) {
            isKnifePressed = InputManager::GetInstance().IsKeyPressed(KEY_INPUT_SPACE);
            isReloadPressed = InputManager::GetInstance().IsKeyPressed(KEY_INPUT_R);
        }
        else if (m_inputType == PlayerInputType::GAMEPAD_1) {
            int pad = GetJoypadInputState(DX_INPUT_PAD1);
            isKnifePressed = (pad & PAD_INPUT_2) != 0;
            isReloadPressed = (pad & PAD_INPUT_5) != 0; // L1 or similar
        }
    }
    
    if (isReloadPressed && !weapons.empty())
    {
        weapons[currentWeaponIndex]->Reload();
    }
    
    // Spaceキー等でナイフ暗殺 (敵が未発覚時のみ実行可能)
    if (isKnifePressed)
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
        // 装備中武器の射程距離を取得（デフォルトは敵と同じ 4.0セル = 160px）
        float weaponRange = 160.0f;
        if (!weapons.empty() && weapons[currentWeaponIndex] && weapons[currentWeaponIndex]->GetData())
        {
            weaponRange = weapons[currentWeaponIndex]->GetData()->range;
        }

        float maxRange = weaponRange;
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
    status.TakeDamage(1);
    damageColorTimer = 30;
}

void Player::OnCollisionEnter(Collider *otherCollider)
{
    OnCollisionStay(otherCollider);
}

void Player::OnCollisionStay(Collider *otherCollider)
{
    if (!otherCollider || !otherCollider->GetOwner() || !otherCollider->GetOwner()->IsActive()) return;

    Object2D *otherObj = otherCollider->GetOwner();
    if (otherObj->GetObjectTag() == ObjectTag::Enemy)
    {
        // 敵と接触した際、無敵時間外であれば接触ダメージを受ける
        if (damageColorTimer <= 0)
        {
            TakeDamage();
        }

        // 敵との押し出し判定（しっかりと重なった時のみ判定）
        Vector2 enemyPos = otherObj->GetPosition();
        float dx = position.x - enemyPos.x;
        float dy = position.y - enemyPos.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        
        float enemyRadius = 15.0f;
        if (CircleCollider *c = dynamic_cast<CircleCollider *>(otherCollider))
        {
            enemyRadius = c->GetRadius();
        }
        float minDist = collider->GetRadius() + enemyRadius; // 20.0f + 15.0f = 35.0px (しっかり重なった時のみ)

        if (dist < minDist && dist > 0.0001f)
        {
            float overlap = minDist - dist;
            Vector2 pushDir(dx / dist, dy / dist);
            position.x += pushDir.x * (overlap * 0.5f);
            position.y += pushDir.y * (overlap * 0.5f);

            Vector2 newEnemyPos(enemyPos.x - pushDir.x * (overlap * 0.5f),
                               enemyPos.y - pushDir.y * (overlap * 0.5f));

            if (currentStage)
            {
                currentStage->ResolveCollision(position, 20.0f, cellSize);
                currentStage->ResolveCollision(newEnemyPos, enemyRadius, cellSize);
            }

            otherObj->SetPosition(newEnemyPos);
        }
    }
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

    float flashPixelX = 0.0f;
    float flashPixelY = 0.0f;
    bool hasFlash = false;
    float flashProgress = 0.0f;
    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (scene && scene->GetEffectManager())
    {
        int flashTimer = scene->GetEffectManager()->GetGunFlashTimer();
        if (flashTimer > 0)
        {
            hasFlash = true;
            flashProgress = (flashTimer / 5.0f);
            flashPixelX = Camera::WorldToScreenX(scene->GetEffectManager()->GetLastFlashWorldX());
            flashPixelY = Camera::WorldToScreenY(scene->GetEffectManager()->GetLastFlashWorldY());
        }
    }

    // 明るい時間(月明かり: 15秒 = 900フレーム)と暗い時間(黒暗闇: 25秒 = 1500フレーム)の切り替え管理
    m_flickerTimer--;
    if (m_flickerTimer <= 0)
    {
        // 状態を反転
        m_isMoonlightFlicker = !m_isMoonlightFlicker;

        // 明るい時間（月明かり状態）になったら15秒間（900フレーム）維持
        // 暗い時間（デフォルトの黒暗闇状態）になったら25秒間（1500フレーム）維持
        if (m_isMoonlightFlicker)
        {
            m_flickerTimer = 900;  // 60fps x 15秒 = 900フレーム
        }
        else
        {
            m_flickerTimer = 1500; // 60fps x 25秒 = 1500フレーム
        }
    }

    // デフォルト: 真っ暗な黒 (alpha=245, RGB=4,5,10)
    // チラつき時: 前より少し暗めの月明かり (alpha=215, RGB=8,12,25)
    int maxAlpha = m_isMoonlightFlicker ? 215 : 245;
    unsigned int ambientColor = m_isMoonlightFlicker ? GetColor(8, 12, 25) : GetColor(4, 5, 10);

    for (int py = rectY; py < rectY + rectH; py += resolutionStep)
    {
        for (int px = rectX; px < rectX + rectW; px += resolutionStep)
        {
            float dx = px - playerPixelX;
            float dy = py - playerPixelY;
            float dist = std::sqrt(dx * dx + dy * dy);

            float lightVal = 0.0f;

            // 撃った人の位置の周りのみの局所的フラッシュ照射
            if (hasFlash)
            {
                float fdx = px - flashPixelX;
                float fdy = py - flashPixelY;
                float fDist = std::sqrt(fdx * fdx + fdy * fdy);
                float flashRadius = zoomCellSize * 4.5f;
                if (fDist < flashRadius)
                {
                    float fLight = (1.0f - (fDist / flashRadius)) * flashProgress * 0.70f;
                    lightVal = (std::max)(lightVal, fLight);
                }
            }

            // A. プレイヤー周囲の足元明かり
            if (dist < closeRadius)
            {
                float ambientLight = 0.38f * (1.0f - (dist / closeRadius) * 0.6f);
                lightVal = (std::max)(lightVal, ambientLight);
            }

            // B. 前方60°スポットライト
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
                int alpha = static_cast<int>((1.0f - (std::min)(1.0f, lightVal)) * maxAlpha);
                if (alpha > 8)
                {
                    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
                    DrawBox(px, py, px + resolutionStep, py + resolutionStep, ambientColor, TRUE);
                }
            }
        }
    }

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void Player::RenderBloodSplatterOverlay(int screenWidth, int screenHeight) const
{
    if (!GameSettings::GetInstance().isBloodSplatterEnabled) return;

    int currentHp = status.GetCurrentHp();
    if (currentHp > 3) return;

    // HP残量に応じた血の濃さ・透過度の決定 (HP3:やや薄め, HP2:濃いめ, HP1:非常に濃い)
    int alpha = (currentHp == 3) ? 90 : (currentHp == 2) ? 140 : 190;
    
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

    // 1. 画面四隅・フチの血溜まりビネット（段階的レイヤーグラデーション）
    int edgeWidth = (currentHp == 1) ? 160 : 110;
    for (int i = 0; i < 4; ++i)
    {
        int inset = i * (edgeWidth / 4);
        int stepAlpha = alpha / (i + 1);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, stepAlpha);

        // 上・下・左・右の画面端の血溜まりボックス
        DrawBox(0, 0 + inset, screenWidth, 30 + inset, GetColor(120, 10, 10), TRUE);
        DrawBox(0, screenHeight - 30 - inset, screenWidth, screenHeight, GetColor(120, 10, 10), TRUE);
        DrawBox(0 + inset, 0, 30 + inset, screenHeight, GetColor(120, 10, 10), TRUE);
        DrawBox(screenWidth - 30 - inset, 0, screenWidth, screenHeight, GetColor(120, 10, 10), TRUE);
    }

    // 2. 画面四隅の大きめな血の滲み・雫
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    DrawOval(60, 60, 140, 100, GetColor(140, 5, 5), TRUE);
    DrawOval(screenWidth - 60, 60, 150, 110, GetColor(130, 0, 0), TRUE);
    DrawOval(60, screenHeight - 60, 160, 120, GetColor(150, 10, 10), TRUE);
    DrawOval(screenWidth - 60, screenHeight - 60, 170, 130, GetColor(120, 0, 0), TRUE);

    // 3. 画面上に散らばる不規則な血飛沫（ドット・大小の円・雫）
    // 固定シード値で生成することで毎フレームランダムに動いてチラつくのを防ぎ、画面上に固着したリアルな血飛沫を表現
    struct BloodDrop { int x, y, r; };
    static const BloodDrop drops[] = {
        // 画面上部〜左上
        { 120, 80, 18 }, { 210, 45, 12 }, { 340, 95, 24 }, { 90, 180, 15 }, { 180, 240, 8 },
        // 画面右上〜右部
        { 1780, 90, 22 }, { 1650, 50, 14 }, { 1850, 210, 28 }, { 1720, 310, 10 }, { 1890, 420, 16 },
        // 画面左下〜下部
        { 80, 980, 26 }, { 190, 1020, 15 }, { 310, 940, 20 }, { 450, 1010, 12 }, { 150, 850, 9 },
        // 画面右下
        { 1820, 990, 30 }, { 1700, 1030, 18 }, { 1860, 880, 22 }, { 1620, 950, 11 }, { 1750, 800, 14 },
        // 中央寄りの飛び散り雫
        { 420, 220, 7 }, { 1500, 260, 9 }, { 380, 820, 11 }, { 1540, 810, 8 },
        { 260, 510, 6 }, { 1680, 580, 7 }, { 520, 90, 13 }, { 1400, 100, 11 }
    };

    for (const auto& drop : drops)
    {
        // 円本体
        DrawCircle(drop.x, drop.y, drop.r, GetColor(140, 10, 10), TRUE);
        // 血の滴り垂れ（下方向に伸びる小さな楕円/ライン）
        DrawOval(drop.x, drop.y + drop.r / 2, drop.r / 2, drop.r, GetColor(110, 0, 0), TRUE);
    }

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void Player::DrawUI(int screenX, int screenY)
{
    if (weapons.empty()) return;
    
    Weapon* currentWeapon = weapons[currentWeaponIndex];
    if (currentWeapon)
    {
        const WeaponData* data = currentWeapon->GetData();
        if (data && data->uiImageHandle != -1)
        {
            DrawGraph(screenX, screenY, data->uiImageHandle, TRUE);
        }
        else
        {
            DrawBox(screenX, screenY, screenX + 100, screenY + 50, GetColor(50, 50, 50), TRUE);
            DrawString(screenX + 10, screenY + 10, currentWeapon->GetName().c_str(), GetColor(255, 255, 255));
        }

        int currentAmmo = currentWeapon->GetCurrentAmmo();
        int maxAmmo = currentWeapon->GetMaxAmmo();
        char ammoText[64];
        if (currentWeapon->IsReloading()) {
            sprintf_s(ammoText, sizeof(ammoText), "Reloading...");
        } else {
            sprintf_s(ammoText, sizeof(ammoText), "Ammo: %d / %d", currentAmmo, maxAmmo);
        }
        DrawString(screenX + 10, screenY + 60, ammoText, GetColor(255, 255, 0));

        // Draw Player HP
        char hpText[64];
        snprintf(hpText, sizeof(hpText), "HP: %d / %d", status.GetCurrentHp(), status.GetMaxHp());
        DrawString(screenX + 10, screenY + 80, hpText, GetColor(100, 255, 100));
    }
}

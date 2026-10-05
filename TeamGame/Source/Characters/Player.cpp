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
#include "SniperRifle.h"
#include "SceneManager.h"
#include "Scene.h"
#include "GameSettings.h"
#include "CharacterManager.h"
#include "GameBalanceManager.h"
#include "../Skills/Skill.h"
#include "../Skills/SkillData.h"
#include <cmath>
#include <algorithm>

Player::Player(float startX, float startY)
    : Character(ObjectTag::Player, startX, startY, 35.0f), damageColorTimer(0),
      facingDir(0.0f, -1.0f)
{
    teamId = 0;
    autoPingTimer = 600;
    
    // CharacterManager(characters.csv)からパラメータを適用
    const CharacterData* data = CharacterManager::GetInstance().GetCharacterData("Player");
    if (data)
    {
        status.Init(data->maxHp, data->moveSpeed, data->attackPower);
        if (collider) collider->SetRadius(data->colliderRadius);
        radius = data->colliderRadius;
    }
    else
    {
        status.Init(10, 1.5f, 1);
    }

    collider->SetTag("Player");
    weapons.push_back(new Handgun());
    weapons.push_back(new Shotgun());
    weapons.push_back(new SniperRifle());
    currentWeaponIndex = 0;
    const SkillData* defaultSkillData = SkillDataManager::GetInstance().GetSkillData(1);
    if (defaultSkillData) {
        currentSkill = new Skill(defaultSkillData);
    } else {
        currentSkill = nullptr;
    }
}

Player::~Player()
{
    for (auto w : weapons)
        delete w;
    weapons.clear();
    
    if (currentSkill) {
        delete currentSkill;
    }
}

void Player::Update()
{
    if (invincibleTimer > 0) invincibleTimer--;
    
    // 追加: バフ・デバフのタイマー更新
    UpdateActiveEffects();

    // 追加: スキルのクールダウンタイマー更新
    if (currentSkill) {
        currentSkill->Update();
    }

    if (!isRemote)
    {
        // 懐中電灯スイッチ
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
    m_isMoving = false;
    Vector2 moveDir(0.0f, 0.0f);

    if (!isRemote)
    {
        if (m_inputType == PlayerInputType::KEYBOARD_MOUSE)
        {
            if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_LEFT) || InputManager::GetInstance().IsKeyHeld(KEY_INPUT_A))
            {
                moveDir.x -= 1.0f;
                m_isMoving = true;
            }
            if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_RIGHT) || InputManager::GetInstance().IsKeyHeld(KEY_INPUT_D))
            {
                moveDir.x += 1.0f;
                m_isMoving = true;
            }
            if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_UP) || InputManager::GetInstance().IsKeyHeld(KEY_INPUT_W))
            {
                moveDir.y -= 1.0f;
                m_isMoving = true;
            }
            if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_DOWN) || InputManager::GetInstance().IsKeyHeld(KEY_INPUT_S))
            {
                moveDir.y += 1.0f;
                m_isMoving = true;
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
            
            if (moveDir.x != 0.0f || moveDir.y != 0.0f) m_isMoving = true;
        }
    }

    if (m_isMoving)
    {
        float length = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
        if (length > 0.0001f)
        {
            float velX = (moveDir.x / length) * status.GetSpeed();
            float velY = (moveDir.y / length) * status.GetSpeed();
            
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
        if (m_inputType == PlayerInputType::KEYBOARD_MOUSE)
        {
            // 視点固定（Eキー）の処理
            bool isEKeyHeld = InputManager::GetInstance().IsKeyHeld(KEY_INPUT_E);
            bool isEKeyPressed = InputManager::GetInstance().IsKeyPressed(KEY_INPUT_E);

            if (GameSettings::GetInstance().isAimLockHoldMode)
            {
                // 長押しモード: Eキーを押している間のみ固定
                isAimLocked = isEKeyHeld;
            }
            else
            {
                // 切り替えモード: Eキーが押された瞬間にトグル
                if (isEKeyPressed && !prevAimLockKey)
                {
                    isAimLocked = !isAimLocked;
                }
            }
            prevAimLockKey = isEKeyPressed;

            // 視点固定が解除されている時のみマウス方向へ更新
            if (!isAimLocked)
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
            }

            // 武器切り替え処理 (Qキー / 1~3キー / マウスホイール)
            int wheelRot = GetMouseWheelRotVol();
            if (wheelRot > 0)
            {
                currentWeaponIndex = (currentWeaponIndex + static_cast<int>(weapons.size()) - 1) % weapons.size();
            }
            else if (wheelRot < 0)
            {
                currentWeaponIndex = (currentWeaponIndex + 1) % weapons.size();
            }

            if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_Q))
            {
                currentWeaponIndex = (currentWeaponIndex + 1) % weapons.size();
            }
            if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_1) || InputManager::GetInstance().IsKeyPressed(KEY_INPUT_NUMPAD1))
            {
                if (weapons.size() > 0) currentWeaponIndex = 0;
            }
            if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_2) || InputManager::GetInstance().IsKeyPressed(KEY_INPUT_NUMPAD2))
            {
                if (weapons.size() > 1) currentWeaponIndex = 1;
            }
            if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_3) || InputManager::GetInstance().IsKeyPressed(KEY_INPUT_NUMPAD3))
            {
                if (weapons.size() > 2) currentWeaponIndex = 2;
            }

            if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_Z) || (GetMouseInput() & MOUSE_INPUT_LEFT))
            {
                if (!weapons.empty()) weapons[currentWeaponIndex]->Fire(position, facingDir, teamId, m_isMoving ? weapons[currentWeaponIndex]->GetMoveSpreadPenalty() : 0.0f);
            }
            
            // スキル（ガジェット）の発動 (Fキー)
            if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_F))
            {
                if (currentSkill && currentSkill->CanUse(this)) {
                    currentSkill->Use(this);
                }
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
                if (!weapons.empty()) weapons[currentWeaponIndex]->Fire(position, facingDir, teamId, m_isMoving ? weapons[currentWeaponIndex]->GetMoveSpreadPenalty() : 0.0f);
            }
            
            if (padState & PAD_INPUT_3) // Button X
            {
                if (currentSkill && currentSkill->CanUse(this)) {
                    currentSkill->Use(this);
                }
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
                if (enemy && enemy->IsActive() && enemy->teamId != this->teamId)
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

    // プレイヤー頭上に現在装備中の武器名を表示（自キャラの円と重ならない位置に上移動）
    if (!weapons.empty() && weapons[currentWeaponIndex]) {
        std::string wName = weapons[currentWeaponIndex]->GetName();
        if (isAimLocked) wName += " [視点固定]";
        DrawString(static_cast<int>(screenX) - 35, static_cast<int>(screenY) - static_cast<int>(radius) - 22, wName.c_str(), isAimLocked ? GetColor(255, 220, 0) : GetColor(200, 220, 255));
    }

    // 【サーチ的な感じでブレ幅（予測線）を描画】
    if (currentStage && cellSize > 0.0f)
    {
        float zoomCellSize = 75.0f;
        float maxRange = cellSize * 12.0f;
        if (!weapons.empty() && weapons[currentWeaponIndex]->GetData()) {
            maxRange = weapons[currentWeaponIndex]->GetData()->range;
        }
        float stepDist = cellSize * 0.4f;
        
        // 現在の武器のブレ幅を取得
        float spreadAngle = 0.0f;
        if (!weapons.empty() && weapons[currentWeaponIndex]->GetData()) {
            spreadAngle = weapons[currentWeaponIndex]->GetData()->spreadAngle;
        }
        // 歩行中ならブレ幅が広がる
        if (m_isMoving && !weapons.empty()) {
            spreadAngle += weapons[currentWeaponIndex]->GetMoveSpreadPenalty();
        }
        
        float halfSpreadRad = (spreadAngle / 2.0f) * (3.14159f / 180.0f);
        float baseAngle = std::atan2(ny, nx);
        
        // 描画用の関数内ラムダ（指定角度に向けてレイキャストして線を描画する）
        auto DrawTrajectory = [&](float angle, int colorR, int colorG, int colorB, int alpha) {
            float dirX = std::cos(angle);
            float dirY = std::sin(angle);
            float currDist = radius + 5.0f;
            Vector2 hitPos = Vector2(position.x + dirX * maxRange, position.y + dirY * maxRange);

            while (currDist < maxRange)
            {
                float testX = position.x + dirX * currDist;
                float testY = position.y + dirY * currDist;
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

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
            DrawLine(static_cast<int>(screenX), static_cast<int>(screenY),
                     static_cast<int>(hitScreenX), static_cast<int>(hitScreenY),
                     GetColor(colorR, colorG, colorB), 2);
            DrawCircle(static_cast<int>(hitScreenX), static_cast<int>(hitScreenY), 4, GetColor(colorR, colorG, colorB), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        };

        // 1. 中央の線（少し薄め）
        DrawTrajectory(baseAngle, 255, 100, 100, 100);
        
        // 2. ブレの最大幅を示す左右のサーチ線（濃いめ）
        if (spreadAngle > 0.0f) {
            DrawTrajectory(baseAngle + halfSpreadRad, 255, 50, 50, 180);
            DrawTrajectory(baseAngle - halfSpreadRad, 255, 50, 50, 180);
            
            // 扇形を塗って「サーチ範囲」っぽくする演出
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 30); // とても薄い赤
            float hitLeftX = Camera::WorldToScreenX(position.x + std::cos(baseAngle - halfSpreadRad) * (maxRange * 0.8f));
            float hitLeftY = Camera::WorldToScreenY(position.y + std::sin(baseAngle - halfSpreadRad) * (maxRange * 0.8f));
            float hitRightX = Camera::WorldToScreenX(position.x + std::cos(baseAngle + halfSpreadRad) * (maxRange * 0.8f));
            float hitRightY = Camera::WorldToScreenY(position.y + std::sin(baseAngle + halfSpreadRad) * (maxRange * 0.8f));
            DrawTriangle(
                static_cast<int>(screenX), static_cast<int>(screenY),
                static_cast<int>(hitLeftX), static_cast<int>(hitLeftY),
                static_cast<int>(hitRightX), static_cast<int>(hitRightY),
                GetColor(255, 50, 50), TRUE
            );
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
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
            if (enemy && enemy->IsActive() && enemy->teamId != this->teamId && !enemy->IsAlerted())
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

void Player::RenderBloodSplatterOverlay(int screenWidth, int screenHeight) const
{
    if (!IsActive()) return;
    int lowHpThreshold = GameBalanceManager::GetInstance().GetInt("LowHpThreshold", 3);
    if (status.GetCurrentHp() > lowHpThreshold) return;

    // HP 3 以下の時に画面外枠にパルス赤枠エフェクトを描画
    static float pulseTimer = 0.0f;
    pulseTimer += 0.08f;
    float pulse = (std::sin(pulseTimer) + 1.0f) * 0.5f; // 0.0 ~ 1.0

    int hpGap = 4 - status.GetCurrentHp(); // 1(HP=3), 2(HP=2), 3(HP=1)
    int baseAlpha = 70 + hpGap * 35;
    int currentAlpha = static_cast<int>(baseAlpha + pulse * 60);
    if (currentAlpha > 240) currentAlpha = 240;

    int borderSteps = 6;
    for (int i = 0; i < borderSteps; ++i)
    {
        int bandThickness = (borderSteps - i) * 10;
        int stepAlpha = static_cast<int>(currentAlpha * (0.3f + 0.7f * (float)(borderSteps - i) / borderSteps));

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, stepAlpha);
        // 上
        DrawBox(0, 0, screenWidth, bandThickness, GetColor(180, 0, 0), TRUE);
        // 下
        DrawBox(0, screenHeight - bandThickness, screenWidth, screenHeight, GetColor(180, 0, 0), TRUE);
        // 左
        DrawBox(0, 0, bandThickness, screenHeight, GetColor(180, 0, 0), TRUE);
        // 右
        DrawBox(screenWidth - bandThickness, 0, screenWidth, screenHeight, GetColor(180, 0, 0), TRUE);
    }

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(currentAlpha * 0.95f));
    DrawBox(0, 0, screenWidth, screenHeight, GetColor(255, 30, 30), FALSE);
    DrawBox(3, 3, screenWidth - 3, screenHeight - 3, GetColor(220, 10, 10), FALSE);

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(160 + pulse * 85));
    DrawString(screenWidth / 2 - 80, 30, "!! LOW HP DANGER !!", GetColor(255, 60, 60));
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void Player::TakeDamage(int amount)
{
    if (invincibleTimer > 0) return;
    status.TakeDamage(amount);
    damageColorTimer = 30;
    
    if (status.IsDead())
    {
        SetActive(false);
        // ここに将来的にゲームオーバー画面（ResultScene）への遷移を追加します
    }
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

void Player::DrawUI(int screenX, int screenY)
{
    if (weapons.empty()) return;
    
    // UI背景の半透明黒ボックスを描画（視認性を向上させて文字の重なりを解消）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
    DrawBox(screenX - 10, screenY - 10, screenX + 220, screenY + 130, GetColor(0, 0, 0), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(screenX - 10, screenY - 10, screenX + 220, screenY + 130, GetColor(100, 100, 100), FALSE);

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
            char weaponTitle[64];
            snprintf(weaponTitle, sizeof(weaponTitle), "[%d] %s", currentWeaponIndex + 1, currentWeapon->GetName().c_str());
            DrawBox(screenX, screenY, screenX + 140, screenY + 35, GetColor(40, 40, 60), TRUE);
            DrawBox(screenX, screenY, screenX + 140, screenY + 35, GetColor(0, 200, 255), FALSE);
            DrawString(screenX + 8, screenY + 8, weaponTitle, GetColor(255, 255, 255));
        }

        int currentAmmo = currentWeapon->GetCurrentAmmo();
        int maxAmmo = currentWeapon->GetMaxAmmo();
        char ammoText[64];
        if (currentWeapon->IsReloading()) {
            sprintf_s(ammoText, sizeof(ammoText), "Reloading...");
        } else {
            sprintf_s(ammoText, sizeof(ammoText), "Ammo: %d / %d", currentAmmo, maxAmmo);
        }
        DrawString(screenX + 10, screenY + 50, ammoText, GetColor(255, 255, 0));

        // HP表示
        char hpText[64];
        snprintf(hpText, sizeof(hpText), "HP: %d / %d", status.GetCurrentHp(), status.GetMaxHp());
        DrawString(screenX + 10, screenY + 75, hpText, GetColor(100, 255, 100));

        // スキルUI表示
        if (currentSkill)
        {
            char skillText[64];
            int ct = currentSkill->GetCoolTimeTimer();
            if (ct > 0) {
                snprintf(skillText, sizeof(skillText), "Skill: CT %d.%.1fs", ct / 60, (ct % 60) / 6.0f);
                DrawString(screenX + 10, screenY + 100, skillText, GetColor(200, 200, 200));
            } else {
                snprintf(skillText, sizeof(skillText), "Skill: READY");
                DrawString(screenX + 10, screenY + 100, skillText, GetColor(0, 220, 255));
            }
        }
    }
}



void Player::AddAmmo(int amount)
{
    if (!weapons.empty() && weapons[currentWeaponIndex])
    {
        weapons[currentWeaponIndex]->AddAmmo(amount);
    }
}

#include "Camera.h"
#include "FontManager.h"
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
#include "../Managers/SkillManager.h"
#include <cmath>
#include <algorithm>

Player::Player(float startX, float startY)
    : Character(ObjectTag::Player, startX, startY, 8.9f), damageColorTimer(0),
      facingDir(0.0f, -1.0f)
{
    teamId = 0;
    autoPingTimer = 600;
    
    // CharacterManager(characters.csv)からパラメータを適用
    const CharacterData* data = CharacterManager::GetInstance().GetCharacterData("Player");
    if (!data) data = CharacterManager::GetInstance().GetCharacterDataById(1);

    if (data)
    {
        status.Init(data->maxHp, data->moveSpeed, data->attackPower);
        if (collider) collider->SetRadius(data->colliderRadius);
        radius = data->colliderRadius;
    }
    else
    {
        status.Init(33, 4.2f, 10);
        if (collider) collider->SetRadius(8.9f);
        radius = 8.9f;
    }

    collider->SetTag("Player");
    weapons.push_back(new Handgun());
    weapons.push_back(new Shotgun());
    weapons.push_back(new SniperRifle());
    currentWeaponIndex = 0;
    SetEquippedSkillById(1);
    SetModelPath("Resource/GreenCharacter/character.mv1");
    
}

Player::~Player()
{
    for (auto w : weapons)
        delete w;
    weapons.clear();
}

void Player::SetWeapons(const std::vector<Weapon*>& newWeapons)
{
    for (auto w : weapons)
        delete w;
    weapons.clear();

    weapons = newWeapons;
    currentWeaponIndex = 0;
}


void Player::Update()
{
    Character::Update();
    if (!CanAction()) return;

    if (invincibleTimer > 0) invincibleTimer--;

    if (!isRemote)
    {
        if (m_inputType == PlayerInputType::KEYBOARD_MOUSE)
        {
            // 懐中電灯スイッチ (右クリック)
            bool currMouseRight = ((GetMouseInput() & MOUSE_INPUT_RIGHT) != 0);
            if (currMouseRight && !m_prevMouseRight)
            {
                m_isLightOn = !m_isLightOn;
            }
            m_prevMouseRight = currMouseRight;
        }
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
            // 1P: WASDキー移動
            if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_A)) { moveDir.x -= 1.0f; m_isMoving = true; }
            if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_D)) { moveDir.x += 1.0f; m_isMoving = true; }
            if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_W)) { moveDir.y -= 1.0f; m_isMoving = true; }
            if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_S)) { moveDir.y += 1.0f; m_isMoving = true; }

            // 1P: ゲームパッド1が接続されている場合はパッドでもスティック/十字キー移動可能 (ハイブリッド)
            int ax = 0, ay = 0;
            GetJoypadAnalogInput(&ax, &ay, DX_INPUT_PAD1);
            if (ax < -200) { moveDir.x -= 1.0f; m_isMoving = true; }
            if (ax > 200)  { moveDir.x += 1.0f; m_isMoving = true; }
            if (ay < -200) { moveDir.y -= 1.0f; m_isMoving = true; }
            if (ay > 200)  { moveDir.y += 1.0f; m_isMoving = true; }
            int padState = GetJoypadInputState(DX_INPUT_PAD1);
            if (padState & PAD_INPUT_LEFT)  { moveDir.x -= 1.0f; m_isMoving = true; }
            if (padState & PAD_INPUT_RIGHT) { moveDir.x += 1.0f; m_isMoving = true; }
            if (padState & PAD_INPUT_UP)    { moveDir.y -= 1.0f; m_isMoving = true; }
            if (padState & PAD_INPUT_DOWN)  { moveDir.y += 1.0f; m_isMoving = true; }
        }
        else // GAMEPAD_1 または GAMEPAD_2
        {
            int padIdx = (m_inputType == PlayerInputType::GAMEPAD_2) ? DX_INPUT_PAD2 : DX_INPUT_PAD1;
            // 2P: ゲームパッド入力 (PAD1 または PAD2)
            int ax = 0, ay = 0;
            GetJoypadAnalogInput(&ax, &ay, padIdx);
            if (ax < -200) { moveDir.x -= 1.0f; m_isMoving = true; }
            if (ax > 200)  { moveDir.x += 1.0f; m_isMoving = true; }
            if (ay < -200) { moveDir.y -= 1.0f; m_isMoving = true; }
            if (ay > 200)  { moveDir.y += 1.0f; m_isMoving = true; }
            
            int padState = GetJoypadInputState(padIdx);
            if (padState & PAD_INPUT_LEFT)  { moveDir.x -= 1.0f; m_isMoving = true; }
            if (padState & PAD_INPUT_RIGHT) { moveDir.x += 1.0f; m_isMoving = true; }
            if (padState & PAD_INPUT_UP)    { moveDir.y -= 1.0f; m_isMoving = true; }
            if (padState & PAD_INPUT_DOWN)  { moveDir.y += 1.0f; m_isMoving = true; }
            
            // 2P: ゲームパッドがない場合でも矢印キーで移動可能
            if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_LEFT))  { moveDir.x -= 1.0f; m_isMoving = true; }
            if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_RIGHT)) { moveDir.x += 1.0f; m_isMoving = true; }
            if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_UP))    { moveDir.y -= 1.0f; m_isMoving = true; }
            if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_DOWN))  { moveDir.y += 1.0f; m_isMoving = true; }
        }
    }

    if (m_isMoving)
    {
        float length = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
        if (length > 0.0001f)
        {
            float velX = (moveDir.x / length) * status.GetSpeed();
            float velY = (moveDir.y / length) * status.GetSpeed();

            // 周囲の敵キャラクターの座標と当たり判定半径を取得
            std::vector<std::pair<Vector2, float>> enemyInfos;
            auto scene = SceneManager::GetInstance().GetCurrentScene();
            if (scene && scene->GetObjectManager())
            {
                for (auto obj : scene->GetObjectManager()->GetObjects())
                {
                    if (obj && obj->IsActive() && obj->GetObjectTag() == ObjectTag::Enemy)
                    {
                        Character *enemyChara = dynamic_cast<Character *>(obj);
                        float eRad = enemyChara ? enemyChara->GetRadius() : radius;
                        enemyInfos.push_back({obj->GetPosition(), eRad});
                    }
                }
            }

            // X軸の移動と衝突判定（壁および敵接触）
            float nextX = position.x + velX;
            bool blockedX = false;
            if (currentStage)
            {
                int gridX = static_cast<int>(nextX / cellSize);
                int gridY = static_cast<int>(position.y / cellSize);
                if (currentStage->IsSolidWall(gridX, gridY))
                {
                    blockedX = true;
                }
            }
            if (!blockedX)
            {
                // 敵に触れている場合、敵の方向（距離が縮まる方向）への移動をブロック
                for (const auto &info : enemyInfos)
                {
                    float minDist = radius + info.second;
                    float curDx = info.first.x - position.x;
                    float curDy = info.first.y - position.y;
                    float curDistSq = curDx * curDx + curDy * curDy;

                    float nextDx = info.first.x - nextX;
                    float nextDy = info.first.y - position.y;
                    float nextDistSq = nextDx * nextDx + nextDy * nextDy;

                    if (nextDistSq < minDist * minDist && nextDistSq < curDistSq)
                    {
                        blockedX = true;
                        break;
                    }
                }
            }
            if (!blockedX)
            {
                position.x = nextX;
            }

            // Y軸の移動と衝突判定（壁および敵接触）
            float nextY = position.y + velY;
            bool blockedY = false;
            if (currentStage)
            {
                int gridX = static_cast<int>(position.x / cellSize);
                int gridY = static_cast<int>(nextY / cellSize);
                if (currentStage->IsSolidWall(gridX, gridY))
                {
                    blockedY = true;
                }
            }
            if (!blockedY)
            {
                // 敵に触れている場合、敵の方向（距離が縮まる方向）への移動をブロック
                for (const auto &info : enemyInfos)
                {
                    float minDist = radius + info.second;
                    float curDx = info.first.x - position.x;
                    float curDy = info.first.y - position.y;
                    float curDistSq = curDx * curDx + curDy * curDy;

                    float nextDx = info.first.x - position.x;
                    float nextDy = info.first.y - nextY;
                    float nextDistSq = nextDx * nextDx + nextDy * nextDy;

                    if (nextDistSq < minDist * minDist && nextDistSq < curDistSq)
                    {
                        blockedY = true;
                        break;
                    }
                }
            }
            if (!blockedY)
            {
                position.y = nextY;
            }
        }
    }

    if (damageColorTimer > 0)
    {
        damageColorTimer--;
    }

    // 3Dモデルのモーション制御（歩行・待機）
    if (m_model)
    {
        if (m_isMoving)
        {
            m_model->PlayWalk();
        }
        else
        {
            m_model->PlayIdle();
        }
        m_model->Update(1.0f / 60.0f);
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
                if (m_model) m_model->PlayShoot();
            }
            
            // スキル（ガジェット）の発動 (Fキー)
            if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_F))
            {
                SkillManager::GetInstance().TriggerSkill(this);
            }
        }
        else // GAMEPAD_1 または GAMEPAD_2
        {
            int padIdx = (m_inputType == PlayerInputType::GAMEPAD_2) ? DX_INPUT_PAD2 : DX_INPUT_PAD1;
            bool hasAimInput = false;

            // 右アナログスティック入力
            int rx = 0, ry = 0;
            GetJoypadAnalogInputRight(&rx, &ry, padIdx);
            if (rx < -200 || rx > 200 || ry < -200 || ry > 200)
            {
                float dx = (float)rx;
                float dy = (float)ry;
                float dirLen = std::sqrt(dx * dx + dy * dy);
                if (dirLen > 0.0001f)
                {
                    facingDir.x = dx / dirLen;
                    facingDir.y = dy / dirLen;
                    hasAimInput = true;
                }
            }
            
            // 2Pキーボードエイム（テンキーまたはIJKL）
            float keyAimX = 0.0f, keyAimY = 0.0f;
            if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_NUMPAD4) || InputManager::GetInstance().IsKeyHeld(KEY_INPUT_J)) keyAimX -= 1.0f;
            if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_NUMPAD6) || InputManager::GetInstance().IsKeyHeld(KEY_INPUT_L)) keyAimX += 1.0f;
            if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_NUMPAD8) || InputManager::GetInstance().IsKeyHeld(KEY_INPUT_I)) keyAimY -= 1.0f;
            if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_NUMPAD2) || InputManager::GetInstance().IsKeyHeld(KEY_INPUT_K)) keyAimY += 1.0f;
            if (keyAimX != 0.0f || keyAimY != 0.0f)
            {
                float len = std::sqrt(keyAimX * keyAimX + keyAimY * keyAimY);
                facingDir.x = keyAimX / len;
                facingDir.y = keyAimY / len;
                hasAimInput = true;
            }
            else if (!hasAimInput && m_isMoving)
            {
                float len = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
                if (len > 0.0001f)
                {
                    facingDir.x = moveDir.x / len;
                    facingDir.y = moveDir.y / len;
                }
            }

            // ★ 強力オートエイム（敵吸い付きアシスト）
            UpdateAutoAim(moveDir);
            
            int padState = GetJoypadInputState(padIdx);
            static int prevPadState = 0;
            
            // 武器切り替え: パッド Button 6(LB) / Button 3(X) または キーボード P / NumPad 7
            bool isWeaponChangePressed = ((padState & PAD_INPUT_6) && !(prevPadState & PAD_INPUT_6)) ||
                                         InputManager::GetInstance().IsKeyPressed(KEY_INPUT_P) ||
                                         InputManager::GetInstance().IsKeyPressed(KEY_INPUT_NUMPAD7);
            if (isWeaponChangePressed)
            {
                currentWeaponIndex = (currentWeaponIndex + 1) % weapons.size();
            }

            // 懐中電灯ON/OFF: パッド Button 4(Y) または キーボード セミコロン / NumPad 5
            bool isLightTogglePressed = ((padState & PAD_INPUT_4) && !(prevPadState & PAD_INPUT_4)) ||
                                        InputManager::GetInstance().IsKeyPressed(KEY_INPUT_SEMICOLON) ||
                                        InputManager::GetInstance().IsKeyPressed(KEY_INPUT_NUMPAD5);
            if (isLightTogglePressed)
            {
                ToggleLight();
            }
            
            // 射撃: パッド Button 1(A) / RT または キーボード Right Ctrl / Slash / NumPad 0
            bool isShootHeld = (padState & PAD_INPUT_1) ||
                               InputManager::GetInstance().IsKeyHeld(KEY_INPUT_RCONTROL) ||
                               InputManager::GetInstance().IsKeyHeld(KEY_INPUT_SLASH) ||
                               InputManager::GetInstance().IsKeyHeld(KEY_INPUT_NUMPAD0);
            if (isShootHeld)
            {
                if (!weapons.empty() && currentWeaponIndex >= 0 && currentWeaponIndex < static_cast<int>(weapons.size()) && weapons[currentWeaponIndex]) 
                    weapons[currentWeaponIndex]->Fire(position, facingDir, teamId, m_isMoving ? weapons[currentWeaponIndex]->GetMoveSpreadPenalty() : 0.0f);
                if (m_model) m_model->PlayShoot();
            }
            
            // スキル: パッド Button 8(RT) / Button 7 または キーボード U / NumPad 9
            bool isSkillPressed = ((padState & PAD_INPUT_8) && !(prevPadState & PAD_INPUT_8)) ||
                                  InputManager::GetInstance().IsKeyPressed(KEY_INPUT_U) ||
                                  InputManager::GetInstance().IsKeyPressed(KEY_INPUT_NUMPAD9);
            if (isSkillPressed)
            {
                SkillManager::GetInstance().TriggerSkill(this);
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
        else {
            int padIdx = (m_inputType == PlayerInputType::GAMEPAD_2) ? DX_INPUT_PAD2 : DX_INPUT_PAD1;
            int pad = GetJoypadInputState(padIdx);
            isKnifePressed = ((pad & PAD_INPUT_2) != 0) ||
                             InputManager::GetInstance().IsKeyPressed(KEY_INPUT_RSHIFT) ||
                             InputManager::GetInstance().IsKeyPressed(KEY_INPUT_NUMPAD3);
            isReloadPressed = ((pad & PAD_INPUT_5) != 0) ||
                              InputManager::GetInstance().IsKeyPressed(KEY_INPUT_PERIOD) ||
                              InputManager::GetInstance().IsKeyPressed(KEY_INPUT_O) ||
                              InputManager::GetInstance().IsKeyPressed(KEY_INPUT_NUMPAD1);
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
        (damageColorTimer > 0) ? GetColor(255, 255, 0) : m_themeColor;

    float renderRadius = radius * Camera::ZoomScale;

    // 3Dモデルが存在する場合はモデル描画と足元ガイド円を描画
    if (m_model && m_model->IsLoaded())
    {
        // 足元にチームカラーのガイド円を描画（半透明）
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);
        DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
                   static_cast<int>(renderRadius), color, TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
                   static_cast<int>(renderRadius), GetColor(255, 255, 255), FALSE);

        // 3Dモデルを描画 (2Dスクリーン座標に合わせて配置・回転)
        m_model->SetScreenPosition(screenX, screenY);
        m_model->SetFacingDir(facingDir);
        m_model->SetScaleUniform(m_modelScale * (Camera::ZoomScale / 2.0f));
        float playerOpacity = IsTransparent() ? 0.35f : 1.0f;
        m_model->DrawWithOpacity(playerOpacity);

        // HighRisk（悪魔）発動中: 「ロウソクで縁取られている」演出（揺らめく赤・橙・黄色の炎の輪とロウソクの火の粉）
        if (IsHighRisk())
        {
            float timeSec = GetNowCount() / 1000.0f;
            for (int k = 0; k < 8; ++k)
            {
                float ang = (k / 8.0f) * 6.283185f + timeSec * 1.5f;
                float flicker = std::sin(timeSec * 10.0f + k * 1.3f) * 4.0f;
                float rDist = radius + 14.0f + flicker;
                float fx = screenX + std::cos(ang) * rDist;
                float fy = screenY + std::sin(ang) * rDist;

                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 140);
                DrawCircle(static_cast<int>(fx), static_cast<int>(fy), 7, GetColor(255, 80, 20), TRUE);  // 炎外縁（赤橙）
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 210);
                DrawCircle(static_cast<int>(fx), static_cast<int>(fy), 4, GetColor(255, 220, 50), TRUE); // 炎中心核（黄・白）
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            }
            // プレイヤー外周のロウソク光彩リング
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(80 + std::sin(timeSec * 8.0f) * 30));
            DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY), static_cast<int>(radius + 20), GetColor(255, 120, 30), FALSE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
    }
    else
    {
        // 3Dモデルがない場合の2Dフォールバック描画
        DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
                   static_cast<int>(renderRadius), color, TRUE);
    }

    // 向いている方角の単位ベクトル
    float dirLen = std::sqrt(facingDir.x * facingDir.x + facingDir.y * facingDir.y);
    float nx = (dirLen > 0.0001f) ? (facingDir.x / dirLen) : facingDir.x;
    float ny = (dirLen > 0.0001f) ? (facingDir.y / dirLen) : facingDir.y;

    // 銃身（ガンバレル）の描画: 装備中の武器固有の描画アセットを描画
    if (!weapons.empty() && weapons[currentWeaponIndex])
    {
        weapons[currentWeaponIndex]->DrawBarrel(screenX, screenY, facingDir);
    }
    else
    {
        // 向いている方角を線で描画
        float lineLen = 50.0f; // 線の長さ
        int x1 = static_cast<int>(screenX);
        int y1 = static_cast<int>(screenY);
        int x2 = static_cast<int>(screenX + nx * lineLen);
        int y2 = static_cast<int>(screenY + ny * lineLen);
        DrawLine(x1, y1, x2, y2, GetColor(255, 255, 255), 2);
    }

    // プレイヤー頭上に現在装備中の武器名を表示（自キャラの円と重ならない位置に上移動）
    if (!weapons.empty() && weapons[currentWeaponIndex]) {
        std::string wName = weapons[currentWeaponIndex]->GetName();
        if (isAimLocked) wName += " [視点固定]";
        DrawString(static_cast<int>(screenX) - 35, static_cast<int>(screenY) - static_cast<int>(renderRadius) - 22, wName.c_str(), isAimLocked ? GetColor(255, 220, 0) : GetColor(200, 220, 255));
    }

    // 【サーチ的な感じでブレ幅（予測線）を描画】
    if (currentStage && cellSize > 0.0f)
    {
        float zoomCellSize = cellSize * Camera::ZoomScale;
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
            float zoomScale = Camera::ZoomScale;
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

        // 悪魔のHighRisk敵位置探知インジケータ（暗闇より優先して最前面に描画するため、RenderHighRiskEnemyIndicatorsで描画）
    }

    if (m_isInBush)
    {
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}

void Player::RenderHighRiskEnemyIndicators() const
{
    if (!IsActive() || !IsHighRisk()) return;

    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (!scene || !scene->GetObjectManager()) return;

    for (auto obj : scene->GetObjectManager()->GetObjects())
    {
        Enemy* enemy = dynamic_cast<Enemy*>(obj);
        if (enemy && enemy->IsActive() && enemy->teamId != this->teamId)
        {
            Vector2 ePos = enemy->GetPosition();
            float eScreenX = Camera::WorldToScreenX(ePos.x);
            float eScreenY = Camera::WorldToScreenY(ePos.y);

            float dx = ePos.x - position.x;
            float dy = ePos.y - position.y;
            float distPx = std::sqrt(dx * dx + dy * dy);
            float distMeter = distPx / 40.0f; // 40px = 1m換算

            // 敵が画面外にある場合は画面端に追跡矢印を配置、画面内なら敵の直上に表示
            float arrowScreenX = eScreenX;
            float arrowScreenY = eScreenY;
            bool isOffScreen = false;

            float margin = 80.0f;
            if (arrowScreenX < margin || arrowScreenX > 1920.0f - margin ||
                arrowScreenY < margin || arrowScreenY > 1080.0f - margin)
            {
                isOffScreen = true;
                float dirX = dx / (distPx > 0.001f ? distPx : 1.0f);
                float dirY = dy / (distPx > 0.001f ? distPx : 1.0f);
                // 画面中心から画面端へ投影
                float cScreenX = 1920.0f / 2.0f;
                float cScreenY = 1080.0f / 2.0f;
                arrowScreenX = cScreenX + dirX * 420.0f;
                arrowScreenY = cScreenY + dirY * 420.0f;
            }

            // 1. 敵頭上/画面端インジケータ（矢印 ＋ 「〇.〇m」距離表示）
            float angleToEnemy = std::atan2(dy, dx);
            float tipX = arrowScreenX;
            float tipY = isOffScreen ? arrowScreenY : (eScreenY - 35.0f);

            // 矢印（三角形）の描画（暗闇に埋もれないよう黒縁取り＋鮮烈な赤色）
            float arrowLen = 16.0f;
            float p1X = tipX + std::cos(angleToEnemy) * arrowLen;
            float p1Y = tipY + std::sin(angleToEnemy) * arrowLen;
            float p2X = tipX + std::cos(angleToEnemy + 2.4f) * (arrowLen * 0.8f);
            float p2Y = tipY + std::sin(angleToEnemy + 2.4f) * (arrowLen * 0.8f);
            float p3X = tipX + std::cos(angleToEnemy - 2.4f) * (arrowLen * 0.8f);
            float p3Y = tipY + std::sin(angleToEnemy - 2.4f) * (arrowLen * 0.8f);

            // 矢印の外枠（黒縁取りで暗闇でも明瞭に輪郭を浮き立たせる）
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
            DrawTriangle(static_cast<int>(p1X), static_cast<int>(p1Y),
                         static_cast<int>(p2X), static_cast<int>(p2Y),
                         static_cast<int>(p3X), static_cast<int>(p3Y),
                         GetColor(0, 0, 0), FALSE);

            // 矢印本体塗りつぶし（ビビッドな赤）
            DrawTriangle(static_cast<int>(p1X), static_cast<int>(p1Y),
                         static_cast<int>(p2X), static_cast<int>(p2Y),
                         static_cast<int>(p3X), static_cast<int>(p3Y),
                         GetColor(255, 40, 40), TRUE);

            // 距離表示「あと 〇.〇m」の背景黒帯（文字が暗闇でも確実にくっきり読めるようにする）
            char distText[32];
            snprintf(distText, sizeof(distText), "あと %.1fm", distMeter);
            int textW = 86;
            int textH = 18;
            int textX = static_cast<int>(tipX) - textW / 2;
            int textY = static_cast<int>(tipY) - 26;

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
            DrawBox(textX - 4, textY - 2, textX + textW, textY + textH, GetColor(0, 0, 0), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            DrawBox(textX - 4, textY - 2, textX + textW, textY + textH, GetColor(255, 220, 0), FALSE);

            // 距離テキスト（鮮やかな黄色で最前面表示）
            DrawString(textX, textY, distText, GetColor(255, 230, 40));

            // 2. 敵本体の強調サークル（画面内なら敵の足元・頭部位置を壁透過でハイライト）
            if (!isOffScreen)
            {
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
                DrawCircle(static_cast<int>(eScreenX), static_cast<int>(eScreenY), 26, GetColor(255, 30, 30), FALSE);
                DrawCircle(static_cast<int>(eScreenX), static_cast<int>(eScreenY), 24, GetColor(255, 210, 0), FALSE);
                DrawCircle(static_cast<int>(eScreenX), static_cast<int>(eScreenY), 22, GetColor(255, 50, 50), FALSE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            }
        }
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

void Player::TakeDamage(int amount, const std::string& attackerName, int attackerTeamId)
{
    if (invincibleTimer > 0) return;
    if (!attackerName.empty() || attackerTeamId != -1)
    {
        SetLastAttacker(attackerName, attackerTeamId);
    }
    status.TakeDamage(amount);
    damageColorTimer = 30;
    
    if (status.IsDead())
    {
        SetActive(false);
        if (m_model) m_model->PlayDie();
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

void Player::RenderLightMask(int rectX, int rectY, int rectW, int rectH, float startDrawX, float startDrawY, float brightRatio) const
{
    if (!currentStage || cellSize <= 0.0f) return;

    // プレイヤーの画面上での中心位置 (960, 540)
    float playerPixelX = Camera::WorldToScreenX(position.x);
    float playerPixelY = Camera::WorldToScreenY(position.y);

    float zoomCellSize = cellSize * Camera::ZoomScale;
    float zoomScale = Camera::ZoomScale;

    float maxSpotDist = zoomCellSize * m_maxSpotDistCells * GetSightMultiplier();
    float closeRadius = zoomCellSize * m_closeRadiusCells * GetSightMultiplier();

    int stageWidth = currentStage->GetWidth();
    int stageHeight = currentStage->GetHeight();

    

    const int resolutionStep = 8; // 8pxステップで高精細かつ大幅軽量化

    float lightAngle = std::atan2(facingDir.y, facingDir.x);

    // 月明かり（薄明時間帯）の環境光ブーストと最大暗闇アルファの滑らかな緩和
    float globalBrightVal = brightRatio * 0.05f;
    float maxDarkAlpha = 248.0f * (1.0f - brightRatio * 0.10f);

    float maxSpotDistSq = maxSpotDist * maxSpotDist;
    float closeRadiusSq = closeRadius * closeRadius;

    // 扇形コサイン事前計算
    float cosHalfAngle = std::cos(m_fanAngleHalf);
    float cosHalfAngleSq = cosHalfAngle * cosHalfAngle;

    unsigned int darkColor = GetColor(4, 5, 10);
    int currentBlendAlpha = -1;

    for (int py = rectY; py < rectY + rectH; py += resolutionStep)
    {
        int bottomY = (std::min)(py + resolutionStep, rectY + rectH);
        int runStartX = -1;
        int runAlpha = -1;

        for (int px = rectX; px < rectX + rectW; px += resolutionStep)
        {
            float dx = (px + resolutionStep * 0.5f) - playerPixelX;
            float dy = (py + resolutionStep * 0.5f) - playerPixelY;
            float distSq = dx * dx + dy * dy;

            float lightVal = 0.0f;

            // A. プレイヤー周囲の足元明かり
            if (distSq < closeRadiusSq)
            {
                float dist = std::sqrt(distSq);
                float ambientLight = 0.38f * (1.0f - (dist / closeRadius) * 0.6f);
                lightVal = (std::max)(lightVal, ambientLight);
            }

            // B. 前方60°扇形スポットライト
            if (m_isLightOn && distSq < maxSpotDistSq)
            {
                // 向きベクトルとの内積（前方方向への投影）
                float forwardProj = dx * facingDir.x + dy * facingDir.y;

                // 前方かつ扇形角度内（cosの比較による高速内外判定）
                if (forwardProj > 0.0f && (forwardProj * forwardProj >= cosHalfAngleSq * distSq))
                {
                    float dist = std::sqrt(distSq);
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

                        float angleCos = forwardProj / dist;
                        float angleDiff = std::acos((std::min)(1.0f, (std::max)(-1.0f, angleCos)));
                        float angleFade = 1.0f - (angleDiff / m_fanAngleHalf);
                        if (angleFade < 0.0f) angleFade = 0.0f;
                        float spotLight = distFade * angleFade * 0.95f;

                        lightVal = (std::max)(lightVal, spotLight);
                    }
                }
            }

            float effectiveLightVal = (std::max)(lightVal, globalBrightVal);

            int alpha = 0;
            if (effectiveLightVal < 0.98f)
            {
                alpha = static_cast<int>((1.0f - (std::min)(1.0f, effectiveLightVal)) * maxDarkAlpha);
                if (alpha <= 8) alpha = 0;
            }

            // アルファ値のクオンタイズ（微細な差をまとめてDrawBoxのランレングス結合率を最大化）
            int qAlpha = (alpha / 4) * 4;

            if (qAlpha != runAlpha)
            {
                if (runAlpha > 0 && runStartX >= 0)
                {
                    if (currentBlendAlpha != runAlpha)
                    {
                        SetDrawBlendMode(DX_BLENDMODE_ALPHA, runAlpha);
                        currentBlendAlpha = runAlpha;
                    }
                    DrawBox(runStartX, py, px, bottomY, darkColor, TRUE);
                }
                runStartX = px;
                runAlpha = qAlpha;
            }
        }

        // 行末のランをフラッシュ描画
        if (runAlpha > 0 && runStartX >= 0)
        {
            if (currentBlendAlpha != runAlpha)
            {
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, runAlpha);
                currentBlendAlpha = runAlpha;
            }
            DrawBox(runStartX, py, rectX + rectW, bottomY, darkColor, TRUE);
        }
    }

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // ★ 2P等のオートエイム発動中：ロックオン対象の敵にHUDターゲットレティクルを描画
    if (m_autoAimTarget && m_autoAimTarget->IsActive() && !m_autoAimTarget->status.IsDead())
    {
        Vector2 targetPos = m_autoAimTarget->GetPosition();
        float tScreenX = Camera::WorldToScreenX(targetPos.x);
        float tScreenY = Camera::WorldToScreenY(targetPos.y);

        // 画面内に存在する場合のみ描画
        if (tScreenX >= -100.0f && tScreenX <= 2020.0f && tScreenY >= -100.0f && tScreenY <= 1180.0f)
        {
            float targetR = m_autoAimTarget->GetRadius() * Camera::ZoomScale + 12.0f;
            float timeSec = GetNowCount() / 1000.0f;
            float pulse = std::sin(timeSec * 8.0f) * 3.0f;
            float drawR = targetR + pulse;

            unsigned int lockColor = GetColor(255, 60, 60);

            // ロックオンリング (赤色)
            DrawCircle(static_cast<int>(tScreenX), static_cast<int>(tScreenY), static_cast<int>(drawR), lockColor, FALSE);

            // カッコ型レティクル [  ]
            int len = 10;
            int offset = static_cast<int>(drawR);
            // 左上
            DrawLine(static_cast<int>(tScreenX - offset), static_cast<int>(tScreenY - offset), static_cast<int>(tScreenX - offset + len), static_cast<int>(tScreenY - offset), lockColor, 2);
            DrawLine(static_cast<int>(tScreenX - offset), static_cast<int>(tScreenY - offset), static_cast<int>(tScreenX - offset), static_cast<int>(tScreenY - offset + len), lockColor, 2);
            // 右上
            DrawLine(static_cast<int>(tScreenX + offset), static_cast<int>(tScreenY - offset), static_cast<int>(tScreenX + offset - len), static_cast<int>(tScreenY - offset), lockColor, 2);
            DrawLine(static_cast<int>(tScreenX + offset), static_cast<int>(tScreenY - offset), static_cast<int>(tScreenX + offset), static_cast<int>(tScreenY - offset + len), lockColor, 2);
            // 左下
            DrawLine(static_cast<int>(tScreenX - offset), static_cast<int>(tScreenY + offset), static_cast<int>(tScreenX - offset + len), static_cast<int>(tScreenY + offset), lockColor, 2);
            DrawLine(static_cast<int>(tScreenX - offset), static_cast<int>(tScreenY + offset), static_cast<int>(tScreenX - offset), static_cast<int>(tScreenY + offset - len), lockColor, 2);
            // 右下
            DrawLine(static_cast<int>(tScreenX + offset), static_cast<int>(tScreenY + offset), static_cast<int>(tScreenX + offset - len), static_cast<int>(tScreenY + offset), lockColor, 2);
            DrawLine(static_cast<int>(tScreenX + offset), static_cast<int>(tScreenY + offset), static_cast<int>(tScreenX + offset), static_cast<int>(tScreenY + offset - len), lockColor, 2);

            // LOCK-ON テキスト
            FontManager::GetInstance().DrawString(static_cast<int>(tScreenX - 32), static_cast<int>(tScreenY - offset - 18), "LOCK ON", lockColor, 16);
        }
    }
}

void Player::DrawUI(int screenX, int screenY)
{
    if (weapons.empty()) return;

    // 広々と見やすいUIパネルパラメータ (横幅320px, 高さ190px)
    const int panelW = 320;
    const int panelH = 190;
    const int marginX = 16;

    // 画面下端からはみ出さないよう座標補正
    if (screenY + panelH > 1060) {
        screenY = 1060 - panelH;
    }

    // 1. UI背景パネル（ダークブルー半透明）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 210);
    DrawBox(screenX, screenY, screenX + panelW, screenY + panelH, GetColor(12, 16, 26), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 外枠ボーダーと立体アクセント
    DrawBox(screenX, screenY, screenX + panelW, screenY + panelH, GetColor(45, 75, 110), FALSE);
    DrawBox(screenX + 1, screenY + 1, screenX + panelW - 1, screenY + panelH - 1, GetColor(20, 35, 55), FALSE);
    // 左端の青色シアンアクセントライン
    DrawBox(screenX, screenY, screenX + 5, screenY + panelH, GetColor(0, 180, 240), TRUE);

    Weapon* currentWeapon = weapons[currentWeaponIndex];
    if (!currentWeapon) return;

    int curY = screenY + 14;

    // 2. 武器ヘッダー表示
    const WeaponData* data = currentWeapon->GetData();
    if (data && data->uiImageHandle != -1)
    {
        DrawGraph(screenX + marginX, curY, data->uiImageHandle, TRUE);
        curY += 45;
    }
    else
    {
        // 武器タイトル用背景枠
        DrawBox(screenX + marginX, curY, screenX + panelW - marginX, curY + 34, GetColor(25, 40, 65), TRUE);
        DrawBox(screenX + marginX, curY, screenX + panelW - marginX, curY + 34, GetColor(0, 160, 230), FALSE);

        char weaponTitle[64];
        snprintf(weaponTitle, sizeof(weaponTitle), "SLOT [%d]  %s", currentWeaponIndex + 1, currentWeapon->GetName().c_str());

        FontManager::GetInstance().DrawString(screenX + marginX + 10, curY + 7, weaponTitle, GetColor(240, 245, 255), 18);
        curY += 44;
    }

    // 3. 弾薬 (AMMO) 表示
    int currentAmmo = currentWeapon->GetCurrentAmmo();
    int maxAmmo = currentWeapon->GetMaxAmmo();

    FontManager::GetInstance().DrawString(screenX + marginX, curY, "AMMO", GetColor(180, 190, 210), 15);

    if (currentWeapon->IsReloading()) {
        FontManager::GetInstance().DrawString(screenX + marginX + 70, curY - 1, "RELOADING...", GetColor(255, 200, 50), 18);
    } else {
        char ammoText[64];
        snprintf(ammoText, sizeof(ammoText), "%d", currentAmmo);
        char maxAmmoText[64];
        snprintf(maxAmmoText, sizeof(maxAmmoText), " / %d", maxAmmo);

        unsigned int ammoColor = (currentAmmo == 0) ? GetColor(255, 60, 60) : GetColor(255, 220, 40);

        int font22 = FontManager::GetInstance().GetFont(22);
        int font16 = FontManager::GetInstance().GetFont(16);
        DrawStringToHandle(screenX + marginX + 70, curY - 5, ammoText, ammoColor, font22);
        int numW = GetDrawStringWidthToHandle(ammoText, static_cast<int>(strlen(ammoText)), font22);
        DrawStringToHandle(screenX + marginX + 70 + numW, curY, maxAmmoText, GetColor(160, 170, 180), font16);
    }
    curY += 34;

    // 4. 体力 (HP) 表示 & ゲージバー
    int curHp = status.GetCurrentHp();
    int maxHp = status.GetMaxHp();
    float hpRatio = (maxHp > 0) ? static_cast<float>(curHp) / maxHp : 0.0f;
    if (hpRatio < 0.0f) hpRatio = 0.0f;
    if (hpRatio > 1.0f) hpRatio = 1.0f;

    char hpStr[32];
    snprintf(hpStr, sizeof(hpStr), "HP  %d / %d", curHp, maxHp);
    FontManager::GetInstance().DrawString(screenX + marginX, curY, hpStr, GetColor(200, 220, 240), 15);

    curY += 20;
    int barW = panelW - marginX * 2 - 10;
    int barH = 10;
    DrawBox(screenX + marginX, curY, screenX + marginX + barW, curY + barH, GetColor(30, 40, 50), TRUE);
    DrawBox(screenX + marginX, curY, screenX + marginX + barW, curY + barH, GetColor(70, 90, 110), FALSE);

    unsigned int hpBarColor = GetColor(50, 220, 100);
    if (hpRatio <= 0.3f) hpBarColor = GetColor(255, 60, 60);
    else if (hpRatio <= 0.6f) hpBarColor = GetColor(255, 200, 40);

    int fillW = static_cast<int>(barW * hpRatio);
    if (fillW > 0) {
        DrawBox(screenX + marginX + 1, curY + 1, screenX + marginX + fillW - 1, curY + barH - 1, hpBarColor, TRUE);
    }
    curY += 22;

    // 5. スキル (SKILL) 表示
    Skill* skill = GetEquippedSkill();
    if (skill)
    {
        FontManager::GetInstance().DrawString(screenX + marginX, curY, "SKILL", GetColor(180, 190, 210), 15);

        int ct = skill->GetCoolTimeTimer();
        int activeFrames = 0;
        int maxDuration = 300;
        if (skill->GetData()) maxDuration = skill->GetData()->duration;
        if (maxDuration <= 0) maxDuration = 300;

        // 透明化 (Transparency) の持続中判定
        if (IsTransparent()) {
            activeFrames = GetEffectRemainingFrames(SkillMinorTag::Transparency);
        } else if (IsHighRisk()) {
            activeFrames = GetEffectRemainingFrames(SkillMinorTag::HighRisk);
        }

        if (activeFrames > 0) {
            char activeText[32];
            snprintf(activeText, sizeof(activeText), "ACTIVE %.1fs", activeFrames / 60.0f);
            DrawString(screenX + marginX + 70, curY, activeText, GetColor(0, 255, 180));

            curY += 18;
            int actBarW = panelW - marginX * 2 - 10;
            int actBarH = 6;
            float actRatio = static_cast<float>(activeFrames) / static_cast<float>(maxDuration);
            if (actRatio < 0.0f) actRatio = 0.0f;
            if (actRatio > 1.0f) actRatio = 1.0f;

            DrawBox(screenX + marginX, curY, screenX + marginX + actBarW, curY + actBarH, GetColor(20, 40, 30), TRUE);
            DrawBox(screenX + marginX, curY, screenX + marginX + actBarW, curY + actBarH, GetColor(0, 200, 140), FALSE);
            int fillActW = static_cast<int>(actBarW * actRatio);
            if (fillActW > 0) {
                DrawBox(screenX + marginX + 1, curY + 1, screenX + marginX + fillActW - 1, curY + actBarH - 1, GetColor(0, 255, 180), TRUE);
            }
        } else if (ct > 0) {
            char ctText[32];
            snprintf(ctText, sizeof(ctText), "CT %d.%.1fs", ct / 60, (ct % 60) / 6.0f);
            DrawString(screenX + marginX + 70, curY, ctText, GetColor(170, 180, 190));
        } else {
            DrawString(screenX + marginX + 70, curY, "[ READY ] (F)", GetColor(0, 230, 255));
        }
    }

    // 2P等のオートエイム状態インジケーター表示
    if (m_inputType != PlayerInputType::KEYBOARD_MOUSE)
    {
        if (m_autoAimTarget)
        {
            FontManager::GetInstance().DrawString(screenX + marginX, screenY + panelH - 22, "[ AUTO-AIM: LOCKED ON ]", GetColor(255, 80, 80), 16);
        }
        else
        {
            FontManager::GetInstance().DrawString(screenX + marginX, screenY + panelH - 22, "[ AUTO-AIM: SEARCHING ]", GetColor(80, 220, 100), 16);
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

void Player::UpdateAutoAim(const Vector2& moveDir)
{
    m_autoAimTarget = nullptr;
    m_isAutoAimActive = false;

    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (!scene || !scene->GetObjectManager()) return;

    float bestScore = -99999.0f;
    Character* bestTarget = nullptr;
    const float maxAssistRange = 850.0f; // 吸い付き索敵範囲

    for (auto obj : scene->GetObjectManager()->GetObjects())
    {
        Character* other = dynamic_cast<Character*>(obj);
        if (!other || other == this || !other->IsActive() || other->status.IsDead()) continue;
        if (other->teamId == this->teamId || other->teamId == -1) continue;
        if (other->IsTransparent()) continue; // 透明化中の敵は対象外

        Vector2 targetPos = other->GetPosition();
        float dx = targetPos.x - position.x;
        float dy = targetPos.y - position.y;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist > maxAssistRange || dist < 1.0f) continue;

        // 壁遮蔽チェック（レイキャスト）
        if (currentStage && cellSize > 0.0f)
        {
            int steps = static_cast<int>(dist / (cellSize * 0.4f));
            if (steps < 2) steps = 2;
            float stepX = dx / steps;
            float stepY = dy / steps;
            bool hitWall = false;

            for (int i = 1; i < steps; ++i)
            {
                int gx = static_cast<int>((position.x + stepX * i) / cellSize);
                int gy = static_cast<int>((position.y + stepY * i) / cellSize);
                if (currentStage->IsOutOfBounds(gx, gy) || currentStage->IsSolidWall(gx, gy))
                {
                    hitWall = true;
                    break;
                }
            }
            if (hitWall) continue; // 壁の裏にいる敵は除外
        }

        Vector2 toEnemy(dx / dist, dy / dist);

        // 基準ベクトル（移動中なら移動方向、そうでなければ現在のエイム方向）
        Vector2 refDir = facingDir;
        if (m_isMoving)
        {
            float mLen = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
            if (mLen > 0.001f) refDir = Vector2(moveDir.x / mLen, moveDir.y / mLen);
        }

        float dot = refDir.x * toEnemy.x + refDir.y * toEnemy.y;

        // 前方150度以内 (dot > -0.3f) または 近距離 (dist < 300.0f)
        if (dot > -0.3f || dist < 300.0f)
        {
            // 正面度合いと近さからスコア算出
            float score = dot * 400.0f + (maxAssistRange - dist);
            if (score > bestScore)
            {
                bestScore = score;
                bestTarget = other;
            }
        }
    }

    m_autoAimTarget = bestTarget;

    if (m_autoAimTarget)
    {
        m_isAutoAimActive = true;
        Vector2 targetPos = m_autoAimTarget->GetPosition();
        float dx = targetPos.x - position.x;
        float dy = targetPos.y - position.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist > 0.001f)
        {
            // 敵の方向へピタリと吸い付き・ロックオン！
            facingDir = Vector2(dx / dist, dy / dist);
        }
    }
}

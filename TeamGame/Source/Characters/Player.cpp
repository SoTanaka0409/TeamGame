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
#include "../Managers/SoundManager.h"
#include "../Managers/HitFeedbackManager.h"
#include "GameSettings.h"
#include "CharacterManager.h"
#include "GameBalanceManager.h"
#include "../Skills/Skill.h"
#include "../Skills/SkillData.h"
#include "../Managers/SkillManager.h"
#include "../UI/HudStyle.h"
#include "../UI/PlayerHudRenderer.h"
#include "../Graphics/LightMaskRenderer.h"
#include <cmath>
#include <algorithm>
#include <utility>

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
        status.Init(33, 2.1f, 10);
        if (collider) collider->SetRadius(8.9f);
        radius = 8.9f;
    }

    collider->SetTag("Player");
    weapons.push_back(std::make_unique<Handgun>());
    weapons.push_back(std::make_unique<Shotgun>());
    weapons.push_back(std::make_unique<SniperRifle>());
    currentWeaponIndex = 0;
    SetEquippedSkillById(1);
    SetModelPath("Resource/GreenCharacter/character.mv1");
    m_hudRenderer = std::make_unique<PlayerHudRenderer>();
}

Player::~Player() = default;

void Player::SetWeapons(std::vector<std::unique_ptr<Weapon>> newWeapons)
{
    weapons = std::move(newWeapons);
    currentWeaponIndex = 0;
}


void Player::Update()
{
    Character::Update();

    // 死亡時のその場停止演出（死んだ場所で倒れて一定時間留まる）
    if (m_isDying)
    {
        m_isMoving = false;
        knockbackVelocity = Vector2(0.0f, 0.0f);
        if (m_model)
        {
            m_model->PlayDie();
            m_model->Update(1.0f / 60.0f);
        }

        m_deathStayTimer--;
        if (m_deathStayTimer <= 0)
        {
            m_isDying = false;
            SetActive(false); // 停止演出終了後、初めて非アクティブ化（リスポーン処理へ移行）
        }
        return;
    }

    if (!CanAction()) return;

    if (invincibleTimer > 0) invincibleTimer--;

    if (m_inputType == PlayerInputType::KEYBOARD_MOUSE)
    {
        // 懐中電灯スイッチ (右クリック)
            bool currMouseRight = ((GetMouseInput() & MOUSE_INPUT_RIGHT) != 0);
            if (currMouseRight && !m_prevMouseRight)
            {
                if (m_characterId != 1)
                {
                    m_isLightOn = !m_isLightOn;
                }
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
            int padIdx = DX_INPUT_PAD1;
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

    if (m_isMoving)
    {
        float length = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
        if (length > 0.0001f)
        {
            float speed = status.GetSpeed();
            // ハンドガン装備時: 軽量サイドアーム特有の高機動ボーナス (+12% 移動速度UP)
            if (!weapons.empty() && weapons[currentWeaponIndex] && weapons[currentWeaponIndex]->GetName() == "Handgun")
            {
                speed *= 1.12f;
            }
            float velX = (moveDir.x / length) * speed;
            float velY = (moveDir.y / length) * speed;

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

    // 歩行足音SEの再生制御 (草むら: grass_walk / 土・地面: dirt_walk)
    if (m_isMoving)
    {
        m_footstepTimer--;
        if (m_footstepTimer <= 0)
        {
            const char* seKey = m_isInBush ? "grass_walk" : "dirt_walk";
            float baseVol = m_isInBush ? 0.85f : 0.72f;
            SoundManager::GetInstance().Play3D(seKey, position, 1200.0f, baseVol, teamId);

            float spd = status.GetSpeed();
            int interval = (spd > 0.1f) ? static_cast<int>(18.0f * (4.0f / spd)) : 18;
            if (interval < 12) interval = 12;
            if (interval > 24) interval = 24;
            m_footstepTimer = interval;
        }
    }
    else
    {
        m_footstepTimer = 0;
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

        // プレイヤーの見える範囲のみアニメーション更新を実行して負荷削減
        if (IsVisibleToPlayer())
        {
            m_model->Update(1.0f / 60.0f);
        }
    }


    if (!weapons.empty())
    {
        weapons[currentWeaponIndex]->Update();
    }

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

            // 視点固定が解除されている時のみマウス方向へ更新（ウィンドウ拡縮対応）
            if (!isAimLocked)
            {
                int mouseX = 0, mouseY = 0;
                GetMousePoint(&mouseX, &mouseY);
                int drawW = 1920, drawH = 1080;
                GetDrawScreenSize(&drawW, &drawH);
                if (drawW > 0 && drawH > 0 && (drawW != 1920 || drawH != 1080))
                {
                    mouseX = static_cast<int>(mouseX * (1920.0f / drawW));
                    mouseY = static_cast<int>(mouseY * (1080.0f / drawH));
                }
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
            int padIdx = DX_INPUT_PAD1;
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

    bool isKnifePressed = false;
    bool isReloadPressed = false;
    if (m_inputType == PlayerInputType::KEYBOARD_MOUSE) {
            isKnifePressed = InputManager::GetInstance().IsKeyPressed(KEY_INPUT_SPACE);
            isReloadPressed = InputManager::GetInstance().IsKeyPressed(KEY_INPUT_R);
        }
        else {
            int padIdx = DX_INPUT_PAD1;
            int pad = GetJoypadInputState(padIdx);
            isKnifePressed = ((pad & PAD_INPUT_2) != 0) ||
                             InputManager::GetInstance().IsKeyPressed(KEY_INPUT_RSHIFT) ||
                             InputManager::GetInstance().IsKeyPressed(KEY_INPUT_NUMPAD3);
            isReloadPressed = ((pad & PAD_INPUT_5) != 0) ||
                              InputManager::GetInstance().IsKeyPressed(KEY_INPUT_PERIOD) ||
                              InputManager::GetInstance().IsKeyPressed(KEY_INPUT_O) ||
                              InputManager::GetInstance().IsKeyPressed(KEY_INPUT_NUMPAD1);
        }
    
    if (isReloadPressed && !weapons.empty())
    {
        weapons[currentWeaponIndex]->Reload(position, teamId);
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

                int hpBefore = targetEnemy->status.GetCurrentHp();
                bool wasDying = targetEnemy->IsDying();
                targetEnemy->StealthKill();

                // ナイフ暗殺の手ごたえ演出（とどめ扱い）
                if (!wasDying && targetEnemy->IsDying())
                {
                    HitFeedbackManager::GetInstance().OnPlayerHit(targetEnemy, targetEnemy->GetPosition(), hpBefore, true);
                }
            }
        }
    }
}

void Player::Draw()
{
    // 画面中央 (960, 540) に固定描画
    float screenX = Camera::WorldToScreenX(position.x);
    float screenY = Camera::WorldToScreenY(position.y);


    unsigned int color = m_isDying ? GetColor(220, 50, 50) : m_themeColor;

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
                   static_cast<int>(renderRadius), m_isDying ? GetColor(255, 80, 80) : GetColor(255, 255, 255), FALSE);

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

    if (!m_isDying)
    {
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

    }
    else
    {
        // 死亡演出中: 頭上に「DOWN」と表示
        FontManager::GetInstance().DrawStringCenter(static_cast<int>(screenX), static_cast<int>(screenY) - static_cast<int>(renderRadius) - 26, "DOWN", GetColor(255, 60, 60), 20);
    }

    // 【サーチ的な感じでブレ幅（予測線）を描画】
    if (!m_isDying && currentStage && cellSize > 0.0f)
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

                if (currentStage->IsOutOfBounds(gX, gY) || currentStage->IsBulletBlockingWall(gX, gY))
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

            HudStyle::DrawRoundPanel(boxX1, boxY1, boxX2 - boxX1, boxY2 - boxY1, HudStyle::RadiusBadge, GetColor(20, 20, 20), GetColor(255, 200, 0));
            DrawStringF(eScreenX - 65.0f, eScreenY - 50.0f, "[SPACE] KNIFE KILL", GetColor(255, 230, 0));
        }

        // 悪魔のHighRisk敵位置探知インジケータ（暗闇より優先して最前面に描画するため、RenderHighRiskEnemyIndicatorsで描画）
    }


}

void Player::RenderHighRiskEnemyIndicators() const
{
    if (m_hudRenderer)
    {
        m_hudRenderer->DrawHighRiskEnemyIndicators(*this);
    }
}

void Player::RenderBloodSplatterOverlay(int screenWidth, int screenHeight) const
{
    if (m_hudRenderer)
    {
        m_hudRenderer->DrawBloodSplatterOverlay(*this, screenWidth, screenHeight);
    }
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
        if (!m_isDying)
        {
            m_isDying = true;
            m_deathStayTimer = 110; // 約1.8秒間、死んだ場所で停止し倒れるアニメーションを表示
            knockbackVelocity = Vector2(0.0f, 0.0f);
            m_isMoving = false;
            if (m_model) m_model->PlayDie(1.2f);
        }
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

LightMaskParams Player::BuildLightMaskParams(float brightRatio) const
{
    LightMaskParams params;
    params.stage = currentStage;
    params.worldPos = position;
    params.facingDir = facingDir;
    params.cellSize = cellSize;
    params.bodyRadius = radius;
    params.closeRadiusCells = m_closeRadiusCells;
    params.maxSpotDistCells = m_maxSpotDistCells;
    params.fanAngleHalf = m_fanAngleHalf;
    params.sightMultiplier = GetSightMultiplier();
    params.brightRatio = brightRatio;
    params.isLightOn = m_isLightOn;
    params.isConductorMode = (m_characterId == 1);
    return params;
}

bool Player::IsWorldPosInLight(const Vector2& targetPos) const
{
    return LightMaskRenderer::IsWorldPosInLight(BuildLightMaskParams(0.0f), targetPos);
}

void Player::ReleaseLightMaskResources()
{
    LightMaskRenderer::ReleaseSharedResources();
}

void Player::RenderLightMask(int rectX, int rectY, int rectW, int rectH, float startDrawX, float startDrawY, float brightRatio) const
{
    (void)startDrawX;
    (void)startDrawY;
    LightMaskRenderer::Render(BuildLightMaskParams(brightRatio), rectX, rectY, rectW, rectH);

    // オートエイム発動中: ロックオン対象の敵にHUDターゲットレティクルを描画
    if (m_hudRenderer)
    {
        m_hudRenderer->DrawAutoAimReticle(*this);
    }
}

void Player::DrawUI(int screenX, int screenY)
{
    if (m_hudRenderer)
    {
        m_hudRenderer->DrawStatusHud(*this, screenX, screenY);
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
                if (currentStage->IsOutOfBounds(gx, gy) || currentStage->IsBulletBlockingWall(gx, gy))
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

bool Player::IsVisibleToPlayer() const
{
    // 自機プレイヤーは透明化スキル中であっても半透明表示されるため、常に可視とみなす
    return true;
}

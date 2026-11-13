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
#include "GameSettings.h"
#include "CharacterManager.h"
#include "GameBalanceManager.h"
#include "../Skills/Skill.h"
#include "../Skills/SkillData.h"
#include "../Managers/SkillManager.h"
#include "../UI/HudStyle.h"
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
        status.Init(33, 2.1f, 10);
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

    if (m_hudPlateHandle != -1)
    {
        DeleteGraph(m_hudPlateHandle);
        m_hudPlateHandle = -1;
    }
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

bool Player::IsWorldPosInLight(const Vector2& targetPos) const
{
    if (!currentStage || cellSize <= 0.0f) return false;

    float dx = targetPos.x - position.x;
    float dy = targetPos.y - position.y;
    float distSq = dx * dx + dy * dy;

    int stageWidth = currentStage->GetWidth();
    int stageHeight = currentStage->GetHeight();

    // 遮蔽物レイキャスト判定ラムダ
    auto isRayBlocked = [&](float toX, float toY, float totalDist) -> bool {
        if (totalDist <= cellSize * 0.5f) return false;
        int raySteps = static_cast<int>(totalDist / (cellSize * 0.35f));
        if (raySteps < 2) raySteps = 2;
        for (int s = 1; s < raySteps; ++s)
        {
            float t = static_cast<float>(s) / raySteps;
            float wx = position.x + (toX - position.x) * t;
            float wy = position.y + (toY - position.y) * t;
            int gx = static_cast<int>(wx / cellSize);
            int gy = static_cast<int>(wy / cellSize);
            if (gx < 0 || gx >= stageWidth || gy < 0 || gy >= stageHeight || currentStage->IsLightBlockingWall(gx, gy))
            {
                return true;
            }
        }
        return false;
    };

    // 1. 足元の全方位明かり範囲（指揮者の時は至近距離のみ）
    bool isConductor = (m_characterId == 1);
    float closeRadius = isConductor ? (radius + 4.0f) : (cellSize * m_closeRadiusCells * GetSightMultiplier());
    if (distSq < closeRadius * closeRadius)
    {
        float dist = std::sqrt(distSq);
        if (!isRayBlocked(targetPos.x, targetPos.y, dist))
        {
            return true;
        }
    }

    // 2. 前方60度扇形スポットライト範囲（指揮者以外かつライトON時のみ）
    if (m_characterId != 1 && m_isLightOn)
    {
        float maxSpotDist = cellSize * m_maxSpotDistCells * GetSightMultiplier();
        if (distSq < maxSpotDist * maxSpotDist)
        {
            float dist = std::sqrt(distSq);
            if (dist > 0.001f)
            {
                // エイム向きベクトルとの内積
                float forwardProj = (dx * facingDir.x + dy * facingDir.y) / dist;
                float cosHalfAngle = std::cos(m_fanAngleHalf); // 約0.866 (30度)

                if (forwardProj >= cosHalfAngle)
                {
                    if (!isRayBlocked(targetPos.x, targetPos.y, dist))
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

void Player::RenderLightMask(int rectX, int rectY, int rectW, int rectH, float startDrawX, float startDrawY, float brightRatio) const
{
    if (!currentStage || cellSize <= 0.0f) return;

    // プレイヤーの画面上での中心位置 (960, 540)
    float playerPixelX = Camera::WorldToScreenX(position.x);
    float playerPixelY = Camera::WorldToScreenY(position.y);

    float zoomCellSize = cellSize * Camera::ZoomScale;
    float zoomScale = Camera::ZoomScale;

    bool isConductor = (m_characterId == 1); // 指揮者判定

    float maxSpotDist = zoomCellSize * m_maxSpotDistCells * GetSightMultiplier();
    // 指揮者の時は足元の全方位明かりを自身の身体直近のみに抑え、前方ライト光以外は見えないようにする
    float closeRadius = isConductor ? (radius * zoomScale + 4.0f) : (zoomCellSize * m_closeRadiusCells * GetSightMultiplier());

    int stageWidth = currentStage->GetWidth();
    int stageHeight = currentStage->GetHeight();

    const int resolutionStep = 8; // 8pxステップで高精細かつ大幅軽量化

    float lightAngle = std::atan2(facingDir.y, facingDir.x);

    // 月明かり（薄明時間帯）の環境光ブーストと最大暗闇アルファ
    // 指揮者の時は完全な漆黒(255)かつ環境光ブーストなし（ライトの光以外は見えない）
    float globalBrightVal = isConductor ? 0.0f : (brightRatio * 0.05f);
    float maxDarkAlpha = isConductor ? 255.0f : (248.0f * (1.0f - brightRatio * 0.10f));

    float maxSpotDistSq = maxSpotDist * maxSpotDist;
    float closeRadiusSq = closeRadius * closeRadius;

    // 扇形コサイン事前計算
    float cosHalfAngle = std::cos(m_fanAngleHalf);
    float cosHalfAngleSq = cosHalfAngle * cosHalfAngle;

    unsigned int darkColor = isConductor ? GetColor(0, 0, 0) : GetColor(4, 5, 10);
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

            // B. 前方60°扇形スポットライト（指揮者は無能キャラ仕様のためライトが一切見えない・光が照射されない）
            if (m_characterId != 1 && m_isLightOn && distSq < maxSpotDistSq)
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
            if (alpha >= 250) qAlpha = 255;

            if (qAlpha != runAlpha)
            {
                if (runAlpha > 0 && runStartX >= 0)
                {
                    if (currentBlendAlpha != runAlpha)
                    {
                        if (runAlpha >= 255)
                        {
                            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                        }
                        else
                        {
                            SetDrawBlendMode(DX_BLENDMODE_ALPHA, runAlpha);
                        }
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
                if (runAlpha >= 255)
                {
                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                }
                else
                {
                    SetDrawBlendMode(DX_BLENDMODE_ALPHA, runAlpha);
                }
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

    Weapon* currentWeapon = weapons[currentWeaponIndex];
    if (!currentWeapon) return;

    // 1. キャラクター別HUDプレート画像のロード（未ロード時）
    if (m_hudPlateHandle == -1)
    {
        static const std::string hudFiles[5] = {
            "Resource/Images/UI/HUD/hud_conductor.png",
            "Resource/Images/UI/HUD/hud_rebel.png",
            "Resource/Images/UI/HUD/hud_baka.png",
            "Resource/Images/UI/HUD/hud_housewife.png",
            "Resource/Images/UI/HUD/hud_devil.png"
        };
        int idx = m_characterId - 1;
        if (idx >= 0 && idx < 5)
        {
            m_hudPlateHandle = Scene::LoadGraphWithSearch(hudFiles[idx]);
        }
    }

    // 2. HUDプレートの配置・スケール設定 (1024x480 -> スケール0.42: 430x201)
    const float scale = 0.42f;
    const int plateW = static_cast<int>(1024 * scale); // 430
    const int plateH = static_cast<int>(480 * scale);  // 201
    
    int plateX = screenX;
    int plateY = screenY;
    if (plateY + plateH > 1060) {
        plateY = 1060 - plateH;
    }
    if (plateX < 20) plateX = 20;

    // 3. 上部: コンパクトな武器・弾薬 (AMMO) バナー
    const int bannerH = 26;
    const int bannerY = plateY - bannerH - 6;
    HudStyle::DrawPanel(plateX, bannerY, plateW, bannerH, false);

    // 武器名
    std::string wName = currentWeapon->GetName();
    if (isAimLocked) wName += " [LOCK]";
    char weaponTitle[64];
    snprintf(weaponTitle, sizeof(weaponTitle), "[%d] %s", currentWeaponIndex + 1, wName.c_str());
    FontManager::GetInstance().DrawString(plateX + 12, bannerY + 5, weaponTitle, isAimLocked ? HudStyle::ColAmber() : HudStyle::ColText(), 14);

    // 弾薬数
    int currentAmmo = currentWeapon->GetCurrentAmmo();
    int maxAmmo = currentWeapon->GetMaxAmmo();
    if (currentWeapon->IsReloading())
    {
        FontManager::GetInstance().DrawStringRight(plateX + plateW - 12, bannerY + 5, "RELOADING...", HudStyle::ColAmber(), 14);
    }
    else
    {
        char ammoText[32];
        snprintf(ammoText, sizeof(ammoText), "AMMO  %d / %d", currentAmmo, maxAmmo);
        unsigned int ammoColor = HudStyle::GetAmmoColor(currentAmmo, maxAmmo);
        FontManager::GetInstance().DrawStringRight(plateX + plateW - 12, bannerY + 5, ammoText, ammoColor, 14);
    }

    // 4. キャラクター別HUDプレート画像の描画
    if (m_hudPlateHandle != -1)
    {
        DrawExtendGraph(plateX, plateY, plateX + plateW, plateY + plateH, m_hudPlateHandle, TRUE);
    }
    else
    {
        // フォールバック描画
        HudStyle::DrawPanel(plateX, plateY, plateW, plateH, true);
    }

    // 5. 各キャラクター固有のバー座標（1024x480画像内の矩形座標）
    struct BarRect {
        int x0, y0, x1, y1;
    };
    static const BarRect mpSlots[5] = {
        { 200, 102, 880, 155 }, // 1: 指揮者（ト音記号の右から終止線まで）
        { 136, 104, 990, 168 }, // 2: 反逆者（有刺鉄線フレーム内）
        { 140, 105, 880, 165 }, // 3: バカ（黄色キャンバス）
        { 140, 105, 880, 165 }, // 4: 主婦（桜吹雪の軌道上）
        { 140, 105, 750, 165 }  // 5: 悪魔（月手前の境界まで）
    };
    static const BarRect hpSlots[5] = {
        { 200, 324, 880, 377 }, // 1: 指揮者（ヘ音記号の右から終止線まで）
        { 136, 326, 990, 390 }, // 2: 反逆者（有刺鉄線フレーム内）
        { 140, 320, 880, 380 }, // 3: バカ（黄色キャンバス）
        { 140, 320, 880, 380 }, // 4: 主婦（桜吹雪の軌道上）
        { 140, 320, 750, 380 }  // 5: 悪魔
    };

    int charIdx = m_characterId - 1;
    if (charIdx < 0 || charIdx >= 5) charIdx = 0;

    int mpX = plateX + static_cast<int>(mpSlots[charIdx].x0 * scale);
    int mpY = plateY + static_cast<int>(mpSlots[charIdx].y0 * scale);
    int mpW = static_cast<int>((mpSlots[charIdx].x1 - mpSlots[charIdx].x0) * scale);
    int mpH = static_cast<int>((mpSlots[charIdx].y1 - mpSlots[charIdx].y0) * scale);

    int hpX = plateX + static_cast<int>(hpSlots[charIdx].x0 * scale);
    int hpY = plateY + static_cast<int>(hpSlots[charIdx].y0 * scale);
    int hpW = static_cast<int>((hpSlots[charIdx].x1 - hpSlots[charIdx].x0) * scale);
    int hpH = static_cast<int>((hpSlots[charIdx].y1 - hpSlots[charIdx].y0) * scale);

    // 6. MPバー（スキルクールタイム：MAXで使用可能）の描画
    Skill* skill = GetEquippedSkill();
    if (skill)
    {
        int ct = skill->GetCoolTimeTimer();
        int maxCt = (skill->GetData() && skill->GetData()->coolTime > 0) ? skill->GetData()->coolTime : 300;
        int activeFrames = 0;
        int maxDuration = (skill->GetData() && skill->GetData()->duration > 0) ? skill->GetData()->duration : 300;

        if (IsTransparent()) {
            activeFrames = GetEffectRemainingFrames(SkillMinorTag::Transparency);
        } else if (IsHighRisk()) {
            activeFrames = GetEffectRemainingFrames(SkillMinorTag::HighRisk);
        }

        // 下地トラック（半透明ブラック）
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 140);
        DrawBox(mpX, mpY, mpX + mpW, mpY + mpH, GetColor(10, 15, 20), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        if (activeFrames > 0)
        {
            // スキル効果発動中（アンバー色で効果持続時間を表示）
            float actRatio = static_cast<float>(activeFrames) / static_cast<float>(maxDuration);
            if (actRatio > 1.0f) actRatio = 1.0f; else if (actRatio < 0.0f) actRatio = 0.0f;
            int fillW = static_cast<int>(mpW * actRatio);

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
            DrawBox(mpX, mpY, mpX + fillW, mpY + mpH, GetColor(255, 180, 20), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            DrawBox(mpX, mpY, mpX + mpW, mpY + mpH, GetColor(255, 220, 80), FALSE);

            char actText[32];
            snprintf(actText, sizeof(actText), "ACTIVE %.1fs", activeFrames / 60.0f);
            FontManager::GetInstance().DrawStringCenter(mpX + mpW / 2, mpY + (mpH - 14) / 2, actText, GetColor(255, 255, 255), 13);
        }
        else if (ct > 0)
        {
            // クールタイム中: MPが0%から徐々に100%(MAX)に向かってチャージされる
            float mpRatio = 1.0f - static_cast<float>(ct) / static_cast<float>(maxCt);
            if (mpRatio < 0.0f) mpRatio = 0.0f; else if (mpRatio > 1.0f) mpRatio = 1.0f;
            int fillW = static_cast<int>(mpW * mpRatio);

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 175);
            DrawBox(mpX, mpY, mpX + fillW, mpY + mpH, GetColor(0, 160, 240), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            DrawBox(mpX, mpY, mpX + mpW, mpY + mpH, GetColor(30, 70, 110), FALSE);

            char ctText[32];
            snprintf(ctText, sizeof(ctText), "MP CHARGING %.1fs", ct / 60.0f);
            FontManager::GetInstance().DrawStringCenter(mpX + mpW / 2, mpY + (mpH - 14) / 2, ctText, GetColor(210, 235, 255), 13);
        }
        else
        {
            // MPがMAXに到達！ スキル発動可能（Readyパルス発光）
            static int readyTimer = 0;
            readyTimer++;
            int pulseAlpha = 180 + static_cast<int>(55.0f * std::sin(readyTimer * 0.08f));

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, pulseAlpha);
            DrawBox(mpX, mpY, mpX + mpW, mpY + mpH, GetColor(0, 205, 255), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            // 発光枠
            DrawBox(mpX - 1, mpY - 1, mpX + mpW + 1, mpY + mpH + 1, GetColor(220, 250, 255), FALSE);

            const char* readyKey = (m_inputType == PlayerInputType::KEYBOARD_MOUSE) ? "READY [F]" : "READY [RT]";
            FontManager::GetInstance().DrawStringCenter(mpX + mpW / 2, mpY + (mpH - 15) / 2, readyKey, GetColor(255, 255, 255), 14);
        }
    }

    // 7. HPバー（プレイヤー体力）の描画
    int curHp = status.GetCurrentHp();
    int maxHp = status.GetMaxHp();
    float hpRatio = (maxHp > 0) ? static_cast<float>(curHp) / static_cast<float>(maxHp) : 0.0f;
    if (hpRatio < 0.0f) hpRatio = 0.0f; else if (hpRatio > 1.0f) hpRatio = 1.0f;

    // 下地トラック（半透明ブラック）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 140);
    DrawBox(hpX, hpY, hpX + hpW, hpY + hpH, GetColor(10, 15, 20), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // HPゲージ充填 (緑 -> 黄 -> 赤)
    int hpFillW = static_cast<int>(hpW * hpRatio);
    unsigned int hpCol = GetColor(40, 220, 100);
    if (hpRatio <= 0.25f) hpCol = GetColor(230, 50, 50);
    else if (hpRatio <= 0.50f) hpCol = GetColor(240, 180, 40);

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 195);
    DrawBox(hpX, hpY, hpX + hpFillW, hpY + hpH, hpCol, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(hpX, hpY, hpX + hpW, hpY + hpH, GetColor(50, 60, 70), FALSE);

    // HP数値表示
    char hpStr[32];
    snprintf(hpStr, sizeof(hpStr), "%d / %d", curHp, maxHp);
    FontManager::GetInstance().DrawStringCenter(hpX + hpW / 2, hpY + (hpH - 14) / 2, hpStr, GetColor(255, 255, 255), 14);

    // 8. 2P等のオートエイム状態インジケーター表示（必要時）
    if (m_inputType != PlayerInputType::KEYBOARD_MOUSE)
    {
        if (m_autoAimTarget)
        {
            FontManager::GetInstance().DrawString(plateX + 10, bannerY - 18, "[ AUTO-AIM: LOCKED ]", HudStyle::ColEnemy(), 11);
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

bool Player::IsVisibleToPlayer() const
{
    // 自機プレイヤーは透明化スキル中であっても半透明表示されるため、常に可視とみなす
    return true;
}

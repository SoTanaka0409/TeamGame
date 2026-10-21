#include "Camera.h"
#define NOMINMAX
#include "Enemy.h"
#include "SoundManager.h"
#include "DxLib.h"
#include "Bullet.h"
#include "Player.h"
#include "Stage.h"
#include "SceneManager.h"
#include "Scene.h"
#include "CharacterManager.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

Enemy::Enemy(float startX, float startY, int tId)
    : Character(ObjectTag::Enemy, startX, startY, 20.0f), damageColorTimer(0),
      currentStage(nullptr), cellSize(1.0f), targetCharacter(nullptr),
      aiState(EnemyAIState::PATROL), facingDir(0.0f, 1.0f), moveDir(0.0f, 1.0f),
      lastKnownPos(startX, startY), patrolChangeTimer(0),
      moveToCenterTimer(300), investigateTimer(0), shootCooldown(0),
      aimDelayTimer(0), strafeDirection(1), strafeTimer(0)
{
    teamId = tId;
    characterName = (teamId == 0) ? "味方ボット" : "敵ボット";
    std::string botType = (teamId == 0) ? "AllyBot" : "EnemyBot";
    const CharacterData* data = CharacterManager::GetInstance().GetCharacterData(botType);
    if (!data && teamId != 0) data = CharacterManager::GetInstance().GetCharacterData("EnemyBot");

    if (data)
    {
        status.Init(data->maxHp, data->moveSpeed, data->attackPower);
        collider->SetRadius(data->colliderRadius);
        radius = data->colliderRadius;
        sightRangeCells = data->sightRangeCells;
        effectiveRangeCells = data->effectiveRangeCells;
    }
    else
    {
        status.Init(3, 1.5f, 1);
        collider->SetRadius(20.0f);
        radius = 20.0f;
    }
    collider->SetTag("Enemy");
    SetModelPath("Resource/GreenCharacter/character.mv1");
}

Enemy::~Enemy()
{
    delete equippedWeapon;
}

void Enemy::SetEquippedWeapon(Weapon* w)
{
    if (equippedWeapon) delete equippedWeapon;
    equippedWeapon = w;
}

float Enemy::GetEffectiveRange() const
{
    if (equippedWeapon && equippedWeapon->GetData())
    {
        return equippedWeapon->GetData()->range;
    }
    return effectiveRangeCells * cellSize;
}

void Enemy::OnHearGunshot(const Vector2 &soundPos, float loudness, int shooterTeamId)
{
    // 非パニック時: 同じチームからの発砲音は味方の援護射撃なので警戒・敵対しない
    if (!this->IsPanic() && shooterTeamId != -1 && shooterTeamId == this->teamId)
    {
        return;
    }

    // 発砲者が透明化(Transparency)中の場合は音も消滅・敵に察知されない
    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (scene && scene->GetObjectManager())
    {
        for (auto* obj : scene->GetObjectManager()->GetObjects())
        {
            Character* c = dynamic_cast<Character*>(obj);
            if (c && c->IsActive() && c->IsTransparent())
            {
                float sx = soundPos.x - c->GetPosition().x;
                float sy = soundPos.y - c->GetPosition().y;
                if (sx * sx + sy * sy < 40.0f * 40.0f)
                {
                    // 透明化中のキャラクター自身が発生させた音なので無音化
                    return;
                }
            }
        }
    }

    if (aiState == EnemyAIState::ALERT) return;

    float dx = soundPos.x - position.x;
    float dy = soundPos.y - position.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    float maxDist = (loudness > 1.0f) ? loudness : (cellSize * 15.0f);
    if (cellSize > 0.0f && dist < maxDist)
    {
        lastKnownPos = soundPos;
        aiState = EnemyAIState::INVESTIGATE;
        investigateTimer = 180;
        if (dist > 0.0001f)
        {
            facingDir = Vector2(dx / dist, dy / dist);
        }
    }
}

void Enemy::UpdateTarget()
{
    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (!scene || !scene->GetObjectManager()) return;

    // ターゲットが非アクティブ、または混乱解除後に味方をターゲットしている場合は即座に解除
    if (targetCharacter && (!targetCharacter->IsActive() || (!this->IsPanic() && targetCharacter->teamId == this->teamId)))
    {
        targetCharacter = nullptr;
    }

    float minDist = 999999.0f;
    Character* visibleEnemy = nullptr;

    bool selfPanic = this->IsPanic();
    for (auto obj : scene->GetObjectManager()->GetObjects())
    {
        Character* c = dynamic_cast<Character*>(obj);
        if (c && c != this && c->IsActive() && c->teamId != -1)
        {
            // 透明化中のキャラは敵・味方問わず視界外・不可視として扱う
            if (c->IsTransparent()) continue;

            // パニック（バトルロワイヤル）中ならチーム関係なく、マップ上の最も近いキャラクターを無差別ロックオン
            if (selfPanic)
            {
                float dx = c->GetPosition().x - position.x;
                float dy = c->GetPosition().y - position.y;
                float dist = dx * dx + dy * dy;
                // 遮蔽物があってもバトロワ中は最短の対象へ向かって追跡・交戦する
                if (dist < minDist) {
                    minDist = dist;
                    visibleEnemy = c;
                }
            }
            else if (c->teamId != this->teamId)
            {
                if (CheckLineOfSightToTarget(c)) {
                    float dx = c->GetPosition().x - position.x;
                    float dy = c->GetPosition().y - position.y;
                    float dist = dx * dx + dy * dy;
                    if (dist < minDist) {
                        minDist = dist;
                        visibleEnemy = c;
                    }
                }
            }
        }
    }

    if (visibleEnemy) {
        targetCharacter = visibleEnemy;
    } else if (!selfPanic && targetCharacter && targetCharacter->teamId == this->teamId) {
        targetCharacter = nullptr;
    }
}

bool Enemy::CheckLineOfSightToTarget(Character* target) const
{
    Character* checkTarget = target ? target : targetCharacter;
    if (!checkTarget || !checkTarget->IsActive() || !currentStage || cellSize <= 0.0f)
    {
        return false;
    }

    Vector2 pPos = checkTarget->GetPosition();
    float dx = pPos.x - position.x;
    float dy = pPos.y - position.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    float maxSightDist = GetSightRange() * GetSightMultiplier();
    if (dist > maxSightDist)
    {
        return false;
    }

    bool selfInBush = this->IsInBush();
    bool targetInBush = checkTarget->IsInBush();

    if (targetInBush)
    {
        if (selfInBush)
        {
            // お互い草むらの中にいる場合は中・近距離(5.0セル)でお互いを認識・交戦
            if (dist > cellSize * 5.0f)
            {
                return false;
            }
        }
        else
        {
            // ターゲットだけが草むら内に潜んでいる場合は至近距離(1.5セル)以外は見えない
            if (dist > cellSize * 1.5f)
            {
                return false;
            }
        }
    }

    // パニック（錯乱）状態中は全方位360度見境なく索敵（視野角制限を解除）
    if (!this->IsPanic() && dist > cellSize * 1.0f)
    {
        float facingAngle = std::atan2(facingDir.y, facingDir.x);
        float targetAngle = std::atan2(dy, dx);
        float angleDiff = std::abs(targetAngle - facingAngle);
        while (angleDiff > 3.14159265f)
        {
            angleDiff = std::abs(angleDiff - 2.0f * 3.14159265f);
        }

        if (angleDiff > 0.8000f)
        {
            return false;
        }
    }

    if (dist > cellSize * 0.5f)
    {
        int steps = static_cast<int>(dist / (cellSize * 0.5f));
        if (steps < 2) steps = 2;

        float stepX = dx / steps;
        float stepY = dy / steps;

        for (int i = 1; i < steps; ++i)
        {
            float checkX = position.x + stepX * i;
            float checkY = position.y + stepY * i;
            int gX = static_cast<int>(checkX / cellSize);
            int gY = static_cast<int>(checkY / cellSize);

            if (currentStage->IsOutOfBounds(gX, gY) || currentStage->IsSolidWall(gX, gY))
            {
                return false;
            }
        }
    }

    return true;
}

// 【賢いナビゲーション】 壁角に引っかからず滑らかに回避・スライド移動する関数
void Enemy::MoveSmart(const Vector2 &desiredDir)
{
    float len = std::sqrt(desiredDir.x * desiredDir.x + desiredDir.y * desiredDir.y);
    if (len < 0.0001f)
    {
        if (m_model) m_model->PlayIdle();
        return;
    }

    if (m_model) m_model->PlayWalk();

    Vector2 normDir(desiredDir.x / len, desiredDir.y / len);
    float velX = normDir.x * status.GetSpeed();
    float velY = normDir.y * status.GetSpeed();

    if (!currentStage || cellSize <= 0.0f)
    {
        position.x += velX;
        position.y += velY;
        return;
    }

    // 接触判定対象（プレイヤーおよび他の敵キャラクター）の座標を取得
    std::vector<Vector2> otherPositions;
    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (scene && scene->GetObjectManager())
    {
        for (auto obj : scene->GetObjectManager()->GetObjects())
        {
            if (obj && obj->IsActive() && obj != this)
            {
                if (obj->GetObjectTag() == ObjectTag::Player || obj->GetObjectTag() == ObjectTag::Enemy)
                {
                    otherPositions.push_back(obj->GetPosition());
                }
            }
        }
    }
    float minDist = radius * 2.0f; // 40px

    // X軸の移動と衝突判定（壁および他キャラクター接触）
    float nextX = position.x + velX;
    int gridX = static_cast<int>(nextX / cellSize);
    int gridY = static_cast<int>(position.y / cellSize);
    bool xBlocked = currentStage->IsSolidWall(gridX, gridY);

    if (!xBlocked)
    {
        // 接触時に対象の方向（距離が縮まる方向）への移動をブロック
        for (const auto &oPos : otherPositions)
        {
            float curDx = oPos.x - position.x;
            float curDy = oPos.y - position.y;
            float curDistSq = curDx * curDx + curDy * curDy;

            float nextDx = oPos.x - nextX;
            float nextDy = oPos.y - position.y;
            float nextDistSq = nextDx * nextDx + nextDy * nextDy;

            if (nextDistSq < minDist * minDist && nextDistSq < curDistSq)
            {
                xBlocked = true;
                break;
            }
        }
    }

    if (!xBlocked)
    {
        position.x = nextX;
    }

    // Y軸の移動と衝突判定（壁および他キャラクター接触）
    float nextY = position.y + velY;
    gridX = static_cast<int>(position.x / cellSize);
    gridY = static_cast<int>(nextY / cellSize);
    bool yBlocked = currentStage->IsSolidWall(gridX, gridY);

    if (!yBlocked)
    {
        // 接触時に対象の方向（距離が縮まる方向）への移動をブロック
        for (const auto &oPos : otherPositions)
        {
            float curDx = oPos.x - position.x;
            float curDy = oPos.y - position.y;
            float curDistSq = curDx * curDx + curDy * curDy;

            float nextDx = oPos.x - position.x;
            float nextDy = oPos.y - nextY;
            float nextDistSq = nextDx * nextDx + nextDy * nextDy;

            if (nextDistSq < minDist * minDist && nextDistSq < curDistSq)
            {
                yBlocked = true;
                break;
            }
        }
    }

    if (!yBlocked)
    {
        position.y = nextY;
    }

    // 正面が壁やキャラクターで引っかかった場合、角を回避するスライド移動を試みる
    if (xBlocked || yBlocked)
    {
        Vector2 slideDir1(normDir.y, -normDir.x);
        Vector2 slideDir2(-normDir.y, normDir.x);

        auto checkSlideValid = [&](const Vector2 &sDir) -> bool {
            float sNextX = position.x + sDir.x * (status.GetSpeed() * 0.7f);
            float sNextY = position.y + sDir.y * (status.GetSpeed() * 0.7f);
            int gx = static_cast<int>(sNextX / cellSize);
            int gy = static_cast<int>(sNextY / cellSize);
            if (currentStage->IsSolidWall(gx, gy)) return false;

            for (const auto &oPos : otherPositions)
            {
                float curDx = oPos.x - position.x;
                float curDy = oPos.y - position.y;
                float curDistSq = curDx * curDx + curDy * curDy;

                float nextDx = oPos.x - sNextX;
                float nextDy = oPos.y - sNextY;
                float nextDistSq = nextDx * nextDx + nextDy * nextDy;

                if (nextDistSq < minDist * minDist && nextDistSq < curDistSq)
                {
                    return false;
                }
            }
            return true;
        };

        if (checkSlideValid(slideDir1))
        {
            position.x += slideDir1.x * (status.GetSpeed() * 0.7f);
            position.y += slideDir1.y * (status.GetSpeed() * 0.7f);
        }
        else if (checkSlideValid(slideDir2))
        {
            position.x += slideDir2.x * (status.GetSpeed() * 0.7f);
            position.y += slideDir2.y * (status.GetSpeed() * 0.7f);
        }
    }

    if (currentStage)
    {
        currentStage->ResolveCollision(position, radius, cellSize);
    }
}

void Enemy::Update()
{
    float startPosX = position.x;
    float startPosY = position.y;

    Character::Update();
    if (!CanAction()) return;

    if (equippedWeapon)
    {
        equippedWeapon->Update();
    }

    // 草むら潜伏チェック
    if (currentStage && cellSize > 0.0f)
    {
        int gX = static_cast<int>(position.x / cellSize);
        int gY = static_cast<int>(position.y / cellSize);
        if (!currentStage->IsOutOfBounds(gX, gY))
        {
            m_isInBush = (currentStage->GetCell(gX, gY) == CellType::BUSH);
        }
        else
        {
            m_isInBush = false;
        }
    }

    // HomingKill 特攻自爆処理
    if (isHomingSuicide)
    {
        suicideTimer--;
        auto scene = SceneManager::GetInstance().GetCurrentScene();
        if (targetCharacter && targetCharacter->IsActive() && !targetCharacter->status.IsDead())
        {
            Vector2 tPos = targetCharacter->GetPosition();
            float dx = tPos.x - position.x;
            float dy = tPos.y - position.y;
            float dist = std::sqrt(dx * dx + dy * dy);

            // 敵の手前（110px：約1.5マス離れた位置）に到達した段階、または時間切れで爆発
            // 爆発半径は90pxとし、「敵の目の前で爆発するが、確実に当たるわけではない（敵の移動等で回避可能）」ようにする
            if (dist <= 110.0f || suicideTimer <= 0)
            {
                if (scene && scene->GetEffectManager())
                {
                    // 指揮者による非情・残酷な特攻爆散ゴアエフェクト（大量鮮血・肉片骨片飛散・血溜まり残留・爆砕波）
                    scene->GetEffectManager()->AddCruelExplosionEffect(position.x, position.y, 90.0f);
                }

                // 周囲の敵キャラクターにダメージ判定（半径90px。手前110pxで止まるため、動いている敵には外れることもある）
                if (scene && scene->GetObjectManager())
                {
                    float blastRadiusSq = 90.0f * 90.0f;
                    for (auto* obj : scene->GetObjectManager()->GetObjects())
                    {
                        Character* c = dynamic_cast<Character*>(obj);
                        if (c && c != this && c->IsActive() && c->teamId != this->teamId)
                        {
                            float cdx = c->GetPosition().x - position.x;
                            float cdy = c->GetPosition().y - position.y;
                            if (cdx * cdx + cdy * cdy <= blastRadiusSq)
                            {
                                c->SetLastAttacker(GetCharacterName(), teamId);
                                c->status.TakeDamage(100);
                            }
                        }
                    }
                }

                // 爆破時に特攻状態を完全解除
                StopHomingSuicide();
                Damage(status.GetMaxHp()); // 自分自身も爆散・死亡
                return;
            }

            // 標的に向かって最短ルートで急行
            if (currentStage && cellSize > 0.0f)
            {
                pathfinder.CalculatePath(position, tPos, currentStage, cellSize);
                Vector2 moveDir = pathfinder.GetMoveDirection(position, status.GetSpeed(), cellSize);
                if (moveDir.x != 0 || moveDir.y != 0) {
                    facingDir = moveDir;
                    MoveSmart(moveDir);
                } else {
                    Vector2 directDir(dx / (dist > 0.001f ? dist : 1.0f), dy / (dist > 0.001f ? dist : 1.0f));
                    facingDir = directDir;
                    MoveSmart(directDir);
                }
            }
            return;
        }
        else
        {
            // 標的が既に死亡していたら特攻を解除して通常状態に復帰（無駄な即爆発死を防止）
            StopHomingSuicide();
            return;
        }
    }

    // 混乱（バトルロワイヤル）解除時の完全復帰処理
    bool currentPanic = IsPanic();
    if (wasPanic && !currentPanic)
    {
        // 混乱が切れた瞬間にターゲットを完全初期化し、味方への誤射・追尾を直ちに中断して巡回へ戻す
        targetCharacter = nullptr;
        aimDelayTimer = 0;
        shootCooldown = 40;
        aiState = EnemyAIState::PATROL;
        patrolChangeTimer = 30;
    }
    wasPanic = currentPanic;

    // 非パニック状態かつ味方をターゲットしていた場合は強制リセット
    if (!currentPanic && targetCharacter && targetCharacter->teamId == this->teamId)
    {
        targetCharacter = nullptr;
        if (aiState == EnemyAIState::ALERT)
        {
            aiState = EnemyAIState::PATROL;
        }
    }

    UpdateTarget();
    if (damageColorTimer > 0)
    {
        damageColorTimer--;
    }

    if (shootCooldown > 0)
    {
        shootCooldown--;
    }

    // 視界チェック
    bool canSeeTarget = CheckLineOfSightToTarget(targetCharacter);

    if (canSeeTarget)
    {
        if (aiState != EnemyAIState::ALERT)
        {
            aiState = EnemyAIState::ALERT;
            aimDelayTimer = IsPanic() ? 5 : 25; // パニック中はエイム遅延ほぼゼロで即射撃
        }
        lastKnownPos = targetCharacter->GetPosition();
    }
    else if (IsPanic())
    {
        // パニック中は標的を見失っても狂乱状態のまま（ALERT継続）
        aiState = EnemyAIState::ALERT;
    }
    else if (aiState == EnemyAIState::ALERT)
    {
        // 視界が切れたら最後に見かけた位置の調査モード(INVESTIGATE)へ移行
        aiState = EnemyAIState::INVESTIGATE;
        investigateTimer = 60; // 1秒間探索してすぐ巡回へ
    }

    // 警戒・交戦状態: 非パニック時に味方なら即座に交戦中止
    if (!IsPanic() && targetCharacter && targetCharacter->teamId == this->teamId)
    {
        targetCharacter = nullptr;
        aiState = EnemyAIState::PATROL;
    }

    if (aiState == EnemyAIState::ALERT && targetCharacter && targetCharacter->IsActive())
    {
        if (aimDelayTimer > 0)
        {
            aimDelayTimer--;
        }

        Vector2 pPos = targetCharacter->GetPosition();
        float dx = pPos.x - position.x;
        float dy = pPos.y - position.y;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist > 0.0001f)
        {
            facingDir = Vector2(dx / dist, dy / dist);
        }

        bool canHitDirectly = (dist <= GetEffectiveRange()) && CheckLineOfSightToTarget(targetCharacter);

        if (!canHitDirectly)
        {
            if (currentStage) {
                pathfinder.CalculatePath(position, pPos, currentStage, cellSize);
                Vector2 pathDir = pathfinder.GetMoveDirection(position, status.GetSpeed(), cellSize);
                if (pathDir.x != 0 || pathDir.y != 0) {
                    facingDir = pathDir;
                    MoveSmart(pathDir);
                } else {
                    MoveSmart(facingDir);
                }
            } else {
                MoveSmart(facingDir);
            }
        }
        else
        {
            strafeTimer--;
            if (strafeTimer <= 0)
            {
                strafeDirection = (rand() % 2 == 0) ? 1 : -1;
                strafeTimer = 40 + (rand() % 40);
            }
            Vector2 strafeDir(-facingDir.y * strafeDirection, facingDir.x * strafeDirection);
            MoveSmart(strafeDir);
        }

        if (shootCooldown <= 0 && aimDelayTimer <= 0 && canHitDirectly)
        {
            Vector2 muzzlePos(position.x + facingDir.x * (radius + 5.0f),
                            position.y + facingDir.y * (radius + 5.0f));

            if (equippedWeapon)
            {
                if (equippedWeapon->CanFire())
                {
                    equippedWeapon->Fire(muzzlePos, facingDir, teamId, false, GetCharacterName(), IsPanic());
                    if (m_model) m_model->PlayShoot();
                    int interval = (equippedWeapon->GetData()) ? equippedWeapon->GetData()->fireInterval : 75;
                    // パニック乱戦中は連射速度が激化（クールダウン半減）
                    if (IsPanic()) interval = (interval > 15) ? (interval / 2) : interval;
                    shootCooldown = interval;
                }
            }
            else
            {
                float baseAngle = std::atan2(facingDir.y, facingDir.x);
                float angleOffset = 0.0f;

                if (rand() % 2 == 0)
                {
                    float missDir = (rand() % 2 == 0) ? 1.0f : -1.0f;
                    float missDegree = 12.0f + (rand() % 14);
                    angleOffset = missDir * (missDegree * 3.14159265f / 180.0f);
                }
                else
                {
                    angleOffset = ((rand() % 1000) / 1000.0f - 0.5f) * (6.0f * 3.14159265f / 180.0f);
                }

                float finalAngle = baseAngle + angleOffset;
                Vector2 fireDir(std::cos(finalAngle), std::sin(finalAngle));

                new Bullet(muzzlePos.x, muzzlePos.y, fireDir, 14.0f, GetEffectiveRange(), 6.0f, teamId, 1, GetCharacterName(), IsPanic());
                if (m_model) m_model->PlayShoot();

                auto scene = SceneManager::GetInstance().GetCurrentScene();
                if (scene && scene->GetEffectManager())
                {
                    scene->GetEffectManager()->AddMuzzleFlashEffect(muzzlePos.x, muzzlePos.y, finalAngle, 16.0f);
                }

                SoundManager::GetInstance().Play3D("gunshot", position, 1000.0f, 1.0f, teamId);

                shootCooldown = IsPanic() ? 25 : 75; // パニック中は通常75→25フレームの超連射乱戦
            }
        }
    }
    else if (aiState == EnemyAIState::INVESTIGATE)
    {
        investigateTimer--;
        float dx = lastKnownPos.x - position.x;
        float dy = lastKnownPos.y - position.y;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist > cellSize * 0.8f && investigateTimer > 20)
        {
            if (currentStage) {
                pathfinder.CalculatePath(position, lastKnownPos, currentStage, cellSize);
                Vector2 pathDir = pathfinder.GetMoveDirection(position, status.GetSpeed(), cellSize);
                if (pathDir.x != 0 || pathDir.y != 0) {
                    facingDir = pathDir;
                    MoveSmart(pathDir);
                } else {
                    Vector2 toTarget(dx / dist, dy / dist);
                    facingDir = toTarget;
                    MoveSmart(toTarget);
                }
            } else {
                Vector2 toTarget(dx / dist, dy / dist);
                facingDir = toTarget;
                MoveSmart(toTarget);
            }
        }
        else
        {
            float scanAng = std::atan2(facingDir.y, facingDir.x) + 0.1f;
            facingDir = Vector2(std::cos(scanAng), std::sin(scanAng));

            if (investigateTimer <= 0)
            {
                aiState = EnemyAIState::PATROL; // プレイヤーが見つからなければ巡回に戻る
            }
        }
    }
    else
    {
        // PATROL
        if (moveToCenterTimer > 0 && currentStage)
        {
            moveToCenterTimer--;
            float centerX = (currentStage->GetWidth() * cellSize) / 2.0f;
            float centerY = (currentStage->GetHeight() * cellSize) / 2.0f;
            Vector2 centerPos(centerX, centerY);
            
            float dx = centerX - position.x;
            float dy = centerY - position.y;
            float dist = std::sqrt(dx * dx + dy * dy);
            
            if (dist > cellSize * 2.0f)
            {
                pathfinder.CalculatePath(position, centerPos, currentStage, cellSize);
                Vector2 pathDir = pathfinder.GetMoveDirection(position, status.GetSpeed(), cellSize);
                if (pathDir.x != 0 || pathDir.y != 0) {
                    moveDir = pathDir;
                    facingDir = moveDir;
                    MoveSmart(moveDir);
                } else {
                    moveDir = Vector2(dx / dist, dy / dist);
                    facingDir = moveDir;
                    MoveSmart(moveDir);
                }
                patrolChangeTimer = 60;
            }
            else
            {
                moveToCenterTimer = 0;
            }
        }
        else
        {
            patrolChangeTimer--;
            if (patrolChangeTimer <= 0)
            {
                patrolChangeTimer = 60 + (std::rand() % 90);
                int dirType = std::rand() % 8;
                float angles[] = { 0.0f, 0.7854f, 1.5708f, 2.3562f, 3.1415f, -2.3562f, -1.5708f, -0.7854f };
                float ang = angles[dirType];
                moveDir = Vector2(std::cos(ang), std::sin(ang));
                facingDir = moveDir;
            }
            else if ((std::rand() % 20) == 0)
            {
                float scanOffset = ((std::rand() % 100) / 100.0f - 0.5f) * 1.57f;
                float currAng = std::atan2(moveDir.y, moveDir.x) + scanOffset;
                facingDir = Vector2(std::cos(currAng), std::sin(currAng));
            }

            MoveSmart(moveDir);
        }
    }

    if (m_model)
    {
        float dx = position.x - startPosX;
        float dy = position.y - startPosY;
        if (dx * dx + dy * dy > 0.01f)
        {
            m_model->PlayWalk();
        }
        else
        {
            m_model->PlayIdle();
        }
        m_model->Update(1.0f / 60.0f);
    }
}

void Enemy::Draw()
{
    float screenX = Camera::WorldToScreenX(position.x);
    float screenY = Camera::WorldToScreenY(position.y);
    float renderRadius = radius;

    // カメラ範囲外のカリング
    if (screenX < -100.0f || screenX > 2020.0f || screenY < -100.0f || screenY > 1180.0f)
    {
        return;
    }

    // チーム別カラー（通常：青/赤、バトルロワイヤル錯乱中：全員が敵扱いの紫・マゼンタ・警告オレンジ）
    unsigned int bodyColor;
    if (IsPanic()) {
        // バトルロワイヤル単独孤立化：禍々しいマゼンタ・紫色に変化
        bodyColor = GetColor(220, 30, 160);
        if (damageColorTimer > 0) bodyColor = GetColor(255, 255, 0);
    } else if (teamId == 0) {
        bodyColor = GetColor(30, 30, 180);
        if (damageColorTimer > 0) bodyColor = GetColor(255, 255, 0);
        else if (aiState == EnemyAIState::ALERT) bodyColor = GetColor(40, 40, 240);
        else if (aiState == EnemyAIState::INVESTIGATE) bodyColor = GetColor(30, 140, 230);
    } else {
        bodyColor = GetColor(180, 30, 30);
        if (damageColorTimer > 0) bodyColor = GetColor(255, 255, 0);
        else if (aiState == EnemyAIState::ALERT) bodyColor = GetColor(240, 40, 40);
        else if (aiState == EnemyAIState::INVESTIGATE) bodyColor = GetColor(230, 140, 30);
    }

    // 透明化中の描画制御（完全非表示、または味方なら極薄く表示）
    float opacity = 1.0f;
    if (IsTransparent())
    {
        // 敵側からは完全非表示 (0.0f)
        return;
    }

    // 3Dモデルが存在する場合はモデル描画と足元ガイド円を描画
    if (m_model && m_model->IsLoaded())
    {
        // 足元にチームカラーのガイド円を描画（半透明）
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);
        DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
                   static_cast<int>(renderRadius), bodyColor, TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
                   static_cast<int>(renderRadius), GetColor(255, 255, 255), FALSE);

        // 3Dモデルを描画 (2Dスクリーン座標に合わせて配置・回転)
        m_model->SetScreenPosition(screenX, screenY);
        m_model->SetFacingDir(facingDir);
        m_model->DrawWithOpacity(opacity);
    }
    else
    {
        DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
                   static_cast<int>(renderRadius), bodyColor, TRUE);
        DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
                   static_cast<int>(renderRadius), GetColor(255, 255, 255), FALSE);
    }

    // HomingKill 特攻突撃中の残虐・暴走演出（全身が血のように赤黒く脈動し、頭上に死のカウントダウンと赤黒いオーラ）
    if (isHomingSuicide)
    {
        float timeSec = GetNowCount() / 1000.0f;
        float pulse = std::abs(std::sin(timeSec * 16.0f));

        // 1. 赤黒い禍々しいオーラ
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(140 + pulse * 100));
        DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
                   static_cast<int>(renderRadius + 8.0f + pulse * 6.0f), GetColor(220, 10, 10), FALSE);
        DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
                   static_cast<int>(renderRadius + 4.0f + pulse * 4.0f), GetColor(255, 60, 0), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    }

    // パニック（錯乱）状態の頭上エフェクト描画（回転する紫・マゼンタの星とパルスリング）
    if (IsPanic())
    {
        float timeSec = GetNowCount() / 1000.0f;
        float headY = screenY - renderRadius - 16.0f;

        // 1. 錯乱パルスリング（紫・マゼンタ）
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(120 + std::sin(timeSec * 8.0f) * 60));
        DrawCircle(static_cast<int>(screenX), static_cast<int>(headY), 16, GetColor(255, 50, 220), FALSE);
        DrawCircle(static_cast<int>(screenX), static_cast<int>(headY), 10, GetColor(200, 80, 255), FALSE);

        // 2. 頭上をぐるぐる回転する3つの錯乱スター（黄色〜マゼンタ）
        for (int k = 0; k < 3; ++k)
        {
            float ang = timeSec * 6.0f + (k * 2.094395f); // 120度間隔
            float sx = screenX + std::cos(ang) * 14.0f;
            float sy = headY + std::sin(ang) * 7.0f; // 楕円軌道で回転

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 230);
            DrawCircle(static_cast<int>(sx), static_cast<int>(sy), 4, GetColor(255, 220, 50), TRUE);
            DrawCircle(static_cast<int>(sx), static_cast<int>(sy), 2, GetColor(255, 255, 255), TRUE);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 孤立敵対中（バトルロワイヤル）表示
        DrawString(static_cast<int>(screenX) - 24, static_cast<int>(headY) - 10, "TARGET", GetColor(255, 60, 60));
    }

    // 警戒（ALERT）状態のリング強調および有効射程ガイド円、【赤色でやや透明な弾道予測線】の描画
    if (aiState == EnemyAIState::ALERT)
    {
        DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
                   static_cast<int>(renderRadius + 4.0f), GetColor(255, 80, 80), FALSE);

        // 有効射程を示す円（ガイドライン）描画
        float rangeScreenRadius = GetEffectiveRange() * Camera::ZoomScale;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 50);
        DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
                   static_cast<int>(rangeScreenRadius), GetColor(255, 60, 60), FALSE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 赤色半透明の弾道予測線 (障害物まで伸ばす)
        if (currentStage && cellSize > 0.0f)
        {
            
            float maxRange = GetEffectiveRange();
            float stepDist = cellSize * 0.4f;
            float currDist = radius + 5.0f;
            Vector2 hitPos = Vector2(position.x + facingDir.x * maxRange, position.y + facingDir.y * maxRange);

            while (currDist < maxRange)
            {
                float testX = position.x + facingDir.x * currDist;
                float testY = position.y + facingDir.y * currDist;
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

            // 半透明の赤い弾道予測線
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 115);
            DrawLine(static_cast<int>(screenX), static_cast<int>(screenY),
                     static_cast<int>(hitScreenX), static_cast<int>(hitScreenY),
                     GetColor(255, 40, 40), 2);
            DrawCircle(static_cast<int>(hitScreenX), static_cast<int>(hitScreenY),
                       4, GetColor(255, 80, 80), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
    }
    else if (aiState == EnemyAIState::INVESTIGATE)
    {
        DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
                   static_cast<int>(renderRadius + 3.0f), GetColor(255, 180, 50), FALSE);
    }

    // 敵の視界扇形 (Vision Cone) の透明描画
    if (cellSize > 0.0f)
    {
        float coneDist = GetEffectiveRange() * Camera::ZoomScale;
        float facingAngle = std::atan2(facingDir.y, facingDir.x);
        float halfAngle = 0.3000f; // 17° (左右合計34°)

        // AI状態に応じた視界コーンの色（通常：薄い黄色、警戒：赤橙色）
        unsigned int visionColor = (aiState == EnemyAIState::ALERT) ? GetColor(255, 80, 80) :
                                   (aiState == EnemyAIState::INVESTIGATE) ? GetColor(255, 180, 60) :
                                   GetColor(255, 230, 100);
        int coneAlpha = (aiState == EnemyAIState::ALERT) ? 65 : 40;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, coneAlpha);
        
        // 扇形の端点計算
        int leftX = static_cast<int>(screenX + std::cos(facingAngle - halfAngle) * coneDist);
        int leftY = static_cast<int>(screenY + std::sin(facingAngle - halfAngle) * coneDist);
        int rightX = static_cast<int>(screenX + std::cos(facingAngle + halfAngle) * coneDist);
        int rightY = static_cast<int>(screenY + std::sin(facingAngle + halfAngle) * coneDist);

        // 視界の境界線と先端アーチを描画
        DrawLine(static_cast<int>(screenX), static_cast<int>(screenY), leftX, leftY, visionColor, 2);
        DrawLine(static_cast<int>(screenX), static_cast<int>(screenY), rightX, rightY, visionColor, 2);
        DrawLine(leftX, leftY, rightX, rightY, visionColor, 1);

        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // 向いている方向を示す線を描画 (キョロキョロ視線)
    float lineLen = 30.0f;
    int x1 = static_cast<int>(screenX);
    int y1 = static_cast<int>(screenY);
    int x2 = static_cast<int>(screenX + facingDir.x * lineLen);
    int y2 = static_cast<int>(screenY + facingDir.y * lineLen);

    unsigned int lineCol = (aiState == EnemyAIState::ALERT) ? GetColor(255, 50, 50) : GetColor(230, 160, 100);
    DrawLine(x1, y1, x2, y2, lineCol, 2);
}

#include "SceneManager.h"
#include "Scene.h"

void Enemy::Damage(int amount, const std::string& attackerName, int attackerTeamId)
{
    if (invincibleTimer > 0) return;
    if (!attackerName.empty() || attackerTeamId != -1)
    {
        SetLastAttacker(attackerName, attackerTeamId);
    }
    status.TakeDamage(amount);
    damageColorTimer = 15;
    // 被弾時: パニック中か、あるいは敵チームからの被弾時のみ警戒状態へ移行（混乱解除後の味方の誤射で敵対しない）
    if (IsPanic() || attackerTeamId != this->teamId)
    {
        aiState = EnemyAIState::ALERT;
    }

    if (status.IsDead())
    {
        StopHomingSuicide();
        SetActive(false);
        if (m_model) m_model->PlayDie();
    }
}

void Enemy::StealthKill(const std::string& attackerName, int attackerTeamId)
{
    if (!attackerName.empty() || attackerTeamId != -1)
    {
        SetLastAttacker(attackerName, attackerTeamId);
    }
    StopHomingSuicide();
    status.TakeDamage(status.GetCurrentHp());
    SetActive(false);
    if (m_model) m_model->PlayDie();

    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (scene && scene->GetEffectManager())
    {
        scene->GetEffectManager()->AddBloodEffect(position.x, position.y, 35);
    }
}

void Enemy::OnCollisionEnter(Collider *otherCollider)
{
}

void Enemy::OnCollisionStay(Collider *otherCollider)
{
    // 敵を押し出す処理は廃止（接触時に互いの移動方向を制限する方式に変更）
}

void Enemy::OnCollisionExit(Collider *otherCollider)
{
}

void Enemy::StartHomingSuicide(Character* enemyTarget, float speedMultiplier)
{
    isHomingSuicide = true;
    suicideTimer = 300; // 最長5秒間追尾
    targetCharacter = enemyTarget;
    aiState = EnemyAIState::ALERT;
    status.SetSpeedMultiplier(speedMultiplier);
}

void Enemy::StopHomingSuicide()
{
    isHomingSuicide = false;
    suicideTimer = 0;
    targetCharacter = nullptr;
    status.SetSpeedMultiplier(1.0f);
    aiState = EnemyAIState::PATROL;
}


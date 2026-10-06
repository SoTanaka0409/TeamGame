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
    : Character(ObjectTag::Enemy, startX, startY, 35.0f), damageColorTimer(0),
      currentStage(nullptr), cellSize(1.0f), targetCharacter(nullptr),
      aiState(EnemyAIState::PATROL), facingDir(0.0f, 1.0f), moveDir(0.0f, 1.0f),
      lastKnownPos(startX, startY), patrolChangeTimer(0),
      moveToCenterTimer(300), investigateTimer(0), shootCooldown(0),
      aimDelayTimer(0), strafeDirection(1), strafeTimer(0)
{
    teamId = tId;
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
        collider->SetRadius(35.0f);
        radius = 35.0f;
    }
    collider->SetTag("Enemy");
}

Enemy::~Enemy()
{
}

void Enemy::OnHearGunshot(const Vector2 &soundPos, float loudness, int shooterTeamId)
{
    if (shooterTeamId != -1 && shooterTeamId == this->teamId)
    {
        return;
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

    if (targetCharacter && !targetCharacter->IsActive()) {
        targetCharacter = nullptr;
    }

    float minDist = 999999.0f;
    Character* visibleEnemy = nullptr;

    for (auto obj : scene->GetObjectManager()->GetObjects())
    {
        Character* c = dynamic_cast<Character*>(obj);
        if (c && c != this && c->IsActive() && c->teamId != this->teamId && c->teamId != -1)
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

    if (visibleEnemy) {
        targetCharacter = visibleEnemy;
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

    float maxSightDist = GetSightRange();
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

    if (dist > cellSize * 1.0f)
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
    if (len < 0.0001f) return;

    Vector2 normDir(desiredDir.x / len, desiredDir.y / len);
    float velX = normDir.x * status.GetSpeed();
    float velY = normDir.y * status.GetSpeed();

    if (!currentStage || cellSize <= 0.0f)
    {
        position.x += velX;
        position.y += velY;
        return;
    }

    // X軸の移動と壁衝突
    float nextX = position.x + velX;
    int gridX = static_cast<int>(nextX / cellSize);
    int gridY = static_cast<int>(position.y / cellSize);
    bool xBlocked = currentStage->IsSolidWall(gridX, gridY);

    if (!xBlocked)
    {
        position.x = nextX;
    }

    // Y軸の移動と壁衝突
    float nextY = position.y + velY;
    gridX = static_cast<int>(position.x / cellSize);
    gridY = static_cast<int>(nextY / cellSize);
    bool yBlocked = currentStage->IsSolidWall(gridX, gridY);

    if (!yBlocked)
    {
        position.y = nextY;
    }

    // 正面が壁で引っかかった場合、壁の角を回避するスライド移動を試みる
    if (xBlocked || yBlocked)
    {
        Vector2 slideDir1(normDir.y, -normDir.x);
        Vector2 slideDir2(-normDir.y, normDir.x);

        int slide1X = static_cast<int>((position.x + slideDir1.x * status.GetSpeed()) / cellSize);
        int slide1Y = static_cast<int>((position.y + slideDir1.y * status.GetSpeed()) / cellSize);
        if (!currentStage->IsSolidWall(slide1X, slide1Y))
        {
            position.x += slideDir1.x * (status.GetSpeed() * 0.7f);
            position.y += slideDir1.y * (status.GetSpeed() * 0.7f);
        }
        else
        {
            int slide2X = static_cast<int>((position.x + slideDir2.x * status.GetSpeed()) / cellSize);
            int slide2Y = static_cast<int>((position.y + slideDir2.y * status.GetSpeed()) / cellSize);
            if (!currentStage->IsSolidWall(slide2X, slide2Y))
            {
                position.x += slideDir2.x * (status.GetSpeed() * 0.7f);
                position.y += slideDir2.y * (status.GetSpeed() * 0.7f);
            }
        }
    }

    if (currentStage)
    {
        currentStage->ResolveCollision(position, radius, cellSize);
    }
}

void Enemy::Update()
{
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
            aimDelayTimer = 25; // 初回視認時に約0.4秒のエイム遅延（隙）を発生
        }
        lastKnownPos = targetCharacter->GetPosition();
    }
    else if (aiState == EnemyAIState::ALERT)
    {
        // 視界が切れたら最後に見かけた位置の調査モード(INVESTIGATE)へ移行
        aiState = EnemyAIState::INVESTIGATE;
        investigateTimer = 60; // 1秒間探索してすぐ巡回へ
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

            Vector2 muzzlePos(position.x + facingDir.x * (radius + 5.0f),
                            position.y + facingDir.y * (radius + 5.0f));
            
            new Bullet(muzzlePos.x, muzzlePos.y, fireDir, 14.0f, GetEffectiveRange(), 6.0f, teamId);

            auto scene = SceneManager::GetInstance().GetCurrentScene();
            if (scene && scene->GetEffectManager())
            {
                scene->GetEffectManager()->AddMuzzleFlashEffect(muzzlePos.x, muzzlePos.y, finalAngle, 16.0f);
            }

            SoundManager::GetInstance().Play3D("gunshot", position, 1000.0f, 1.0f, teamId);

            shootCooldown = 75;
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
        }    }
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

    // チーム別カラー（チーム0＝青系、チーム1＝赤系）
    unsigned int bodyColor;
    if (teamId == 0) {
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
    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
               static_cast<int>(renderRadius), bodyColor, TRUE);
    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
               static_cast<int>(renderRadius), GetColor(255, 255, 255), FALSE);

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

void Enemy::Damage(int amount)
{
    status.TakeDamage(amount);
    damageColorTimer = 15;
    aiState = EnemyAIState::ALERT; // 被弾したら即警戒状態

    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (scene && scene->GetEffectManager())
    {
        scene->GetEffectManager()->AddBloodEffect(position.x, position.y, 14);
    }

    if (status.IsDead())
    {
        SetActive(false);
    }
}

void Enemy::StealthKill()
{
    status.TakeDamage(status.GetCurrentHp());
    SetActive(false);

    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (scene && scene->GetEffectManager())
    {
        scene->GetEffectManager()->AddBloodEffect(position.x, position.y, 35);
    }
}

void Enemy::OnCollisionEnter(Collider *otherCollider)
{
    OnCollisionStay(otherCollider);
}

void Enemy::OnCollisionStay(Collider *otherCollider)
{
    if (!otherCollider || !otherCollider->GetOwner() || !otherCollider->GetOwner()->IsActive()) return;

    Object2D *otherObj = otherCollider->GetOwner();
    if (otherObj->GetObjectTag() == ObjectTag::Player || otherObj->GetObjectTag() == ObjectTag::Enemy)
    {
        Vector2 otherPos = otherObj->GetPosition();
        float dx = position.x - otherPos.x;
        float dy = position.y - otherPos.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        float otherRadius = 35.0f;
        if (CircleCollider *c = dynamic_cast<CircleCollider *>(otherCollider))
        {
            otherRadius = c->GetRadius();
        }
        float minDist = collider->GetRadius() + otherRadius; // しっかり重なった時のみ判定

        if (dist < minDist && dist > 0.0001f)
        {
            float overlap = minDist - dist;
            Vector2 pushDir(dx / dist, dy / dist);
            position.x += pushDir.x * (overlap * 0.5f);
            position.y += pushDir.y * (overlap * 0.5f);

            if (currentStage)
            {
                currentStage->ResolveCollision(position, radius, cellSize);
            }
        }
    }
}

void Enemy::OnCollisionExit(Collider *otherCollider)
{
}

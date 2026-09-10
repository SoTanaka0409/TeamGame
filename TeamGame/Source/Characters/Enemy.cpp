#define NOMINMAX
#include "Enemy.h"
#include "DxLib.h"
#include "EnemyBullet.h"
#include "Player.h"
#include "Stage.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

Enemy::Enemy(float startX, float startY)
    : Character(ObjectTag::Enemy, startX, startY, 25.0f), hp(3), damageColorTimer(0),
      currentStage(nullptr), cellSize(1.0f), targetPlayer(nullptr),
      aiState(EnemyAIState::PATROL), facingDir(0.0f, 1.0f), moveDir(0.0f, 1.0f),
      lastKnownPos(startX, startY), patrolChangeTimer(0), investigateTimer(0), shootCooldown(0)
{
    // 【移動速度の低下】 プレイヤー(5.0f)に対し非常に遅い速度 (0.75f)
    speed = 0.75f;
    collider->SetTag("Enemy");
}

Enemy::~Enemy()
{
}

void Enemy::OnHearGunshot(const Vector2 &soundPos)
{
    if (aiState == EnemyAIState::ALERT) return;

    float dx = soundPos.x - position.x;
    float dy = soundPos.y - position.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    // 【聴覚検知低下】 至近距離(4セル以内)の銃声のみ気付く
    if (cellSize > 0.0f && dist < cellSize * 4.0f)
    {
        lastKnownPos = soundPos;
        aiState = EnemyAIState::INVESTIGATE;
        investigateTimer = 60; // 1秒間だけ調査
        if (dist > 0.0001f)
        {
            facingDir = Vector2(dx / dist, dy / dist);
        }
    }
}

bool Enemy::CheckLineOfSightToPlayer() const
{
    if (!targetPlayer || !targetPlayer->IsActive() || !currentStage || cellSize <= 0.0f)
    {
        return false;
    }

    Vector2 pPos = targetPlayer->GetPosition();
    float dx = pPos.x - position.x;
    float dy = pPos.y - position.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    // 【察知能力の低下】 最大視界距離を大幅短縮 (3.5セル分)
    float maxSightDist = cellSize * 3.5f;
    if (dist > maxSightDist)
    {
        return false;
    }

    // 草むら潜伏判定: 草むらの中に居るプレイヤーは超至近距離(1.0セル以内)でしか視認できない
    if (targetPlayer->IsInBush())
    {
        if (dist > cellSize * 1.0f)
        {
            return false;
        }
    }

    // 至近距離(1.0セル以内)以外は視野角判定 (前方34° = 左右17°(0.30rad))
    if (dist > cellSize * 1.0f)
    {
        float facingAngle = std::atan2(facingDir.y, facingDir.x);
        float targetAngle = std::atan2(dy, dx);
        float angleDiff = std::abs(targetAngle - facingAngle);
        while (angleDiff > 3.14159265f)
        {
            angleDiff = std::abs(angleDiff - 2.0f * 3.14159265f);
        }

        if (angleDiff > 0.3000f) // 17°超は視野外
        {
            return false;
        }
    }

    // レイキャスティングによる壁遮蔽チェック (障害物を貫通して見えない)
    if (dist > cellSize * 0.5f)
    {
        int steps = static_cast<int>(dist / (cellSize * 0.5f));
        if (steps < 2) steps = 2;

        float stepX = dx / steps;
        float stepY = dy / steps;

        float currX = position.x + stepX;
        float currY = position.y + stepY;

        for (int i = 1; i < steps; ++i)
        {
            int gX = static_cast<int>(currX / cellSize);
            int gY = static_cast<int>(currY / cellSize);

            if (currentStage->IsOutOfBounds(gX, gY) || currentStage->IsLightBlockingWall(gX, gY))
            {
                return false; // 壁で視界遮断
            }

            currX += stepX;
            currY += stepY;
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
    float velX = normDir.x * speed;
    float velY = normDir.y * speed;

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

        int slide1X = static_cast<int>((position.x + slideDir1.x * speed) / cellSize);
        int slide1Y = static_cast<int>((position.y + slideDir1.y * speed) / cellSize);
        if (!currentStage->IsSolidWall(slide1X, slide1Y))
        {
            position.x += slideDir1.x * (speed * 0.7f);
            position.y += slideDir1.y * (speed * 0.7f);
        }
        else
        {
            int slide2X = static_cast<int>((position.x + slideDir2.x * speed) / cellSize);
            int slide2Y = static_cast<int>((position.y + slideDir2.y * speed) / cellSize);
            if (!currentStage->IsSolidWall(slide2X, slide2Y))
            {
                position.x += slideDir2.x * (speed * 0.7f);
                position.y += slideDir2.y * (speed * 0.7f);
            }
        }
    }
}

void Enemy::Update()
{
    if (damageColorTimer > 0)
    {
        damageColorTimer--;
    }

    if (shootCooldown > 0)
    {
        shootCooldown--;
    }

    // 視界チェック
    bool canSeePlayer = CheckLineOfSightToPlayer();

    if (canSeePlayer)
    {
        aiState = EnemyAIState::ALERT;
        lastKnownPos = targetPlayer->GetPosition();
    }
    else if (aiState == EnemyAIState::ALERT)
    {
        // 視界が切れたら最後に見かけた位置の調査モード(INVESTIGATE)へ移行
        aiState = EnemyAIState::INVESTIGATE;
        investigateTimer = 60; // 1秒間探索してすぐ巡回へ
    }

    if (aiState == EnemyAIState::ALERT && targetPlayer && targetPlayer->IsActive())
    {
        Vector2 pPos = targetPlayer->GetPosition();
        float dx = pPos.x - position.x;
        float dy = pPos.y - position.y;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist > 0.0001f)
        {
            facingDir = Vector2(dx / dist, dy / dist);
        }

        // ゆっくり障害物を避けつつ前進
        MoveSmart(facingDir);

        // 【射撃頻度の緩和】 約2.7秒(160フレーム)ごとにゆっくり発射、弾速も遅い 3.0f
        if (shootCooldown <= 0 && dist < cellSize * 3.5f)
        {
            new EnemyBullet(position.x + facingDir.x * (radius + 5.0f),
                            position.y + facingDir.y * (radius + 5.0f),
                            facingDir, 3.0f);
            shootCooldown = 160;
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
            Vector2 toTarget(dx / dist, dy / dist);
            facingDir = toTarget;
            MoveSmart(toTarget);
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
        // PATROL (のんびり徘徊モード)
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

void Enemy::Draw()
{
    float screenX = position.x;
    float screenY = position.y;
    float renderRadius = radius;

    if (targetPlayer && targetPlayer->IsActive() && cellSize > 0.0f)
    {
        float zoomScale = 75.0f / cellSize;
        Vector2 pPos = targetPlayer->GetPosition();
        screenX = 960.0f + (position.x - pPos.x) * zoomScale;
        screenY = 540.0f + (position.y - pPos.y) * zoomScale;
    }

    // 画面外のカリング
    if (screenX < -100.0f || screenX > 2020.0f || screenY < -100.0f || screenY > 1180.0f)
    {
        return;
    }

    // 本体描画 (被弾時は黄色、警戒時は鮮やかな赤、調査時はオレンジ、通常は暗めの赤)
    unsigned int bodyColor = GetColor(180, 30, 30);
    if (damageColorTimer > 0)
    {
        bodyColor = GetColor(255, 255, 0);
    }
    else if (aiState == EnemyAIState::ALERT)
    {
        bodyColor = GetColor(240, 40, 40);
    }
    else if (aiState == EnemyAIState::INVESTIGATE)
    {
        bodyColor = GetColor(230, 140, 30);
    }

    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
               static_cast<int>(renderRadius), bodyColor, TRUE);
    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
               static_cast<int>(renderRadius), GetColor(255, 255, 255), FALSE);

    // 警戒（ALERT）状態のリング強調および【赤色でやや透明な弾道予測線】の描画
    if (aiState == EnemyAIState::ALERT)
    {
        DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
                   static_cast<int>(renderRadius + 4.0f), GetColor(255, 80, 80), FALSE);

        // 赤色半透明の弾道予測線 (障害物まで伸ばす)
        if (currentStage && cellSize > 0.0f)
        {
            float zoomScale = 75.0f / cellSize;
            Vector2 pPos = targetPlayer->GetPosition();

            float maxRange = cellSize * 8.0f;
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

            float hitScreenX = 960.0f + (hitPos.x - pPos.x) * zoomScale;
            float hitScreenY = 540.0f + (hitPos.y - pPos.y) * zoomScale;

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

    // 向いている方向を示す赤線を描画 (キョロキョロ視線)
    float lineLen = 30.0f;
    int x1 = static_cast<int>(screenX);
    int y1 = static_cast<int>(screenY);
    int x2 = static_cast<int>(screenX + facingDir.x * lineLen);
    int y2 = static_cast<int>(screenY + facingDir.y * lineLen);

    unsigned int lineCol = (aiState == EnemyAIState::ALERT) ? GetColor(255, 50, 50) : GetColor(200, 100, 100);
    DrawLine(x1, y1, x2, y2, lineCol, 2);
}

void Enemy::Damage()
{
    hp--;
    damageColorTimer = 15;
    aiState = EnemyAIState::ALERT; // 被弾したら即警戒状態
    if (hp <= 0)
    {
        SetActive(false);
    }
}

void Enemy::OnCollisionEnter(Collider *otherCollider)
{
}

void Enemy::OnCollisionStay(Collider *otherCollider)
{
}

void Enemy::OnCollisionExit(Collider *otherCollider)
{
}

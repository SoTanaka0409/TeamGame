#include "SceneManager.h"
#include "SoundManager.h"
#include "Scene.h"
#include "Camera.h"
#define NOMINMAX
#include "Enemy.h"
#include "DxLib.h"
#include "EnemyBullet.h"
#include "Player.h"
#include "Stage.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

Enemy::Enemy(float startX, float startY, int tId)
    : Character(ObjectTag::Enemy, startX, startY, 25.0f), damageColorTimer(0),
      currentStage(nullptr), cellSize(1.0f), targetCharacter(nullptr),
      aiState(EnemyAIState::PATROL), facingDir(0.0f, 1.0f), moveDir(0.0f, 1.0f),
      lastKnownPos(startX, startY), patrolChangeTimer(0), investigateTimer(0), shootCooldown(0),
      strafeDirection(1), strafeTimer(0)
{
    // 縲千ｧｻ蜍暮溷ｺｦ縺ｮ菴惹ｸ九・繝励Ξ繧､繝､繝ｼ(5.0f)縺ｫ蟇ｾ縺鈴撼蟶ｸ縺ｫ驕・＞騾溷ｺｦ (0.75f)
    status.Init(3, 0.75f, 1);
    teamId = tId;



    collider->SetTag("Enemy");
}

Enemy::~Enemy()
{
}

#include "ObjectManager.h"

void Enemy::UpdateTarget()
{
    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (!scene || !scene->GetObjectManager()) return;

    float minDist = 999999.0f;
    targetCharacter = nullptr;

    for (auto obj : scene->GetObjectManager()->GetObjects())
    {
        Character* c = dynamic_cast<Character*>(obj);
        if (c && c != this && c->IsActive() && c->teamId != this->teamId && c->teamId != -1)
        {
            float dx = c->GetPosition().x - position.x;
            float dy = c->GetPosition().y - position.y;
            float dist = dx * dx + dy * dy;
            if (dist < minDist)
            {
                minDist = dist;
                targetCharacter = c;
            }
        }
    }
}


void Enemy::OnHearGunshot(const Vector2 &soundPos, float maxDistance)
{
    if (aiState == EnemyAIState::ALERT) return;

    float dx = soundPos.x - position.x;
    float dy = soundPos.y - position.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist < maxDistance)
    {
        float newVolume = 1.0f - (dist / maxDistance); // 0.0(遠い) 〜 1.0(近い)

        if (aiState == EnemyAIState::INVESTIGATE)
        {
            // 距離が近い＝音が大きい。現在追いかけている音の優先度（時間で減衰）より高ければ乗り換える
            if (newVolume > currentInvestigateVolume) {
                lastKnownPos = soundPos;
                investigateTimer = 300;
                currentInvestigateVolume = newVolume;
                if (dist > 0.0001f) {
                    facingDir = Vector2(dx / dist, dy / dist);
                }
            }
        }
        else
        {
            aiState = EnemyAIState::INVESTIGATE;
            lastKnownPos = soundPos;
            investigateTimer = 300;
            currentInvestigateVolume = newVolume;
            if (dist > 0.0001f) {
                facingDir = Vector2(dx / dist, dy / dist);
            }
        }
    }
}

bool Enemy::CheckLineOfSightToTarget() const
{
    if (!targetCharacter || !targetCharacter->IsActive() || !currentStage || cellSize <= 0.0f)
    {
        return false;
    }

    Vector2 pPos = targetCharacter->GetPosition();
    float dx = pPos.x - position.x;
    float dy = pPos.y - position.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    // 縲仙ｯ溽衍閭ｽ蜉帙・菴惹ｸ九・譛螟ｧ隕也阜霍晞屬繧貞､ｧ蟷・洒邵ｮ (3.5繧ｻ繝ｫ蛻・
    float maxSightDist = cellSize * 3.5f;
    if (dist > maxSightDist)
    {
        return false;
    }

    // 闕峨・繧画ｽ應ｼ丞愛螳・ 闕峨・繧峨・荳ｭ縺ｫ螻・ｋ繝励Ξ繧､繝､繝ｼ縺ｯ雜・・霑題ｷ晞屬(1.0繧ｻ繝ｫ莉･蜀・縺ｧ縺励°隕冶ｪ阪〒縺阪↑縺・
    
    // 闕峨・繧画ｽ應ｼ丞愛螳・闕峨・繧峨・荳ｭ縺ｮ逶ｸ謇九・雜・・霑題ｷ晞屬(1.0繧ｻ繝ｫ莉･蜀・縺ｧ縺励°隕冶ｪ阪〒縺阪↑縺・
    Player* pTarget = dynamic_cast<Player*>(targetCharacter);
    if (pTarget && pTarget->IsInBush())

    {
        if (dist > cellSize * 1.0f)
        {
            return false;
        }
    }

    // 閾ｳ霑題ｷ晞屬(1.0繧ｻ繝ｫ莉･蜀・莉･螟悶・隕夜㍽隗貞愛螳・(蜑肴婿34ﾂｰ = 蟾ｦ蜿ｳ17ﾂｰ(0.30rad))
    if (dist > cellSize * 1.0f)
    {
        float facingAngle = std::atan2(facingDir.y, facingDir.x);
        float targetAngle = std::atan2(dy, dx);
        float angleDiff = std::abs(targetAngle - facingAngle);
        while (angleDiff > 3.14159265f)
        {
            angleDiff = std::abs(angleDiff - 2.0f * 3.14159265f);
        }

        if (angleDiff > 0.3000f) // 17ﾂｰ雜・・隕夜㍽螟・
        {
            return false;
        }
    }

    // 繝ｬ繧､繧ｭ繝｣繧ｹ繝・ぅ繝ｳ繧ｰ縺ｫ繧医ｋ螢・・阡ｽ繝√ぉ繝・け (髫懷ｮｳ迚ｩ繧定ｲｫ騾壹＠縺ｦ隕九∴縺ｪ縺・
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
                return false; // 螢√〒隕也阜驕ｮ譁ｭ
            }

            currX += stepX;
            currY += stepY;
        }
    }

    return true;
}

// 縲占ｳ｢縺・リ繝薙ご繝ｼ繧ｷ繝ｧ繝ｳ縲・螢∬ｧ偵↓蠑輔▲縺九°繧峨★貊代ｉ縺九↓蝗樣∩繝ｻ繧ｹ繝ｩ繧､繝臥ｧｻ蜍輔☆繧矩未謨ｰ
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

    // X霆ｸ縺ｮ遘ｻ蜍輔→螢∬｡晉ｪ・
    float nextX = position.x + velX;
    int gridX = static_cast<int>(nextX / cellSize);
    int gridY = static_cast<int>(position.y / cellSize);
    bool xBlocked = currentStage->IsSolidWall(gridX, gridY);

    if (!xBlocked)
    {
        position.x = nextX;
    }

    // Y霆ｸ縺ｮ遘ｻ蜍輔→螢∬｡晉ｪ・
    float nextY = position.y + velY;
    gridX = static_cast<int>(position.x / cellSize);
    gridY = static_cast<int>(nextY / cellSize);
    bool yBlocked = currentStage->IsSolidWall(gridX, gridY);

    if (!yBlocked)
    {
        position.y = nextY;
    }

    // 豁｣髱｢縺悟｣√〒蠑輔▲縺九°縺｣縺溷ｴ蜷医∝｣√・隗偵ｒ蝗樣∩縺吶ｋ繧ｹ繝ｩ繧､繝臥ｧｻ蜍輔ｒ隧ｦ縺ｿ繧・
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
}

void Enemy::Update()
{
    if (invincibleTimer > 0) invincibleTimer--;
    UpdateTarget();

    if (damageColorTimer > 0)
    {
        damageColorTimer--;
    }

    if (shootCooldown > 0)
    {
        shootCooldown--;
    }

    // 隕也阜繝√ぉ繝・け
    bool canSeePlayer = CheckLineOfSightToTarget();

    if (canSeePlayer)
    {
        aiState = EnemyAIState::ALERT;
        lastKnownPos = targetCharacter->GetPosition();
    }
    else if (aiState == EnemyAIState::ALERT)
    {
        // 隕也阜縺悟・繧後◆繧画怙蠕後↓隕九°縺代◆菴咲ｽｮ縺ｮ隱ｿ譟ｻ繝｢繝ｼ繝・INVESTIGATE)縺ｸ遘ｻ陦・
        aiState = EnemyAIState::INVESTIGATE;
        investigateTimer = 60; // 1遘帝俣謗｢邏｢縺励※縺吶＄蟾｡蝗槭∈
    }

    if (aiState == EnemyAIState::ALERT && targetCharacter && targetCharacter->IsActive())
    {
        Vector2 pPos = targetCharacter->GetPosition();
        float dx = pPos.x - position.x;
        float dy = pPos.y - position.y;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist > 0.0001f)
        {
            facingDir = Vector2(dx / dist, dy / dist);
        }

        // 繧ｫ繝区ｭｩ縺搾ｼ医せ繝医Ξ繧､繝包ｼ峨・譁ｹ蜷題ｻ｢謠帙ち繧､繝槭・
        strafeTimer--;
        if (strafeTimer <= 0)
        {
            strafeDirection = (rand() % 2 == 0) ? 1 : -1;
            strafeTimer = 60 + (rand() % 60); // 1縲・遘偵＃縺ｨ縺ｫ譁ｹ蜷題ｻ｢謠・
        }

        // 謾ｻ謦・Δ繝ｼ繧ｷ繝ｧ繝ｳ・亥ｰ・茶逶ｴ蜑搾ｼ峨↓蜈･縺｣縺溘ｉ霑代▼縺上・繧偵ｄ繧√ｋ
        if (shootCooldown < 30)
        {
            // 謦・▽逶ｴ蜑阪・蟾ｦ蜿ｳ縺ｫ縺縺大虚縺擾ｼ医き繝区ｭｩ縺搾ｼ・
            Vector2 strafeDir(-facingDir.y * strafeDirection, facingDir.x * strafeDirection);
            MoveSmart(strafeDir);
        }
        else
        {
            // 繧ｯ繝ｼ繝ｫ繝繧ｦ繝ｳ荳ｭ縺ｯ縲・□縺代ｌ縺ｰ霑代▼縺阪▽縺､蟾ｦ蜿ｳ縺ｫ蜍輔″縲∬ｿ代￠繧後・蟾ｦ蜿ｳ縺ｮ縺ｿ縺ｫ蜍輔￥
            if (dist > cellSize * 2.5f)
            {
                // 蜑埼ｲ ・・蟾ｦ蜿ｳ遘ｻ蜍包ｼ医ず繧ｰ繧ｶ繧ｰ遘ｻ蜍包ｼ・
                Vector2 approachAndStrafe(facingDir.x + (-facingDir.y * strafeDirection * 0.5f),
                                          facingDir.y + (facingDir.x * strafeDirection * 0.5f));
                MoveSmart(approachAndStrafe);
            }
            else
            {
                // 蜊∝・霑代￠繧後・蟾ｦ蜿ｳ遘ｻ蜍輔・縺ｿ
                Vector2 strafeDir(-facingDir.y * strafeDirection, facingDir.x * strafeDirection);
                MoveSmart(strafeDir);
            }
        }

        // 縲千匱遐ｲ鬆ｻ蠎ｦ縺ｮ邱ｩ蜥後・邏・.7遘・160繝輔Ξ繝ｼ繝)縺斐→縺ｫ繧・▲縺上ｊ逋ｺ遐ｲ縲∝ｼｾ騾溘ｂ驕・＞ 3.0f
        if (shootCooldown <= 0 && dist < cellSize * 3.5f)
        {
            auto scene = SceneManager::GetInstance().GetCurrentScene();
            if (scene && scene->GetObjectManager())
            {
                // Bots add random spread (simulating inaccuracy)
                float randAngle = (((float)std::rand() / RAND_MAX) * 20.0f - 10.0f) * (3.14159f / 180.0f);
                float baseAngle = std::atan2(facingDir.y, facingDir.x);
                float finalAngle = baseAngle + randAngle;
                Vector2 finalDir(std::cos(finalAngle), std::sin(finalAngle));

                EnemyBullet* eBullet = new EnemyBullet(position.x + facingDir.x * (radius + 5.0f),
                                position.y + finalDir.y * (radius + 5.0f), // Wait, finalDir.y
                                finalDir, 3.0f, this->teamId);
                // scene->GetObjectManager()->AddObject(eBullet);
                SoundManager::GetInstance().Play3D("enemy_gunshot", position, 1000.0f, 1.0f, this->teamId);
            }
            shootCooldown = 160;
        }
    }
    else if (aiState == EnemyAIState::INVESTIGATE)
    {
        investigateTimer--;
        currentInvestigateVolume -= (1.0f / 300.0f);
        if (currentInvestigateVolume < 0.0f) currentInvestigateVolume = 0.0f;
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
                aiState = EnemyAIState::PATROL; // 繝励Ξ繧､繝､繝ｼ縺瑚ｦ九▽縺九ｉ縺ｪ縺代ｌ縺ｰ蟾｡蝗槭↓謌ｻ繧・
            }
        }
    }
    else
    {
        // PATROL (縺ｮ繧薙・繧雁ｾ伜ｾ翫Δ繝ｼ繝・
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
    float screenX = Camera::WorldToScreenX(position.x);
    float screenY = Camera::WorldToScreenY(position.y);
    float renderRadius = radius;

    // 逕ｻ髱｢螟悶・繧ｫ繝ｪ繝ｳ繧ｰ
    if (screenX < -100.0f || screenX > 2020.0f || screenY < -100.0f || screenY > 1180.0f)
    {
        return;
    }

    // 譛ｬ菴捺緒逕ｻ (陲ｫ蠑ｾ譎ゅ・鮟・牡縲∬ｭｦ謌呈凾縺ｯ魄ｮ繧・°縺ｪ襍､縲∬ｪｿ譟ｻ譎ゅ・繧ｪ繝ｬ繝ｳ繧ｸ縲・壼ｸｸ縺ｯ證励ａ縺ｮ襍､)
    // 繝√・繝縺ｫ蠢懊§縺溘・繝ｼ繧ｹ濶ｲ
    unsigned int bodyColor = (teamId == 0) ? GetColor(30, 80, 180) : GetColor(180, 30, 30);
    
    if (damageColorTimer > 0)
    {
        bodyColor = GetColor(255, 255, 0);
    }
    else if (aiState == EnemyAIState::ALERT)
    {
        bodyColor = (teamId == 0) ? GetColor(40, 100, 240) : GetColor(240, 40, 40);
    }
    else if (aiState == EnemyAIState::INVESTIGATE)
    {
        bodyColor = (teamId == 0) ? GetColor(30, 140, 230) : GetColor(230, 140, 30);
    }

    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
               static_cast<int>(renderRadius), bodyColor, TRUE);
    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
               static_cast<int>(renderRadius), GetColor(255, 255, 255), FALSE);

    // 隴ｦ謌抵ｼ・LERT・臥憾諷九・繝ｪ繝ｳ繧ｰ蠑ｷ隱ｿ縺翫ｈ縺ｳ縲占ｵ､濶ｲ縺ｧ繧・ｄ騾乗・縺ｪ蠑ｾ驕謎ｺ域ｸｬ邱壹代・謠冗判
    if (aiState == EnemyAIState::ALERT)
    {
        DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
                   static_cast<int>(renderRadius + 4.0f), GetColor(255, 80, 80), FALSE);

        // 襍､濶ｲ蜊企乗・縺ｮ蠑ｾ驕謎ｺ域ｸｬ邱・(髫懷ｮｳ迚ｩ縺ｾ縺ｧ莨ｸ縺ｰ縺・
        if (currentStage && cellSize > 0.0f)
        {
            
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

            float hitScreenX = Camera::WorldToScreenX(hitPos.x);
            float hitScreenY = Camera::WorldToScreenY(hitPos.y);

            // 蜊企乗・縺ｮ襍､縺・ｼｾ驕謎ｺ域ｸｬ邱・
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

    // 蜷代＞縺ｦ縺・ｋ譁ｹ蜷代ｒ遉ｺ縺呵ｵ､邱壹ｒ謠冗判 (繧ｭ繝ｧ繝ｭ繧ｭ繝ｧ繝ｭ隕也ｷ・
    float lineLen = 30.0f;
    int x1 = static_cast<int>(screenX);
    int y1 = static_cast<int>(screenY);
    int x2 = static_cast<int>(screenX + facingDir.x * lineLen);
    int y2 = static_cast<int>(screenY + facingDir.y * lineLen);

    unsigned int lineCol = (aiState == EnemyAIState::ALERT) ? GetColor(255, 50, 50) : GetColor(200, 100, 100);
    DrawLine(x1, y1, x2, y2, lineCol, 2);
}

#include "SceneManager.h"
#include "Scene.h"

void Enemy::Damage()
{
    if (invincibleTimer > 0) return;
    status.TakeDamage(1);
    damageColorTimer = 15;
    aiState = EnemyAIState::ALERT; // 陲ｫ蠑ｾ縺励◆繧牙叉隴ｦ謌堤憾諷・

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
    if (invincibleTimer > 0) return;
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
}

void Enemy::OnCollisionStay(Collider *otherCollider)
{
}

void Enemy::OnCollisionExit(Collider *otherCollider)
{
}


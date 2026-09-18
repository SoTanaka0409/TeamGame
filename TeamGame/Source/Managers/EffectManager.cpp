#include "Camera.h"
#include "EffectManager.h"
#include "DxLib.h"
#include "ObjectManager.h"
#include "Player.h"
#include "Scene.h"
#include "SceneManager.h"
#include <cmath>
#include <cstdlib>

void BloodParticle::Update()
{
    Effect::Update();
    x += vx;
    y += vy;
    vx *= 0.90f;
    vy *= 0.90f;
}

void BloodParticle::Draw()
{
    float screenX = x;
    float screenY = y;
    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (scene && scene->GetObjectManager())
    {
        for (auto obj : scene->GetObjectManager()->GetObjects())
        {
            Player *player = dynamic_cast<Player *>(obj);
            if (player && player->IsActive())
            {
                float zoomScale = 75.0f / 40.0f;
                Vector2 pPos = player->GetPosition();
                screenX = 960.0f + (x - pPos.x) * zoomScale;
                screenY = 540.0f + (y - pPos.y) * zoomScale;
                break;
            }
        }
    }

    if (screenX < -50.0f || screenX > 1970.0f || screenY < -50.0f || screenY > 1130.0f)
        return;

    int alpha = static_cast<int>(255.0f * (static_cast<float>(lifeTimer) / maxLife));
    if (alpha <= 0) return;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY), static_cast<int>(size), color, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void KnifeSlashEffect::Draw()
{
    float screenX = x;
    float screenY = y;
    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (scene && scene->GetObjectManager())
    {
        for (auto obj : scene->GetObjectManager()->GetObjects())
        {
            Player *player = dynamic_cast<Player *>(obj);
            if (player && player->IsActive())
            {
                float zoomScale = 75.0f / 40.0f;
                Vector2 pPos = player->GetPosition();
                screenX = 960.0f + (x - pPos.x) * zoomScale;
                screenY = 540.0f + (y - pPos.y) * zoomScale;
                break;
            }
        }
    }

    float arcLength = 40.0f;
    float cosA = std::cos(angle);
    float sinA = std::sin(angle);
    float perpX = -sinA * arcLength;
    float perpY = cosA * arcLength;

    int x1 = static_cast<int>(screenX - perpX);
    int y1 = static_cast<int>(screenY - perpY);
    int x2 = static_cast<int>(screenX + perpX);
    int y2 = static_cast<int>(screenY + perpY);

    int alpha = static_cast<int>(255.0f * (static_cast<float>(lifeTimer) / maxLife));
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    DrawLine(x1, y1, x2, y2, GetColor(255, 255, 255), 5);
    DrawLine(x1, y1, x2, y2, GetColor(255, 40, 40), 2);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void SparkParticle::Update()
{
    Effect::Update();
    x += vx;
    y += vy;
    vx *= 0.85f;
    vy *= 0.85f;
}

void SparkParticle::Draw()
{
    float screenX = Camera::WorldToScreenX(x);
    float screenY = Camera::WorldToScreenY(y);

    if (screenX < -50.0f || screenX > 1970.0f || screenY < -50.0f || screenY > 1130.0f)
        return;

    int alpha = static_cast<int>(255.0f * (static_cast<float>(lifeTimer) / maxLife));
    if (alpha <= 0) return;

    // 火花の尾引き（移動方向への伸び）
    float zoom = Camera::ZoomScale;
    float tailX = screenX - vx * 2.5f * zoom;
    float tailY = screenY - vy * 2.5f * zoom;

    // 加算ブレンドでリアルな発光感を表現
    SetDrawBlendMode(DX_BLENDMODE_ADD, alpha);

    // 火花の線（尾引き）
    DrawLine(static_cast<int>(tailX), static_cast<int>(tailY),
             static_cast<int>(screenX), static_cast<int>(screenY),
             color, (size > 1.5f) ? 2 : 1);

    // 火花先端の輝き
    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
               static_cast<int>(size), GetColor(255, 255, 220), TRUE);

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void MuzzleFlashEffect::Draw()
{
    float screenX = Camera::WorldToScreenX(x);
    float screenY = Camera::WorldToScreenY(y);

    if (screenX < -50.0f || screenX > 1970.0f || screenY < -50.0f || screenY > 1130.0f)
        return;

    float progress = static_cast<float>(lifeTimer) / maxLife; // 1.0 -> 0.0
    int alpha = static_cast<int>(255.0f * progress);
    if (alpha <= 0) return;

    float currentRadius = flashRadius * (0.6f + 0.4f * progress);

    // 撃った人の周りのみの局所的な環境光フラッシュオーラ（加算合成）
    float auraRadius = 220.0f * (0.7f + 0.3f * progress);
    SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(alpha * 0.35f));
    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY), static_cast<int>(auraRadius), GetColor(255, 160, 50), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(alpha * 0.60f));
    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY), static_cast<int>(auraRadius * 0.5f), GetColor(255, 220, 120), TRUE);

    // 加算合成でまばゆいマズルフラッシュ
    SetDrawBlendMode(DX_BLENDMODE_ADD, alpha);
    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY), static_cast<int>(currentRadius * 1.8f), GetColor(255, 120, 20), TRUE);
    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY), static_cast<int>(currentRadius), GetColor(255, 220, 100), TRUE);
    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY), static_cast<int>(currentRadius * 0.5f), GetColor(255, 255, 255), TRUE);

    // マズルフラッシュ扇形コーン（発砲方向）
    float cosA = std::cos(angle);
    float sinA = std::sin(angle);
    float coneLength = currentRadius * 2.5f;
    float coneWidth = currentRadius * 1.2f;

    float tipX = screenX + cosA * coneLength;
    float tipY = screenY + sinA * coneLength;

    float perpX = -sinA * (coneWidth * 0.5f);
    float perpY = cosA * (coneWidth * 0.5f);

    int x1 = static_cast<int>(screenX + perpX);
    int y1 = static_cast<int>(screenY + perpY);
    int x2 = static_cast<int>(screenX - perpX);
    int y2 = static_cast<int>(screenY - perpY);
    int x3 = static_cast<int>(tipX);
    int y3 = static_cast<int>(tipY);

    DrawTriangle(x1, y1, x2, y2, x3, y3, GetColor(255, 240, 150), TRUE);

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

EffectManager::EffectManager()
{
}

EffectManager::~EffectManager()
{
    Clear();
}

void EffectManager::AddEffect(Effect *effect)
{
    effects.push_back(effect);
}

void EffectManager::AddBloodEffect(float worldX, float worldY, int count)
{
    for (int i = 0; i < count; ++i)
    {
        float angle = (static_cast<float>(rand()) / RAND_MAX) * 6.2831853f;
        float speed = 1.5f + (static_cast<float>(rand()) / RAND_MAX) * 5.5f;
        float vx = std::cos(angle) * speed;
        float vy = std::sin(angle) * speed;
        float pSize = 2.5f + (static_cast<float>(rand()) / RAND_MAX) * 4.0f;
        int life = 18 + rand() % 22;

        int colorType = rand() % 3;
        unsigned int col = GetColor(180, 0, 0);
        if (colorType == 1) col = GetColor(230, 20, 20);
        else if (colorType == 2) col = GetColor(120, 0, 0);

        AddEffect(new BloodParticle(worldX, worldY, vx, vy, pSize, life, col));
    }
}

void EffectManager::AddKnifeSlashEffect(float worldX, float worldY, float dirAngle)
{
    AddEffect(new KnifeSlashEffect(worldX, worldY, dirAngle));
}

void EffectManager::AddMuzzleFlashEffect(float worldX, float worldY, float dirAngle, float radius)
{
    AddEffect(new MuzzleFlashEffect(worldX, worldY, dirAngle, radius));

    // 飛び散るリアルな火花（スパーク）粒子の多数生成
    int sparkCount = 12;
    for (int i = 0; i < sparkCount; ++i)
    {
        // 扇状の広い散乱（前方に広がるように）
        float spread = ((static_cast<float>(rand()) / RAND_MAX) - 0.5f) * 1.2f; // 約±34度
        float speed = 4.0f + (static_cast<float>(rand()) / RAND_MAX) * 10.0f;
        float sparkAngle = dirAngle + spread;
        float vx = std::cos(sparkAngle) * speed;
        float vy = std::sin(sparkAngle) * speed;
        float pSize = 1.2f + (static_cast<float>(rand()) / RAND_MAX) * 2.2f;
        int life = 5 + rand() % 8;

        // 火花グラデーション（白熱色〜黄〜オレンジ〜赤）
        int colorType = rand() % 4;
        unsigned int col = GetColor(255, 255, 220); // 白熱
        if (colorType == 1) col = GetColor(255, 210, 80);  // 金黄
        else if (colorType == 2) col = GetColor(255, 140, 30); // オレンジ
        else if (colorType == 3) col = GetColor(255, 80, 20);  // 赤熱

        AddEffect(new SparkParticle(worldX, worldY, vx, vy, pSize, life, col));
    }

    // 発砲の瞬間に撃った人の周囲のみを一瞬明るくフラッシュ（5フレーム間）
    TriggerGunFlash(worldX, worldY, 5);
}

void EffectManager::Update()
{
    if (gunFlashTimer > 0)
    {
        gunFlashTimer--;
    }

    auto it = effects.begin();
    while (it != effects.end())
    {
        (*it)->Update();
        if ((*it)->IsDead())
        {
            delete *it;
            it = effects.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void EffectManager::Draw()
{
    for (auto eff : effects)
    {
        eff->Draw();
    }
}

void EffectManager::Clear()
{
    for (auto eff : effects)
    {
        delete eff;
    }
    effects.clear();
}


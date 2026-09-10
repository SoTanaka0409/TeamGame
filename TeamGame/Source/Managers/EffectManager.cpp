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

void EffectManager::Update()
{
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


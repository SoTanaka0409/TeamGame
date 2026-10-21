#include "ColliderManager.h"
#include "Object2D.h"
#include "Camera.h"
#include "CircleCollider.h"
#include "RectCollider.h"
#include "DxLib.h"

ColliderManager::ColliderManager()
{
}

ColliderManager::~ColliderManager()
{
    Clear();
}

void ColliderManager::AddCollider(Collider *collider)
{
    colliders.push_back(collider);
}

void ColliderManager::RemoveCollider(Collider *collider)
{
    if (!collider) return;

    auto it = colliders.begin();
    while (it != colliders.end())
    {
        if (*it == collider)
        {
            it = colliders.erase(it);
            break;
        }
        else
        {
            ++it;
        }
    }

    auto itPrev = previousCollisions.begin();
    while (itPrev != previousCollisions.end())
    {
        if (itPrev->first == collider || itPrev->second == collider)
        {
            itPrev = previousCollisions.erase(itPrev);
        }
        else
        {
            ++itPrev;
        }
    }
}

void ColliderManager::CheckAllCollisions()
{
    std::set<std::pair<Collider *, Collider *>> currentCollisions;

    for (size_t i = 0; i < colliders.size(); ++i)
    {
        if (!colliders[i]) continue;
        for (size_t j = i + 1; j < colliders.size(); ++j)
        {
            if (!colliders[j]) continue;
            Object2D *ownerA = colliders[i]->GetOwner();
            Object2D *ownerB = colliders[j]->GetOwner();

            if (!ownerA || !ownerB || !ownerA->IsActive() ||
                !ownerB->IsActive())
                continue;

            if (colliders[i]->IsCollision(colliders[j]))
            {
                auto pair = (colliders[i] < colliders[j])
                                ? std::make_pair(colliders[i], colliders[j])
                                : std::make_pair(colliders[j], colliders[i]);
                currentCollisions.insert(pair);
            }
        }
    }

    for (auto &pair : currentCollisions)
    {
        bool isEnter =
            (previousCollisions.find(pair) == previousCollisions.end());

        Collider *colA = pair.first;
        Collider *colB = pair.second;
        Object2D *objA = colA->GetOwner();
        Object2D *objB = colB->GetOwner();

        if (!objA || !objB)
            continue;

        if (isEnter)
        {
            if (objA->IsActive())
                objA->OnCollisionEnter(colB);
            if (objB->IsActive())
                objB->OnCollisionEnter(colA);
        }
        else
        {
            if (objA->IsActive())
                objA->OnCollisionStay(colB);
            if (objB->IsActive())
                objB->OnCollisionStay(colA);
        }
    }

    for (auto &pair : previousCollisions)
    {
        if (currentCollisions.find(pair) == currentCollisions.end())
        {
            Collider *colA = pair.first;
            Collider *colB = pair.second;
            Object2D *objA = colA->GetOwner();
            Object2D *objB = colB->GetOwner();

            if (objA && objA->IsActive())
                objA->OnCollisionExit(colB);
            if (objB && objB->IsActive())
                objB->OnCollisionExit(colA);
        }
    }

    previousCollisions = currentCollisions;
}

void ColliderManager::Clear()
{
    colliders.clear();
    previousCollisions.clear();
}

void ColliderManager::DrawDebug()
{
    for (Collider *col : colliders)
    {
        if (!col) continue;
        Object2D *owner = col->GetOwner();
        if (!owner || !owner->IsActive()) continue;

        // CircleCollider の描画
        CircleCollider *circle = dynamic_cast<CircleCollider *>(col);
        if (circle)
        {
            float worldX = circle->GetWorldX();
            float worldY = circle->GetWorldY();
            float screenX = Camera::WorldToScreenX(worldX);
            float screenY = Camera::WorldToScreenY(worldY);
            float screenRadius = circle->GetRadius() * Camera::ZoomScale;

            // 画面外カリング
            if (screenX + screenRadius < -50.0f || screenX - screenRadius > 1970.0f ||
                screenY + screenRadius < -50.0f || screenY - screenRadius > 1130.0f)
            {
                continue;
            }

            unsigned int colColor = GetColor(255, 255, 255);
            switch (owner->GetObjectTag())
            {
            case ObjectTag::Player:
                colColor = GetColor(0, 255, 255); // シアン
                break;
            case ObjectTag::Enemy:
                colColor = GetColor(255, 60, 60);  // 赤
                break;
            case ObjectTag::PlayerWeapon:
                colColor = GetColor(255, 255, 0); // 黄色
                break;
            case ObjectTag::EnemyWeapon:
                colColor = GetColor(255, 140, 0); // オレンジ
                break;
            default:
                colColor = GetColor(180, 255, 180); // 薄緑
                break;
            }

            int sx = static_cast<int>(screenX);
            int sy = static_cast<int>(screenY);
            int sr = static_cast<int>(screenRadius);

            // 1. 半透明塗りつぶし円
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 70);
            DrawCircle(sx, sy, sr, colColor, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            // 2. 輪郭線
            DrawCircle(sx, sy, sr, colColor, FALSE);

            // 3. 中心十字
            DrawLine(sx - 4, sy, sx + 4, sy, colColor, 2);
            DrawLine(sx, sy - 4, sx, sy + 4, colColor, 2);

            // 4. 情報テキスト（半径とタグ）
            DrawFormatString(sx - sr, sy - sr - 14, colColor, "R:%.1f (%s)", circle->GetRadius(), col->GetTag().c_str());
        }
        else
        {
            // RectCollider の描画
            RectCollider *rect = dynamic_cast<RectCollider *>(col);
            if (rect)
            {
                float worldX = rect->GetWorldX();
                float worldY = rect->GetWorldY();
                float screenX = Camera::WorldToScreenX(worldX);
                float screenY = Camera::WorldToScreenY(worldY);
                float screenW = rect->GetWidth() * Camera::ZoomScale;
                float screenH = rect->GetHeight() * Camera::ZoomScale;

                unsigned int colColor = GetColor(255, 255, 0);
                int sx1 = static_cast<int>(screenX - screenW / 2.0f);
                int sy1 = static_cast<int>(screenY - screenH / 2.0f);
                int sx2 = static_cast<int>(screenX + screenW / 2.0f);
                int sy2 = static_cast<int>(screenY + screenH / 2.0f);

                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 70);
                DrawBox(sx1, sy1, sx2, sy2, colColor, TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                DrawBox(sx1, sy1, sx2, sy2, colColor, FALSE);
            }
        }
    }
}


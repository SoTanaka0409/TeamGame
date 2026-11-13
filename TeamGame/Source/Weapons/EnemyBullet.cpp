#include <cmath>
#include "Camera.h"
#include "EnemyBullet.h"
#include "ColliderManager.h"
#include "DxLib.h"
#include "Player.h"
#include "Scene.h"
#include "SceneManager.h"
#include "Stage.h"
#include "ObjectManager.h"

/**
 * @brief EnemyBulletのコンストラクタ
 * @param startX 初期位置X
 * @param startY 初期位置Y
 * @param dir 進行方向
 * @param speed 弾の速度
 * @param range 最大射程距離
 * @details 初期位置や速度を設定し、敵の弾専用の円形コライダーを作成してコライダーマネージャーに登録する
 */
EnemyBullet::EnemyBullet(float startX, float startY, const Vector2 &dir, float speed, float range)
    : myColliderManager(nullptr), radius(6.0f), maxRange(range), startPos(startX, startY)
{
    position = Vector2(startX, startY);
    width = radius * 2.0f;
    height = radius * 2.0f;
    velocity = Vector2(dir.x * speed, dir.y * speed);

    collider = new CircleCollider(this, radius, "EnemyBullet");

    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (scene)
    {
        myColliderManager = scene->GetColliderManager();
        if (myColliderManager)
        {
            myColliderManager->AddCollider(collider);
        }
    }
}

/**
 * @brief EnemyBulletのデストラクタ
 * @details 登録したコライダーをコライダーマネージャーから削除し、メモリを解放する
 */
EnemyBullet::~EnemyBullet()
{
    if (myColliderManager)
    {
        myColliderManager->RemoveCollider(collider);
    }
    delete collider;
}

/**
 * @brief 毎フレームの敵の弾の更新処理 (連続衝突判定 CCD & サブステップ分割)
 * @details 高速弾の壁・プレイヤー飛び越えを微小ステップと線分判定で防ぎ、確実な着弾・壁ヒットを実現
 */
void EnemyBullet::Update()
{
    auto scene = SceneManager::GetInstance().GetCurrentScene();
    const Stage *stage = (scene) ? scene->GetStage() : nullptr;
    const float cellSize = 40.0f;

    // 1フレームの総移動ベクトルと移動距離
    float totalMoveDist = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
    if (totalMoveDist < 0.0001f)
    {
        return;
    }

    // トンネリング（すり抜け）を完全に防止するためのサブステップ分割
    const float stepSize = 4.0f;
    int numSteps = static_cast<int>(std::ceil(totalMoveDist / stepSize));
    if (numSteps < 1) numSteps = 1;
    Vector2 stepVelocity = Vector2(velocity.x / static_cast<float>(numSteps), velocity.y / static_cast<float>(numSteps));
    Vector2 dir(velocity.x / totalMoveDist, velocity.y / totalMoveDist);

    for (int s = 0; s < numSteps; ++s)
    {
        Vector2 p0 = position;
        Vector2 p1 = Vector2(p0.x + stepVelocity.x, p0.y + stepVelocity.y);

        // ① 射程距離制限チェック (最大射程を超えたら消滅)
        float dx = p1.x - startPos.x;
        float dy = p1.y - startPos.y;
        if (dx * dx + dy * dy > maxRange * maxRange)
        {
            position = p1;
            SetActive(false);
            return;
        }

        // ② 画面外に出たら消滅
        if (p1.x < -200 || p1.x > 2100 || p1.y < -200 || p1.y > 1300)
        {
            position = p1;
            SetActive(false);
            return;
        }

        // ③ 壁・障害物判定 (厚み40pxの壁を絶対に飛び越えない)
        bool hitWall = false;
        if (stage)
        {
            int gX = static_cast<int>(p1.x / cellSize);
            int gY = static_cast<int>(p1.y / cellSize);
            if (stage->IsSolidWall(gX, gY))
            {
                hitWall = true;
            }
            else
            {
                int tipGX = static_cast<int>((p1.x + dir.x * radius) / cellSize);
                int tipGY = static_cast<int>((p1.y + dir.y * radius) / cellSize);
                if (stage->IsSolidWall(tipGX, tipGY))
                {
                    hitWall = true;
                }
            }
        }

        // ④ プレイヤーとの連続衝突判定 (CCD)
        Player* hitPlayer = nullptr;
        Vector2 bestHitPos = p1;
        float minT = 2.0f;

        if (scene && scene->GetObjectManager())
        {
            Vector2 seg = Vector2(p1.x - p0.x, p1.y - p0.y);
            float segLenSq = seg.x * seg.x + seg.y * seg.y;

            for (auto obj : scene->GetObjectManager()->GetObjects())
            {
                if (!obj || !obj->IsActive()) continue;
                Player* p = dynamic_cast<Player*>(obj);
                if (!p || p->status.IsDead()) continue;

                Vector2 cPos = p->GetPosition();
                float cRadius = p->GetRadius();
                float totalR = radius + cRadius;

                float t = 0.0f;
                if (segLenSq > 0.0001f)
                {
                    t = ((cPos.x - p0.x) * seg.x + (cPos.y - p0.y) * seg.y) / segLenSq;
                    if (t < 0.0f) t = 0.0f;
                    else if (t > 1.0f) t = 1.0f;
                }

                Vector2 closest(p0.x + t * seg.x, p0.y + t * seg.y);
                float distSq = (cPos.x - closest.x) * (cPos.x - closest.x) + (cPos.y - closest.y) * (cPos.y - closest.y);

                if (distSq <= totalR * totalR)
                {
                    // 壁の向こう側にいるプレイヤーには当たらない
                    if (stage && stage->IsSolidWall(static_cast<int>(closest.x / cellSize), static_cast<int>(closest.y / cellSize)))
                    {
                        continue;
                    }

                    if (t < minT)
                    {
                        minT = t;
                        hitPlayer = p;
                        bestHitPos = closest;
                    }
                }
            }
        }

        if (hitPlayer)
        {
            position = bestHitPos;
            ApplyHit(hitPlayer, bestHitPos);
            return;
        }
        else if (hitWall)
        {
            position = p1;
            SetActive(false);
            return;
        }

        position = p1;
    }
}

/**
 * @brief 敵の弾の描画処理
 * @details カメラ座標に合わせて描画位置を計算し、赤い円を描画して敵の弾であることを視覚的に表現する
 */
void EnemyBullet::Draw()
{
    float screenX = Camera::WorldToScreenX(position.x);
    float screenY = Camera::WorldToScreenY(position.y);

    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
               static_cast<int>(radius), GetColor(255, 60, 60), TRUE);
    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
               static_cast<int>(radius + 2.0f), GetColor(255, 200, 200), FALSE);
}

/**
 * @brief プレイヤーへの着弾・ダメージ処理
 * @param player 着弾したプレイヤー
 * @param hitPos 着弾地点の座標
 */
void EnemyBullet::ApplyHit(Player* player, const Vector2& hitPos)
{
    if (!player) return;

    player->TakeDamage();

    float effectX = hitPos.x;
    float effectY = hitPos.y;
    Vector2 cPos = player->GetPosition();
    float cRadius = player->GetRadius();
    float dx = hitPos.x - cPos.x;
    float dy = hitPos.y - cPos.y;
    float dist = std::sqrt(dx * dx + dy * dy);
    if (cRadius > 0.0f && dist > 0.001f && dist < cRadius)
    {
        effectX = cPos.x + (dx / dist) * cRadius;
        effectY = cPos.y + (dy / dist) * cRadius;
    }

    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (scene && scene->GetEffectManager())
    {
        scene->GetEffectManager()->AddBloodEffect(effectX, effectY, 16);
    }

    SetActive(false);
}

/**
 * @brief 他のコライダーと衝突したときの処理
 * @param otherCollider 衝突相手のコライダー
 * @details 衝突相手がプレイヤー（タグが"Player"）であればプレイヤーにダメージを与え、弾自身は消滅する
 */
void EnemyBullet::OnCollisionEnter(Collider *otherCollider)
{
    if (!otherCollider || !otherCollider->GetOwner()) return;

    if (otherCollider->GetTag() == "Player")
    {
        Player *player = dynamic_cast<Player *>(otherCollider->GetOwner());
        if (player)
        {
            float hitX = position.x;
            float hitY = position.y;
            CircleCollider *circleCol = dynamic_cast<CircleCollider *>(otherCollider);
            if (circleCol)
            {
                float targetRadius = circleCol->GetRadius();
                float cx = circleCol->GetWorldX();
                float cy = circleCol->GetWorldY();
                float dx = position.x - cx;
                float dy = position.y - cy;
                float dist = std::sqrt(dx * dx + dy * dy);
                if (targetRadius > 0.0f && dist > 0.001f && dist < targetRadius)
                {
                    hitX = cx + (dx / dist) * targetRadius;
                    hitY = cy + (dy / dist) * targetRadius;
                }
            }

            ApplyHit(player, Vector2(hitX, hitY));
        }
    }
}

#include <algorithm>
#include <cmath>
#include "Camera.h"
#include "Bullet.h"
#include "Character.h"
#include "ColliderManager.h"
#include "DxLib.h"
#include "Enemy.h"
#include "Player.h"
#include "Scene.h"
#include "SceneManager.h"
#include "Stage.h"
#include "ObjectManager.h"

/**
 * @brief Bulletのコンストラクタ
 * @param startX 初期位置X
 * @param startY 初期位置Y
 * @param dir 進行方向
 * @param speed 弾の速度
 * @param range 最大射程距離
 * @param bulletRadius 当たり判定の半径
 * @param tId チームID
 * @details 初期位置、速度などの設定を行い、コライダーマネージャーに円形コライダーを登録する
 */
Bullet::Bullet(float startX, float startY, const Vector2 &dir, float speed, float range, float bulletRadius,
               int tId, int dmg, const std::string& sName, bool isPanic, const BulletVisualConfig& visual, int pierce)
    : Object2D(ObjectTag::PlayerWeapon), myColliderManager(nullptr), radius(bulletRadius), maxRange(range),
      startPos(startX, startY), teamId(tId), damage(dmg), shooterName(sName), isPanicBullet(isPanic), visualConfig(visual),
      pierceCount(pierce)
{
    position = Vector2(startX, startY);
    width = radius * 2.0f;
    height = radius * 2.0f;
    velocity = Vector2(dir.x * speed, dir.y * speed);

    collider = new CircleCollider(this, radius, "PlayerBullet");

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
 * @brief Bulletのデストラクタ
 * @details 登録したコライダーをコライダーマネージャーから削除し、メモリを解放する
 */
Bullet::~Bullet()
{
    if (myColliderManager)
    {
        myColliderManager->RemoveCollider(collider);
    }
    delete collider;
}

/**
 * @brief 毎フレームの弾の更新処理 (連続衝突判定 CCD & サブステップ分割によるすり抜け完全防止)
 * @details 高速弾の壁・キャラクター飛び越えを微小ステップと線分判定で防ぎ、確実な着弾・壁ヒットを実現
 */
void Bullet::Update()
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
    // サブステップ幅は弾の半径（3.5〜7.0px）と同等以下の安全なサイズ（4.0px）
    const float stepSize = 4.0f;
    int numSteps = static_cast<int>(std::ceil(totalMoveDist / stepSize));
    if (numSteps < 1) numSteps = 1;
    Vector2 stepVelocity = Vector2(velocity.x / static_cast<float>(numSteps), velocity.y / static_cast<float>(numSteps));

    Vector2 dir(velocity.x / totalMoveDist, velocity.y / totalMoveDist);

    for (int s = 0; s < numSteps; ++s)
    {
        Vector2 p0 = position;
        Vector2 p1 = Vector2(p0.x + stepVelocity.x, p0.y + stepVelocity.y);

        // ① 射程距離判定
        float dx = p1.x - startPos.x;
        float dy = p1.y - startPos.y;
        if (dx * dx + dy * dy > maxRange * maxRange)
        {
            position = p1;
            SetActive(false);
            return;
        }

        // ② 画面外判定
        if (p1.x < -200 || p1.x > 2100 || p1.y < -200 || p1.y > 1300)
        {
            position = p1;
            SetActive(false);
            return;
        }

        // ③ 壁・障害物判定 (微小ステップにより厚み40pxの壁を絶対に飛び越えない。水場は弾丸が通過可能)
        bool hitWall = false;
        if (stage)
        {
            // 中心点
            int gX = static_cast<int>(p1.x / cellSize);
            int gY = static_cast<int>(p1.y / cellSize);
            if (stage->IsBulletSolid(gX, gY))
            {
                hitWall = true;
            }
            else
            {
                // 弾の進行方向の先端（半径分先）も判定して角のすり抜けを防止
                int tipGX = static_cast<int>((p1.x + dir.x * radius) / cellSize);
                int tipGY = static_cast<int>((p1.y + dir.y * radius) / cellSize);
                if (stage->IsBulletSolid(tipGX, tipGY))
                {
                    hitWall = true;
                }
            }
        }

        // ④ キャラクターとの連続衝突判定（CCD: Continuous Collision Detection）
        // p0 から p1 への線分と、全アクティブキャラクターとの最近接距離判定
        Character* hitChar = nullptr;
        Vector2 bestHitPos = p1;
        float minT = 2.0f;

        if (scene && scene->GetObjectManager())
        {
            Vector2 seg = Vector2(p1.x - p0.x, p1.y - p0.y);
            float segLenSq = seg.x * seg.x + seg.y * seg.y;

            for (auto obj : scene->GetObjectManager()->GetObjects())
            {
                if (!obj || !obj->IsActive()) continue;
                Character* ch = dynamic_cast<Character*>(obj);
                if (!ch || ch->status.IsDead()) continue;

                // チーム判定 (別チーム、または錯乱・パニック時のみ被弾)
                bool canHit = false;
                if (ch->teamId != -1)
                {
                    if (ch->teamId != this->teamId)
                    {
                        canHit = true;
                    }
                    else if (ch->IsPanic() || this->isPanicBullet)
                    {
                        canHit = true;
                    }
                }
                if (!canHit) continue;

                // 既にヒット済みの貫通対象はスキップ
                if (std::find(hitTargets.begin(), hitTargets.end(), static_cast<void*>(ch)) != hitTargets.end())
                {
                    continue;
                }

                // キャラクターの判定位置と半径
                Vector2 cPos = ch->GetPosition();
                float cRadius = ch->GetRadius();
                float totalR = radius + cRadius;

                // 線分 p0 -> p1 と 点 cPos の最近接点パラメータ t
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
                    // 壁の向こう側にいるキャラクターには当たらないよう判定 (水場を挟んだ射撃はヒット可能)
                    if (stage && stage->IsBulletSolid(static_cast<int>(closest.x / cellSize), static_cast<int>(closest.y / cellSize)))
                    {
                        continue;
                    }

                    if (t < minT)
                    {
                        minT = t;
                        hitChar = ch;
                        bestHitPos = closest;
                    }
                }
            }
        }

        // キャラクターに衝突した場合 (壁よりも手前にいる敵に確実に当たる)
        if (hitChar)
        {
            position = bestHitPos;
            ApplyHit(hitChar, bestHitPos);
            if (!IsActive())
            {
                return; // 弾が消滅した場合はここで終了
            }
        }
        else if (hitWall)
        {
            // 壁に衝突した場合
            position = p1;
            SetActive(false);
            return;
        }

        // サブステップ位置を更新
        position = p1;
    }
}

/**
 * @brief 弾の描画処理
 * @details カメラ座標に合わせて描画位置を計算し、チームIDに応じた色で弾のコアとグロー（半透明）を描画する
 */
void Bullet::Draw()
{
    float screenX = Camera::WorldToScreenX(position.x);
    float screenY = Camera::WorldToScreenY(position.y);

    unsigned int colorCore = visualConfig.customColors ? visualConfig.coreColor : GetColor(255, 255, 255);
    unsigned int colorGlow = visualConfig.customColors ? visualConfig.glowColor : ((teamId == 0) ? GetColor(0, 150, 255) : GetColor(255, 50, 50));
    int glowAlpha = visualConfig.customColors ? visualConfig.glowAlpha : 160;
    float glowOffset = visualConfig.customColors ? visualConfig.glowRadiusOffset : 4.0f;

    // トレール（弾道軌跡・光の尾）の描画
    if (visualConfig.hasTrail)
    {
        float vLen = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
        if (vLen > 0.001f)
        {
            float tLen = visualConfig.trailLength;
            float backX = screenX - (velocity.x / vLen) * tLen;
            float backY = screenY - (velocity.y / vLen) * tLen;
            unsigned int tColor = visualConfig.customColors ? visualConfig.trailColor : colorGlow;
            int tAlpha = visualConfig.customColors ? visualConfig.trailAlpha : 100;
            int tWidth = visualConfig.trailWidth;

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, tAlpha);
            DrawLine(static_cast<int>(backX), static_cast<int>(backY),
                     static_cast<int>(screenX), static_cast<int>(screenY), tColor, tWidth);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
    }

    // グロー（発光外周）の描画
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, glowAlpha);
    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
               static_cast<int>(radius + glowOffset), colorGlow, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // コア（中心）の描画
    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
               static_cast<int>(radius), colorCore, TRUE);
}

/**
 * @brief 他のコライダーと衝突したときの処理
 * @param otherCollider 衝突相手のコライダー
 * @details 衝突相手が敵またはプレイヤーで、かつ別チームであればダメージを与え、血しぶきエフェクトを発生させて自身は消滅する
 */
/**
 * @brief キャラクターへの着弾・ダメージ処理
 * @param target 着弾したキャラクター
 * @param hitPos 着弾地点の座標
 */
void Bullet::ApplyHit(Character* target, const Vector2& hitPos)
{
    if (!target) return;

    // 多重ヒット防止: 同一弾丸による同一対象への重複ダメージを遮断
    if (std::find(hitTargets.begin(), hitTargets.end(), static_cast<void*>(target)) != hitTargets.end())
    {
        return;
    }
    hitTargets.push_back(static_cast<void*>(target));

    if (target->GetObjectTag() == ObjectTag::Enemy) {
        Enemy *enemy = dynamic_cast<Enemy *>(target);
        if (enemy) enemy->Damage(this->damage, this->shooterName, this->teamId);
    } else if (target->GetObjectTag() == ObjectTag::Player) {
        Player *player = dynamic_cast<Player *>(target);
        if (player) player->TakeDamage(this->damage, this->shooterName, this->teamId);
    }

    // 敵またはプレイヤーに弾が当たった瞬間に、当たった場所（被弾境界位置）でエフェクトを呼び出す
    float effectX = hitPos.x;
    float effectY = hitPos.y;
    Vector2 cPos = target->GetPosition();
    float cRadius = target->GetRadius();
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

    // 貫通弾の処理: pierceCount が残っている場合は弾を消さずに続行
    if (pierceCount > 0)
    {
        pierceCount--;
    }
    else
    {
        SetActive(false);
    }
}

/**
 * @brief 他のコライダーと衝突したときの処理
 * @param otherCollider 衝突相手のコライダー
 * @details 衝突相手が敵またはプレイヤーで、かつ別チームであればダメージを与え、血しぶきエフェクトを発生させて自身は消滅する
 */
void Bullet::OnCollisionEnter(Collider *otherCollider)
{
    if (otherCollider && otherCollider->GetOwner())
    {
        Character *target = dynamic_cast<Character *>(otherCollider->GetOwner());
        bool canHit = false;
        if (target && target->teamId != -1)
        {
            if (target->teamId != this->teamId)
            {
                canHit = true;
            }
            else if (target->IsPanic() || this->isPanicBullet)
            {
                // パニック（錯乱）状態のキャラクターは味方の弾にも被弾し、パニック中の射撃者の弾は味方にも当たる
                canHit = true;
            }
        }

        if (canHit)
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

            ApplyHit(target, Vector2(hitX, hitY));
        }
    }
}

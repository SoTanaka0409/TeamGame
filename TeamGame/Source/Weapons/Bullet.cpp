#include "Camera.h"
#include "Bullet.h"
#include "Character.h"
#include "ColliderManager.h"
#include "DxLib.h"
#include "Enemy.h"
#include "Scene.h"
#include "SceneManager.h"

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
               int tId, int dmg, const std::string& sName, bool isPanic, const BulletVisualConfig& visual)
    : Object2D(ObjectTag::PlayerWeapon), myColliderManager(nullptr), radius(bulletRadius), maxRange(range),
      startPos(startX, startY), teamId(tId), damage(dmg), shooterName(sName), isPanicBullet(isPanic), visualConfig(visual)
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

#include "Stage.h"

/**
 * @brief 毎フレームの弾の更新処理
 * @details 弾を移動させ、最大射程距離の超過、画面外への退出、およびステージ上の障害物との衝突判定を行い、該当すれば自身を非アクティブにする
 */
void Bullet::Update()
{
    position.x += velocity.x;
    position.y += velocity.y;

    float dx = position.x - startPos.x;
    float dy = position.y - startPos.y;
    if (dx * dx + dy * dy > maxRange * maxRange)
    {
        SetActive(false);
        return;
    }

    // 画面外に出たら消去
    if (position.x < -200 || position.x > 2100 || position.y < -200 ||
        position.y > 1300)
    {
        SetActive(false);
        return;
    }

    // ステージ壁・障害物との衝突判定 (障害物を貫通しない)
    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (scene && scene->GetStage())
    {
        const Stage *stage = scene->GetStage();
        float cellSize = 40.0f;
        int gX = static_cast<int>(position.x / cellSize);
        int gY = static_cast<int>(position.y / cellSize);
        if (stage->IsSolidWall(gX, gY))
        {
            SetActive(false); // 壁・障害物にヒットして消滅
            return;
        }
    }
}

#include "ObjectManager.h"
#include "Player.h"

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
void Bullet::OnCollisionEnter(Collider *otherCollider)
{
    if (otherCollider->GetOwner())
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
            if (target->GetObjectTag() == ObjectTag::Enemy) {
                Enemy *enemy = dynamic_cast<Enemy *>(target);
                if (enemy) enemy->Damage(this->damage, this->shooterName, this->teamId);
            } else if (target->GetObjectTag() == ObjectTag::Player) {
                Player *player = dynamic_cast<Player *>(target);
                if (player) player->TakeDamage(this->damage, this->shooterName, this->teamId);
            }
            
            // 敵またはプレイヤーに弾が当たった瞬間に、当たった場所（被弾境界位置）でエフェクトを呼び出す
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
                    // 弾がフレーム間の移動で敵内部に入り込んだ場合、敵表面（被弾地点）に補正
                    hitX = cx + (dx / dist) * targetRadius;
                    hitY = cy + (dy / dist) * targetRadius;
                }
            }

            auto scene = SceneManager::GetInstance().GetCurrentScene();
            if (scene && scene->GetEffectManager())
            {
                scene->GetEffectManager()->AddBloodEffect(hitX, hitY, 16);
            }
            SetActive(false);
        }
    }
}

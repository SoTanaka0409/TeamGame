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
Bullet::Bullet(float startX, float startY, const Vector2 &dir, float speed, float range, float bulletRadius, int tId, int dmg)
    : Object2D(ObjectTag::PlayerWeapon), myColliderManager(nullptr), radius(bulletRadius), maxRange(range), startPos(startX, startY), teamId(tId), damage(dmg)
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
        myColliderManager->AddCollider(collider);
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

    unsigned int colorCore = GetColor(255, 255, 255);
    unsigned int colorGlow = (teamId == 0) ? GetColor(0, 150, 255) : GetColor(255, 50, 50);

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 160);
    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
               static_cast<int>(radius + 4.0f), colorGlow, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

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
        if (target && target->teamId != this->teamId && target->teamId != -1)
        {
            if (target->GetObjectTag() == ObjectTag::Enemy) {
                Enemy *enemy = dynamic_cast<Enemy *>(target);
                if (enemy) enemy->Damage(this->damage);
            } else if (target->GetObjectTag() == ObjectTag::Player) {
                Player *player = dynamic_cast<Player *>(target);
                if (player) player->TakeDamage(this->damage);
            }
            
            auto scene = SceneManager::GetInstance().GetCurrentScene();
            if (scene && scene->GetEffectManager())
            {
                scene->GetEffectManager()->AddBloodEffect(position.x, position.y, 10);
            }
            SetActive(false);
        }
    }
}

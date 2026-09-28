#include "Camera.h"
#include "EnemyBullet.h"
#include "ColliderManager.h"
#include "DxLib.h"
#include "Player.h"
#include "Scene.h"
#include "SceneManager.h"

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
        myColliderManager->AddCollider(collider);
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

#include "Stage.h"

/**
 * @brief 毎フレームの敵の弾の更新処理
 * @details 弾を移動させ、最大射程の超過、画面外への退出、およびステージの壁・障害物との衝突をチェックし、条件を満たせば消滅させる
 */
void EnemyBullet::Update()
{
    position.x += velocity.x;
    position.y += velocity.y;

    // 射程距離制限チェック (最大射程を超えたら消滅)
    float dx = position.x - startPos.x;
    float dy = position.y - startPos.y;
    if (dx * dx + dy * dy > maxRange * maxRange)
    {
        SetActive(false);
        return;
    }

    // 画面外に出たら消滅
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
 * @brief 他のコライダーと衝突したときの処理
 * @param otherCollider 衝突相手のコライダー
 * @details 衝突相手がプレイヤー（タグが"Player"）であればプレイヤーにダメージを与え、弾自身は消滅する
 */
void EnemyBullet::OnCollisionEnter(Collider *otherCollider)
{
    if (otherCollider->GetTag() == "Player")
    {
        Player *player = dynamic_cast<Player *>(otherCollider->GetOwner());
        if (player)
        {
            player->TakeDamage();
        }
        SetActive(false);
    }
}


#include "Camera.h"
#include "EnemyBullet.h"
#include "ColliderManager.h"
#include "DxLib.h"
#include "Player.h"
#include "Scene.h"
#include "SceneManager.h"

EnemyBullet::EnemyBullet(float startX, float startY, const Vector2 &dir, float speed)
    : myColliderManager(nullptr), radius(6.0f)
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

EnemyBullet::~EnemyBullet()
{
    if (myColliderManager)
    {
        myColliderManager->RemoveCollider(collider);
    }
    delete collider;
}

#include "Stage.h"

void EnemyBullet::Update()
{
    position.x += velocity.x;
    position.y += velocity.y;

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

void EnemyBullet::Draw()
{
    float screenX = Camera::WorldToScreenX(position.x);
    float screenY = Camera::WorldToScreenY(position.y);

    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
               static_cast<int>(radius), GetColor(255, 60, 60), TRUE);
    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
               static_cast<int>(radius + 2.0f), GetColor(255, 200, 200), FALSE);
}

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


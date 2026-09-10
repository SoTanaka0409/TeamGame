#include "Bullet.h"
#include "ColliderManager.h"
#include "DxLib.h"
#include "Enemy.h"
#include "Scene.h"
#include "SceneManager.h"

Bullet::Bullet(float startX, float startY, const Vector2 &dir, float speed)
    : Object2D(ObjectTag::PlayerWeapon), myColliderManager(nullptr), radius(5.0f)
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

Bullet::~Bullet()
{
    if (myColliderManager)
    {
        myColliderManager->RemoveCollider(collider);
    }
    delete collider;
}

#include "Stage.h"

void Bullet::Update()
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
#include "Player.h"

void Bullet::Draw()
{
    float screenX = position.x;
    float screenY = position.y;

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
                screenX = 960.0f + (position.x - pPos.x) * zoomScale;
                screenY = 540.0f + (position.y - pPos.y) * zoomScale;
                break;
            }
        }
    }

    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
               static_cast<int>(radius), GetColor(0, 255, 255), TRUE);
}

void Bullet::OnCollisionEnter(Collider *otherCollider)
{
    // 敵に当たったらダメージを与えて自身も消滅
    if (otherCollider->GetOwner() && otherCollider->GetOwner()->GetObjectTag() == ObjectTag::Enemy)
    {
        Enemy *enemy = dynamic_cast<Enemy *>(otherCollider->GetOwner());
        if (enemy)
        {
            enemy->Damage();
            auto scene = SceneManager::GetInstance().GetCurrentScene();
            if (scene && scene->GetEffectManager())
            {
                scene->GetEffectManager()->AddBloodEffect(position.x, position.y, 10);
            }
        }
        SetActive(false);
    }
}

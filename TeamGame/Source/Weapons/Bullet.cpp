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

void Bullet::Update()
{
    position.x += velocity.x;
    position.y += velocity.y;

    // 画面外に出たら消滅
    if (position.x < -100 || position.x > 2000 || position.y < -100 ||
        position.y > 1200)
    {
        SetActive(false);
    }
}

void Bullet::Draw()
{
    DrawCircle(static_cast<int>(position.x), static_cast<int>(position.y),
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
        }
        SetActive(false);
    }
}

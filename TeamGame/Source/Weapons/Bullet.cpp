#include "Camera.h"
#include "Bullet.h"
#include "Character.h"
#include "ColliderManager.h"
#include "DxLib.h"
#include "Enemy.h"
#include "Scene.h"
#include "SceneManager.h"

Bullet::Bullet(float startX, float startY, const Vector2 &dir, float speed, float range, float bulletRadius, int tId)
    : Object2D(ObjectTag::PlayerWeapon), myColliderManager(nullptr), radius(bulletRadius), maxRange(range), startPos(startX, startY), teamId(tId)
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

void Bullet::Draw()
{
    float screenX = Camera::WorldToScreenX(position.x);
    float screenY = Camera::WorldToScreenY(position.y);

    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
               static_cast<int>(radius), GetColor(0, 255, 255), TRUE);
}

void Bullet::OnCollisionEnter(Collider *otherCollider)
{
    if (otherCollider->GetOwner())
    {
        Character *target = dynamic_cast<Character *>(otherCollider->GetOwner());
        if (target && target->teamId != this->teamId && target->teamId != -1)
        {
            if (target->GetObjectTag() == ObjectTag::Enemy) {
                Enemy *enemy = dynamic_cast<Enemy *>(target);
                if (enemy) enemy->Damage();
            } else if (target->GetObjectTag() == ObjectTag::Player) {
                Player *player = dynamic_cast<Player *>(target);
                if (player) player->TakeDamage();
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

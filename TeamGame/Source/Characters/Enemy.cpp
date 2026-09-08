#include "Enemy.h"
#include "DxLib.h"
#include "Player.h"

Enemy::Enemy(float startX, float startY)
    : Character(ObjectTag::Enemy, startX, startY, 25.0f), hp(3)
{
    speed = 2.0f;
    collider->SetTag("Enemy");
}

Enemy::~Enemy()
{
}

void Enemy::Update()
{
    position.y += speed;

    if (position.y > 1080 + radius)
    {
        SetActive(false);
    }
}

void Enemy::Draw()
{
    DrawCircle(static_cast<int>(position.x), static_cast<int>(position.y),
               static_cast<int>(radius), GetColor(255, 0, 0), TRUE);
}

void Enemy::Damage()
{
    hp--;
    if (hp <= 0)
    {
        SetActive(false);
    }
}

void Enemy::OnCollisionEnter(Collider *otherCollider)
{
    // 例: もし相手のコライダーのタグが "PlayerWeapon"
    // ならダメージを受ける、など
}

void Enemy::OnCollisionStay(Collider *otherCollider)
{
}

void Enemy::OnCollisionExit(Collider *otherCollider)
{
}

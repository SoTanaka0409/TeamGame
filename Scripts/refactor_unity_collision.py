import os
import xml.etree.ElementTree as ET

DIR = r'C:\Users\student\Desktop\TeamGame\TeamGame\TeamGame'

files = {
    'Object2D.h': '''#pragma once
#include "Vector2.h"

class Object2D
{
protected:
    Vector2 position;
    float width, height;
    bool isActive;

public:
    Object2D();
    virtual ~Object2D();

    virtual void Update() = 0;
    virtual void Draw() = 0;

    virtual void OnCollisionEnter(Object2D* other) {}
    virtual void OnCollisionStay(Object2D* other) {}
    virtual void OnCollisionExit(Object2D* other) {}

    Vector2 GetPosition() const { return position; }
    void SetPosition(const Vector2& pos) { position = pos; }

    float GetWidth() const { return width; }
    float GetHeight() const { return height; }

    bool IsActive() const { return isActive; }
    void SetActive(bool active) { isActive = active; }
};
''',

    'ColliderManager.h': '''#pragma once
#include "Collider.h"
#include <vector>
#include <set>
#include <utility>

class ColliderManager
{
private:
    std::vector<Collider*> colliders;
    std::set<std::pair<Collider*, Collider*>> previousCollisions;

public:
    ColliderManager();
    ~ColliderManager();

    void AddCollider(Collider* collider);
    void RemoveCollider(Collider* collider);
    void CheckAllCollisions();
    void Clear();
};
''',

    'ColliderManager.cpp': '''#include "ColliderManager.h"
#include "Object2D.h"

ColliderManager::ColliderManager()
{
}

ColliderManager::~ColliderManager()
{
    Clear();
}

void ColliderManager::AddCollider(Collider* collider)
{
    colliders.push_back(collider);
}

void ColliderManager::RemoveCollider(Collider* collider)
{
    // コンテナから削除
    auto it = colliders.begin();
    while (it != colliders.end())
    {
        if (*it == collider)
        {
            it = colliders.erase(it);
            break; // 重複はない想定
        }
        else
        {
            ++it;
        }
    }

    // previousCollisions からも削除し、残る側には OnCollisionExit を通知
    auto itPrev = previousCollisions.begin();
    while (itPrev != previousCollisions.end())
    {
        if (itPrev->first == collider || itPrev->second == collider)
        {
            Object2D* objA = itPrev->first->GetOwner();
            Object2D* objB = itPrev->second->GetOwner();

            if (itPrev->first == collider)
            {
                if (objB && objB->IsActive())
                    objB->OnCollisionExit(objA);
            }
            else
            {
                if (objA && objA->IsActive())
                    objA->OnCollisionExit(objB);
            }

            itPrev = previousCollisions.erase(itPrev);
        }
        else
        {
            ++itPrev;
        }
    }
}

void ColliderManager::CheckAllCollisions()
{
    std::set<std::pair<Collider*, Collider*>> currentCollisions;

    // 当たり判定の総当たりチェック
    for (size_t i = 0; i < colliders.size(); ++i)
    {
        for (size_t j = i + 1; j < colliders.size(); ++j)
        {
            Object2D* ownerA = colliders[i]->GetOwner();
            Object2D* ownerB = colliders[j]->GetOwner();

            if (!ownerA || !ownerB || !ownerA->IsActive() || !ownerB->IsActive())
                continue;

            if (colliders[i]->IsCollision(colliders[j]))
            {
                auto pair = (colliders[i] < colliders[j])
                                ? std::make_pair(colliders[i], colliders[j])
                                : std::make_pair(colliders[j], colliders[i]);
                currentCollisions.insert(pair);
            }
        }
    }

    // Enter と Stay の判定
    for (auto& pair : currentCollisions)
    {
        bool isEnter = (previousCollisions.find(pair) == previousCollisions.end());

        Object2D* objA = pair.first->GetOwner();
        Object2D* objB = pair.second->GetOwner();

        if (!objA || !objB) continue;

        if (isEnter)
        {
            if (objA->IsActive()) objA->OnCollisionEnter(objB);
            if (objB->IsActive()) objB->OnCollisionEnter(objA);
        }
        else
        {
            if (objA->IsActive()) objA->OnCollisionStay(objB);
            if (objB->IsActive()) objB->OnCollisionStay(objA);
        }
    }

    // Exit の判定
    for (auto& pair : previousCollisions)
    {
        if (currentCollisions.find(pair) == currentCollisions.end())
        {
            Object2D* objA = pair.first->GetOwner();
            Object2D* objB = pair.second->GetOwner();

            if (objA && objA->IsActive()) objA->OnCollisionExit(objB);
            if (objB && objB->IsActive()) objB->OnCollisionExit(objA);
        }
    }

    previousCollisions = currentCollisions;
}

void ColliderManager::Clear()
{
    colliders.clear();
    previousCollisions.clear();
}
''',

    'Player.h': '''#pragma once
#include "Character.h"

class Player : public Character
{
private:
    int damageColorTimer; // ダメージを受けた時に色を変えるタイマー

public:
    Player(float startX, float startY);
    virtual ~Player();

    void Update() override;
    void Draw() override;

    void OnCollisionEnter(Object2D* other) override;
    void OnCollisionStay(Object2D* other) override;
    void OnCollisionExit(Object2D* other) override;
};
''',

    'Player.cpp': '''#include "Player.h"
#include "Enemy.h"
#include "InputManager.h"
#include "DxLib.h"

Player::Player(float startX, float startY) : Character(startX, startY, 20.0f), damageColorTimer(0)
{
    speed = 5.0f;
}

Player::~Player()
{
}

void Player::Update()
{
    if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_LEFT))
        position.x -= speed;
    if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_RIGHT))
        position.x += speed;
    if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_UP))
        position.y -= speed;
    if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_DOWN))
        position.y += speed;

    if (damageColorTimer > 0)
    {
        damageColorTimer--;
    }
}

void Player::Draw()
{
    // 通常時は緑、ダメージ中は黄色
    unsigned int color = (damageColorTimer > 0) ? GetColor(255, 255, 0) : GetColor(0, 255, 0);
    DrawCircle(static_cast<int>(position.x), static_cast<int>(position.y), static_cast<int>(radius), color, TRUE);
}

void Player::OnCollisionEnter(Object2D* other)
{
    Enemy* enemy = dynamic_cast<Enemy*>(other);
    if (enemy)
    {
        // 敵と当たった瞬間
        enemy->Damage(); // 敵にダメージを与える
        damageColorTimer = 30; // プレイヤー自身は黄色く光る
    }
}

void Player::OnCollisionStay(Object2D* other)
{
    Enemy* enemy = dynamic_cast<Enemy*>(other);
    if (enemy)
    {
        // 敵と当たり続けている時の処理（例：少しずつダメージを受けるなど）
    }
}

void Player::OnCollisionExit(Object2D* other)
{
    Enemy* enemy = dynamic_cast<Enemy*>(other);
    if (enemy)
    {
        // 敵から離れた時の処理
    }
}
''',

    'Enemy.h': '''#pragma once
#include "Character.h"

class Enemy : public Character
{
private:
    int hp;

public:
    Enemy(float startX, float startY);
    virtual ~Enemy();

    void Update() override;
    void Draw() override;
    
    void Damage();

    void OnCollisionEnter(Object2D* other) override;
    void OnCollisionStay(Object2D* other) override;
    void OnCollisionExit(Object2D* other) override;
};
''',

    'Enemy.cpp': '''#include "Enemy.h"
#include "Player.h"
#include "DxLib.h"

Enemy::Enemy(float startX, float startY) : Character(startX, startY, 25.0f), hp(3)
{
    speed = 2.0f;
}

Enemy::~Enemy()
{
}

void Enemy::Update()
{
    position.y += speed;

    if (position.y > 1080 + radius)
    {
        SetActive(false); // 画面外に出たら消す
    }
}

void Enemy::Draw()
{
    DrawCircle(static_cast<int>(position.x), static_cast<int>(position.y), static_cast<int>(radius), GetColor(255, 0, 0), TRUE);
}

void Enemy::Damage()
{
    hp--;
    if (hp <= 0)
    {
        SetActive(false); // HPが0になったら消滅
    }
}

void Enemy::OnCollisionEnter(Object2D* other)
{
    Player* player = dynamic_cast<Player*>(other);
    if (player)
    {
        // プレイヤーと当たった瞬間
    }
}

void Enemy::OnCollisionStay(Object2D* other)
{
}

void Enemy::OnCollisionExit(Object2D* other)
{
}
'''
}

for name, content in files.items():
    with open(os.path.join(DIR, name), 'w', encoding='utf-8') as f:
        f.write(content)


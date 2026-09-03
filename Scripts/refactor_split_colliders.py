import os
import xml.etree.ElementTree as ET

DIR = r'C:\Users\student\Desktop\TeamGame\TeamGame\TeamGame'

files = {
    'Collider.h': '''#pragma once
#include <string>

class Object2D;
class CircleCollider;
class RectCollider;

class Collider
{
protected:
    Object2D* owner;
    float offsetX, offsetY;
    std::string tag; // コライダーを識別するタグ（"Body", "WeakPoint", "Weapon"など）

public:
    Collider(Object2D* owner, const std::string& tag = "");
    virtual ~Collider();

    Object2D* GetOwner() const { return owner; }
    std::string GetTag() const { return tag; }
    void SetTag(const std::string& t) { tag = t; }

    float GetWorldX() const;
    float GetWorldY() const;

    void SetOffset(float x, float y) { offsetX = x; offsetY = y; }

    // ダブルディスパッチ用関数群
    virtual bool IsCollision(Collider* other) const = 0;
    virtual bool IsCollisionWithCircle(const CircleCollider* circle) const = 0;
    virtual bool IsCollisionWithRect(const RectCollider* rect) const = 0;
};
''',

    'Collider.cpp': '''#include "Collider.h"
#include "Object2D.h"

Collider::Collider(Object2D* owner, const std::string& tag) : owner(owner), offsetX(0.0f), offsetY(0.0f), tag(tag)
{
}

Collider::~Collider()
{
}

float Collider::GetWorldX() const
{
    return owner ? owner->GetPosition().x + offsetX : offsetX;
}

float Collider::GetWorldY() const
{
    return owner ? owner->GetPosition().y + offsetY : offsetY;
}
''',

    'CircleCollider.h': '''#pragma once
#include "Collider.h"

class CircleCollider : public Collider
{
private:
    float radius;

public:
    CircleCollider(Object2D* owner, float radius, const std::string& tag = "");
    ~CircleCollider() override;

    float GetRadius() const { return radius; }
    void SetRadius(float r) { radius = r; }

    bool IsCollision(Collider* other) const override;
    bool IsCollisionWithCircle(const CircleCollider* circle) const override;
    bool IsCollisionWithRect(const RectCollider* rect) const override;
};
''',

    'CircleCollider.cpp': '''#include "CircleCollider.h"
#include "RectCollider.h"

CircleCollider::CircleCollider(Object2D* owner, float radius, const std::string& tag)
    : Collider(owner, tag), radius(radius)
{
}

CircleCollider::~CircleCollider()
{
}

bool CircleCollider::IsCollision(Collider* other) const
{
    // 相手が何型か分からないので、相手に「自分（Circle）と当たっているか？」と聞き返す（ダブルディスパッチ）
    return other->IsCollisionWithCircle(this);
}

bool CircleCollider::IsCollisionWithCircle(const CircleCollider* circle) const
{
    float dx = GetWorldX() - circle->GetWorldX();
    float dy = GetWorldY() - circle->GetWorldY();
    float distSq = dx * dx + dy * dy;
    float rSum = radius + circle->GetRadius();
    return distSq <= rSum * rSum;
}

bool CircleCollider::IsCollisionWithRect(const RectCollider* rect) const
{
    // 今回は簡易的に実装（本来は円と矩形の正確な判定が必要）
    float thisX = GetWorldX();
    float thisY = GetWorldY();
    float rectL = rect->GetWorldX() - rect->GetWidth() / 2;
    float rectR = rect->GetWorldX() + rect->GetWidth() / 2;
    float rectT = rect->GetWorldY() - rect->GetHeight() / 2;
    float rectB = rect->GetWorldY() + rect->GetHeight() / 2;

    float testX = thisX;
    float testY = thisY;

    if (thisX < rectL) testX = rectL;
    else if (thisX > rectR) testX = rectR;
    
    if (thisY < rectT) testY = rectT;
    else if (thisY > rectB) testY = rectB;

    float distX = thisX - testX;
    float distY = thisY - testY;
    float distance = (distX*distX) + (distY*distY);

    return distance <= (radius * radius);
}
''',

    'RectCollider.h': '''#pragma once
#include "Collider.h"

class RectCollider : public Collider
{
private:
    float width, height;

public:
    RectCollider(Object2D* owner, float width, float height, const std::string& tag = "");
    ~RectCollider() override;

    float GetWidth() const { return width; }
    float GetHeight() const { return height; }

    bool IsCollision(Collider* other) const override;
    bool IsCollisionWithCircle(const CircleCollider* circle) const override;
    bool IsCollisionWithRect(const RectCollider* rect) const override;
};
''',

    'RectCollider.cpp': '''#include "RectCollider.h"
#include "CircleCollider.h"

RectCollider::RectCollider(Object2D* owner, float width, float height, const std::string& tag)
    : Collider(owner, tag), width(width), height(height)
{
}

RectCollider::~RectCollider()
{
}

bool RectCollider::IsCollision(Collider* other) const
{
    return other->IsCollisionWithRect(this);
}

bool RectCollider::IsCollisionWithCircle(const CircleCollider* circle) const
{
    // Circle側の判定に任せる
    return circle->IsCollisionWithRect(this);
}

bool RectCollider::IsCollisionWithRect(const RectCollider* rect) const
{
    float l1 = GetWorldX() - width / 2;
    float r1 = GetWorldX() + width / 2;
    float t1 = GetWorldY() - height / 2;
    float b1 = GetWorldY() + height / 2;

    float l2 = rect->GetWorldX() - rect->GetWidth() / 2;
    float r2 = rect->GetWorldX() + rect->GetWidth() / 2;
    float t2 = rect->GetWorldY() - rect->GetHeight() / 2;
    float b2 = rect->GetWorldY() + rect->GetHeight() / 2;

    return l1 < r2 && r1 > l2 && t1 < b2 && b1 > t2;
}
''',

    'Object2D.h': '''#pragma once
#include "Vector2.h"

class Collider;

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

    // 引数を Object2D* から Collider* に変更
    virtual void OnCollisionEnter(Collider* otherCollider) {}
    virtual void OnCollisionStay(Collider* otherCollider) {}
    virtual void OnCollisionExit(Collider* otherCollider) {}

    Vector2 GetPosition() const { return position; }
    void SetPosition(const Vector2& pos) { position = pos; }

    float GetWidth() const { return width; }
    float GetHeight() const { return height; }

    bool IsActive() const { return isActive; }
    void SetActive(bool active) { isActive = active; }
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
    auto it = colliders.begin();
    while (it != colliders.end())
    {
        if (*it == collider)
        {
            it = colliders.erase(it);
            break;
        }
        else
        {
            ++it;
        }
    }

    auto itPrev = previousCollisions.begin();
    while (itPrev != previousCollisions.end())
    {
        if (itPrev->first == collider || itPrev->second == collider)
        {
            Collider* otherCollider = (itPrev->first == collider) ? itPrev->second : itPrev->first;
            Object2D* otherOwner = otherCollider->GetOwner();
            Object2D* thisOwner = collider->GetOwner();

            if (otherOwner && otherOwner->IsActive())
                otherOwner->OnCollisionExit(collider);
                
            if (thisOwner && thisOwner->IsActive())
                thisOwner->OnCollisionExit(otherCollider);

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

    for (auto& pair : currentCollisions)
    {
        bool isEnter = (previousCollisions.find(pair) == previousCollisions.end());

        Collider* colA = pair.first;
        Collider* colB = pair.second;
        Object2D* objA = colA->GetOwner();
        Object2D* objB = colB->GetOwner();

        if (!objA || !objB) continue;

        if (isEnter)
        {
            if (objA->IsActive()) objA->OnCollisionEnter(colB);
            if (objB->IsActive()) objB->OnCollisionEnter(colA);
        }
        else
        {
            if (objA->IsActive()) objA->OnCollisionStay(colB);
            if (objB->IsActive()) objB->OnCollisionStay(colA);
        }
    }

    for (auto& pair : previousCollisions)
    {
        if (currentCollisions.find(pair) == currentCollisions.end())
        {
            Collider* colA = pair.first;
            Collider* colB = pair.second;
            Object2D* objA = colA->GetOwner();
            Object2D* objB = colB->GetOwner();

            if (objA && objA->IsActive()) objA->OnCollisionExit(colB);
            if (objB && objB->IsActive()) objB->OnCollisionExit(colA);
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

    'Character.h': '''#pragma once
#include "Object2D.h"
#include "CircleCollider.h"

class Character : public Object2D
{
protected:
    CircleCollider* collider; // 今回は標準でCircleを持つように
    float speed;
    float radius;

public:
    Character(float startX, float startY, float radius);
    virtual ~Character();

    CircleCollider* GetCollider() const { return collider; }
};
''',

    'Character.cpp': '''#include "Character.h"
#include "SceneManager.h"
#include "Scene.h"
#include "ColliderManager.h"

Character::Character(float startX, float startY, float rad) : speed(0.0f), radius(rad)
{
    position = Vector2(startX, startY);
    width = rad * 2.0f;
    height = rad * 2.0f;

    // Characterクラス内でCircleColliderをnewする
    collider = new CircleCollider(this, rad, "CharacterBody");
    
    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (scene)
    {
        scene->GetColliderManager()->AddCollider(collider);
    }
}

Character::~Character()
{
    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (scene)
    {
        scene->GetColliderManager()->RemoveCollider(collider);
    }
    delete collider;
}
''',

    'Player.h': '''#pragma once
#include "Character.h"

class Player : public Character
{
private:
    int damageColorTimer;

public:
    Player(float startX, float startY);
    virtual ~Player();

    void Update() override;
    void Draw() override;

    // 引数が Collider* に変更
    void OnCollisionEnter(Collider* otherCollider) override;
    void OnCollisionStay(Collider* otherCollider) override;
    void OnCollisionExit(Collider* otherCollider) override;
};
''',

    'Player.cpp': '''#include "Player.h"
#include "Enemy.h"
#include "InputManager.h"
#include "DxLib.h"

Player::Player(float startX, float startY) : Character(startX, startY, 20.0f), damageColorTimer(0)
{
    speed = 5.0f;
    // プレイヤーのコライダーにタグを付ける
    collider->SetTag("Player");
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
    unsigned int color = (damageColorTimer > 0) ? GetColor(255, 255, 0) : GetColor(0, 255, 0);
    DrawCircle(static_cast<int>(position.x), static_cast<int>(position.y), static_cast<int>(radius), color, TRUE);
}

void Player::OnCollisionEnter(Collider* otherCollider)
{
    // タグで判定するか、Ownerのクラスで判定するか選べる！
    Enemy* enemy = dynamic_cast<Enemy*>(otherCollider->GetOwner());
    if (enemy)
    {
        enemy->Damage();
        damageColorTimer = 30;
    }
}

void Player::OnCollisionStay(Collider* otherCollider)
{
}

void Player::OnCollisionExit(Collider* otherCollider)
{
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

    void OnCollisionEnter(Collider* otherCollider) override;
    void OnCollisionStay(Collider* otherCollider) override;
    void OnCollisionExit(Collider* otherCollider) override;
};
''',

    'Enemy.cpp': '''#include "Enemy.h"
#include "Player.h"
#include "DxLib.h"

Enemy::Enemy(float startX, float startY) : Character(startX, startY, 25.0f), hp(3)
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
    DrawCircle(static_cast<int>(position.x), static_cast<int>(position.y), static_cast<int>(radius), GetColor(255, 0, 0), TRUE);
}

void Enemy::Damage()
{
    hp--;
    if (hp <= 0)
    {
        SetActive(false);
    }
}

void Enemy::OnCollisionEnter(Collider* otherCollider)
{
    // 例: もし相手のコライダーのタグが "PlayerWeapon" ならダメージを受ける、など
}

void Enemy::OnCollisionStay(Collider* otherCollider)
{
}

void Enemy::OnCollisionExit(Collider* otherCollider)
{
}
'''
}

for name, content in files.items():
    with open(os.path.join(DIR, name), 'w', encoding='utf-8') as f:
        f.write(content)

# Update vcxproj
proj = os.path.join(DIR, 'TeamGame.vcxproj')
filters = os.path.join(DIR, 'TeamGame.vcxproj.filters')

ET.register_namespace('', 'http://schemas.microsoft.com/developer/msbuild/2003')
ns = {'ms': 'http://schemas.microsoft.com/developer/msbuild/2003'}

tree = ET.parse(proj)
root = tree.getroot()
compile_group = None
include_group = None
for ig in root.findall('ms:ItemGroup', ns):
    if ig.find('ms:ClCompile', ns) is not None:
        compile_group = ig
    if ig.find('ms:ClInclude', ns) is not None:
        include_group = ig

cpp_files = ['CircleCollider.cpp', 'RectCollider.cpp']
h_files = ['CircleCollider.h', 'RectCollider.h']

existing_cpps = [c.get('Include') for c in compile_group.findall('ms:ClCompile', ns)]
existing_hs = [c.get('Include') for c in include_group.findall('ms:ClInclude', ns)]

for f in cpp_files:
    if f not in existing_cpps:
        el = ET.SubElement(compile_group, 'ClCompile')
        el.set('Include', f)
        
for f in h_files:
    if f not in existing_hs:
        el = ET.SubElement(include_group, 'ClInclude')
        el.set('Include', f)

tree.write(proj, encoding='utf-8', xml_declaration=True)

# filters
tree_f = ET.parse(filters)
root_f = tree_f.getroot()
compile_group_f = None
include_group_f = None
for ig in root_f.findall('ms:ItemGroup', ns):
    if ig.find('ms:ClCompile', ns) is not None:
        compile_group_f = ig
    elif ig.find('ms:ClInclude', ns) is not None:
        include_group_f = ig

for f in cpp_files:
    if f not in existing_cpps:
        el = ET.SubElement(compile_group_f, 'ClCompile')
        el.set('Include', f)
        filt = ET.SubElement(el, 'Filter')
        filt.text = 'ソース ファイル'

for f in h_files:
    if f not in existing_hs:
        el = ET.SubElement(include_group_f, 'ClInclude')
        el.set('Include', f)
        filt = ET.SubElement(el, 'Filter')
        filt.text = 'ヘッダー ファイル'

tree_f.write(filters, encoding='utf-8', xml_declaration=True)

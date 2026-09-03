import os
import xml.etree.ElementTree as ET

DIR = r'C:\Users\student\Desktop\TeamGame\TeamGame\TeamGame'

files = {
    'Vector2.h': '''#pragma once

struct Vector2
{
    float x;
    float y;

    Vector2() : x(0.0f), y(0.0f) {}
    Vector2(float x, float y) : x(x), y(y) {}

    Vector2 operator+(const Vector2& other) const { return Vector2(x + other.x, y + other.y); }
    Vector2 operator-(const Vector2& other) const { return Vector2(x - other.x, y - other.y); }
    Vector2& operator+=(const Vector2& other) { x += other.x; y += other.y; return *this; }
    Vector2& operator-=(const Vector2& other) { x -= other.x; y -= other.y; return *this; }
    Vector2 operator*(float scalar) const { return Vector2(x * scalar, y * scalar); }
    Vector2 operator/(float scalar) const { return Vector2(x / scalar, y / scalar); }
};
''',

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

    Vector2 GetPosition() const { return position; }
    void SetPosition(const Vector2& pos) { position = pos; }

    float GetWidth() const { return width; }
    float GetHeight() const { return height; }

    bool IsActive() const { return isActive; }
    void SetActive(bool active) { isActive = active; }
};
''',

    'Object2D.cpp': '''#include "Object2D.h"
#include "ObjectManager.h"

Object2D::Object2D() : position(0.0f, 0.0f), width(0.0f), height(0.0f), isActive(true)
{
    ObjectManager::GetInstance().AddObject(this);
}

Object2D::~Object2D()
{
}
''',

    'ColliderManager.h': '''#pragma once
#include "Collider.h"
#include <vector>

class ColliderManager
{
private:
    std::vector<Collider*> colliders;

    ColliderManager();
    ~ColliderManager();

public:
    static ColliderManager& GetInstance()
    {
        static ColliderManager instance;
        return instance;
    }

    ColliderManager(const ColliderManager&) = delete;
    ColliderManager& operator=(const ColliderManager&) = delete;

    void AddCollider(Collider* collider);
    void RemoveCollider(Collider* collider);
    void CheckAllCollisions();
    void Clear();
};
''',

    'ColliderManager.cpp': '''#include "ColliderManager.h"

ColliderManager::ColliderManager() {}
ColliderManager::~ColliderManager() { Clear(); }

void ColliderManager::AddCollider(Collider* collider) {
    colliders.push_back(collider);
}

void ColliderManager::RemoveCollider(Collider* collider) {
    auto it = colliders.begin();
    while (it != colliders.end()) {
        if (*it == collider) {
            it = colliders.erase(it);
            return;
        } else {
            ++it;
        }
    }
}

void ColliderManager::CheckAllCollisions() {
    for (size_t i = 0; i < colliders.size(); ++i) {
        for (size_t j = i + 1; j < colliders.size(); ++j) {
            if (colliders[i]->IsCollision(colliders[j])) {
                // TODO: Collision Callback
            }
        }
    }
}

void ColliderManager::Clear() {
    colliders.clear();
}
''',

    'Collider.cpp': '''#include "Collider.h"
#include "Object2D.h"

Collider::Collider(Object2D* owner, float radius) : type(ColliderType::Circle), owner(owner), radius(radius), width(0), height(0), offsetX(0), offsetY(0) {}
Collider::Collider(Object2D* owner, float w, float h) : type(ColliderType::Rectangle), owner(owner), radius(0), width(w), height(h), offsetX(0), offsetY(0) {}
Collider::~Collider() {}

float Collider::GetWorldX() const { return owner ? owner->GetPosition().x + offsetX : offsetX; }
float Collider::GetWorldY() const { return owner ? owner->GetPosition().y + offsetY : offsetY; }

bool Collider::IsCollision(Collider* other) const {
    if (!other) return false;
    
    if (type == ColliderType::Circle && other->type == ColliderType::Circle) {
        float dx = GetWorldX() - other->GetWorldX();
        float dy = GetWorldY() - other->GetWorldY();
        float distSq = dx * dx + dy * dy;
        float rSum = radius + other->radius;
        return distSq <= rSum * rSum;
    }
    else if (type == ColliderType::Rectangle && other->type == ColliderType::Rectangle) {
        float l1 = GetWorldX() - width / 2;
        float r1 = GetWorldX() + width / 2;
        float t1 = GetWorldY() - height / 2;
        float b1 = GetWorldY() + height / 2;
        float l2 = other->GetWorldX() - other->width / 2;
        float r2 = other->GetWorldX() + other->width / 2;
        float t2 = other->GetWorldY() - other->height / 2;
        float b2 = other->GetWorldY() + other->height / 2;
        return l1 < r2 && r1 > l2 && t1 < b2 && b1 > t2;
    }
    return false;
}
''',

    'Character.h': '''#pragma once
#include "Object2D.h"

class Collider;

class Character : public Object2D
{
protected:
    Collider* collider;
    float speed;
    float radius;

public:
    Character(float startX, float startY, float radius);
    virtual ~Character();

    Collider* GetCollider() const { return collider; }
};
''',

    'Character.cpp': '''#include "Character.h"
#include "Collider.h"
#include "ColliderManager.h"

Character::Character(float startX, float startY, float rad) : speed(0.0f), radius(rad)
{
    position = Vector2(startX, startY);
    width = rad * 2.0f;
    height = rad * 2.0f;
    
    collider = new Collider(this, rad);
    ColliderManager::GetInstance().AddCollider(collider);
}

Character::~Character()
{
    ColliderManager::GetInstance().RemoveCollider(collider);
    delete collider;
}
''',

    'Player.h': '''#pragma once
#include "Character.h"

class Player : public Character
{
public:
    Player(float startX, float startY);
    virtual ~Player();

    void Update() override;
    void Draw() override;
};
''',

    'Player.cpp': '''#include "Player.h"
#include "InputManager.h"
#include "DxLib.h"

Player::Player(float startX, float startY) : Character(startX, startY, 20.0f)
{
    speed = 5.0f;
}

Player::~Player() {}

void Player::Update()
{
    if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_LEFT))  position.x -= speed;
    if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_RIGHT)) position.x += speed;
    if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_UP))    position.y -= speed;
    if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_DOWN))  position.y += speed;
}

void Player::Draw()
{
    DrawCircle(static_cast<int>(position.x), static_cast<int>(position.y), static_cast<int>(radius), GetColor(0, 255, 0), TRUE);
}
''',

    'Enemy.h': '''#pragma once
#include "Character.h"

class Enemy : public Character
{
public:
    Enemy(float startX, float startY);
    virtual ~Enemy();

    void Update() override;
    void Draw() override;
};
''',

    'Enemy.cpp': '''#include "Enemy.h"
#include "DxLib.h"

Enemy::Enemy(float startX, float startY) : Character(startX, startY, 25.0f)
{
    speed = 2.0f;
}

Enemy::~Enemy() {}

void Enemy::Update()
{
    position.y += speed; // Simple logic
}

void Enemy::Draw()
{
    DrawCircle(static_cast<int>(position.x), static_cast<int>(position.y), static_cast<int>(radius), GetColor(255, 0, 0), TRUE);
}
'''
}

for name, content in files.items():
    with open(os.path.join(DIR, name), 'w', encoding='utf-8') as f:
        f.write(content)

# Update GameScene to also spawn an Enemy and Check Collisions
game_scene_path = os.path.join(DIR, 'GameScene.cpp')
with open(game_scene_path, 'r', encoding='utf-8') as f:
    code = f.read()
    
if '#include "Enemy.h"' not in code:
    code = code.replace('#include "Player.h"', '#include "Player.h"\n#include "Enemy.h"\n#include "ColliderManager.h"')
    
code = code.replace('ObjectManager::GetInstance().Clear();', 'ObjectManager::GetInstance().Clear();\n    ColliderManager::GetInstance().Clear();')
if 'new Enemy' not in code:
    code = code.replace('new Player(320.0f, 240.0f);', 'new Player(320.0f, 240.0f);\n    new Enemy(400.0f, 100.0f);')
if 'ColliderManager::GetInstance().CheckAllCollisions();' not in code:
    code = code.replace('ObjectManager::GetInstance().Update();', 'ObjectManager::GetInstance().Update();\n    ColliderManager::GetInstance().CheckAllCollisions();')

with open(game_scene_path, 'w', encoding='utf-8') as f:
    f.write(code)

# Add new files to vcxproj and filters
proj = os.path.join(DIR, 'TeamGame.vcxproj')
filters = os.path.join(DIR, 'TeamGame.vcxproj.filters')

ET.register_namespace('', 'http://schemas.microsoft.com/developer/msbuild/2003')
ns = {'ms': 'http://schemas.microsoft.com/developer/msbuild/2003'}

# vcxproj
tree = ET.parse(proj)
root = tree.getroot()
compile_group = None
include_group = None
for ig in root.findall('ms:ItemGroup', ns):
    if ig.find('ms:ClCompile', ns) is not None:
        compile_group = ig
    if ig.find('ms:ClInclude', ns) is not None:
        include_group = ig

cpp_files = ['Character.cpp', 'Enemy.cpp']
h_files = ['Vector2.h', 'Character.h', 'Enemy.h']

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

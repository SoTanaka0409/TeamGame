import os
import xml.etree.ElementTree as ET

DIR = r'C:\Users\student\Desktop\TeamGame\TeamGame\TeamGame'

files = {
    'Object2D.h': '''#pragma once
class Object2D {
protected:
    float x, y;
    float width, height;
    bool isActive;
public:
    Object2D() : x(0.0f), y(0.0f), width(0.0f), height(0.0f), isActive(true) {}
    virtual ~Object2D() {}
    virtual void Update() = 0;
    virtual void Draw() = 0;
    float GetX() const { return x; }
    float GetY() const { return y; }
    float GetWidth() const { return width; }
    float GetHeight() const { return height; }
    bool IsActive() const { return isActive; }
    void SetActive(bool active) { isActive = active; }
};
''',
    
    'Object2D.cpp': '''#include "Object2D.h"
''',

    'ObjectManager.h': '''#pragma once
#include <vector>
#include <memory>
#include "Object2D.h"

class ObjectManager {
private:
    std::vector<std::shared_ptr<Object2D>> objects;
public:
    ObjectManager();
    ~ObjectManager();
    void AddObject(std::shared_ptr<Object2D> obj);
    void Update();
    void Draw();
    void RemoveDestroyedObjects();
    void Clear();
};
''',

    'ObjectManager.cpp': '''#include "ObjectManager.h"

ObjectManager::ObjectManager() {}
ObjectManager::~ObjectManager() { Clear(); }

void ObjectManager::AddObject(std::shared_ptr<Object2D> obj) {
    objects.push_back(obj);
}

void ObjectManager::Update() {
    for (auto& obj : objects) {
        if (obj->IsActive()) obj->Update();
    }
}

void ObjectManager::Draw() {
    for (auto& obj : objects) {
        if (obj->IsActive()) obj->Draw();
    }
}

void ObjectManager::RemoveDestroyedObjects() {
    auto it = objects.begin();
    while (it != objects.end()) {
        if (!(*it)->IsActive()) it = objects.erase(it);
        else ++it;
    }
}

void ObjectManager::Clear() {
    objects.clear();
}
''',

    'Collider.h': '''#pragma once

class Object2D;

enum class ColliderType {
    Rectangle,
    Circle
};

class Collider {
private:
    ColliderType type;
    Object2D* owner;
    float radius; // For circle
    float width, height; // For rectangle
    float offsetX, offsetY;

public:
    Collider(Object2D* owner, float radius); // Circle constructor
    Collider(Object2D* owner, float w, float h); // Rect constructor
    ~Collider();

    ColliderType GetType() const { return type; }
    Object2D* GetOwner() const { return owner; }
    
    float GetWorldX() const;
    float GetWorldY() const;
    float GetRadius() const { return radius; }
    float GetWidth() const { return width; }
    float GetHeight() const { return height; }

    bool IsCollision(Collider* other) const;
};
''',

    'Collider.cpp': '''#include "Collider.h"
#include "Object2D.h"
#include <cmath>

Collider::Collider(Object2D* owner, float radius) : type(ColliderType::Circle), owner(owner), radius(radius), width(0), height(0), offsetX(0), offsetY(0) {}

Collider::Collider(Object2D* owner, float w, float h) : type(ColliderType::Rectangle), owner(owner), radius(0), width(w), height(h), offsetX(0), offsetY(0) {}

Collider::~Collider() {}

float Collider::GetWorldX() const { return owner ? owner->GetX() + offsetX : offsetX; }
float Collider::GetWorldY() const { return owner ? owner->GetY() + offsetY : offsetY; }

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
    // Circle vs Rect would go here (simplified for now)
    return false;
}
''',

    'ColliderManager.h': '''#pragma once
#include <vector>
#include "Collider.h"

class ColliderManager {
private:
    std::vector<Collider*> colliders;

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
        } else {
            ++it;
        }
    }
}

void ColliderManager::CheckAllCollisions() {
    for (size_t i = 0; i < colliders.size(); ++i) {
        for (size_t j = i + 1; j < colliders.size(); ++j) {
            if (colliders[i]->IsCollision(colliders[j])) {
                // Handle collision callback here if needed
            }
        }
    }
}

void ColliderManager::Clear() {
    colliders.clear();
}
''',

    'BaseScene.h': '''#pragma once

class BaseScene {
public:
    BaseScene() {}
    virtual ~BaseScene() {}
    virtual void Init() = 0;
    virtual void Update() = 0;
    virtual void Draw() = 0;
    virtual void Finalize() = 0;
};
''',

    'SceneManager.h': '''#pragma once
#include <memory>
#include "BaseScene.h"

class SceneManager {
private:
    std::shared_ptr<BaseScene> currentScene;
    std::shared_ptr<BaseScene> nextScene;

public:
    SceneManager();
    ~SceneManager();

    void ChangeScene(std::shared_ptr<BaseScene> newScene);
    void Update();
    void Draw();
};
''',

    'SceneManager.cpp': '''#include "SceneManager.h"

SceneManager::SceneManager() : currentScene(nullptr), nextScene(nullptr) {}
SceneManager::~SceneManager() {}

void SceneManager::ChangeScene(std::shared_ptr<BaseScene> newScene) {
    nextScene = newScene;
}

void SceneManager::Update() {
    if (nextScene) {
        if (currentScene) currentScene->Finalize();
        currentScene = nextScene;
        currentScene->Init();
        nextScene = nullptr;
    }
    if (currentScene) currentScene->Update();
}

void SceneManager::Draw() {
    if (currentScene) currentScene->Draw();
}
''',

    'SoundManager.h': '''#pragma once
#include <string>
#include <unordered_map>
#include "DxLib.h"

class SoundManager {
private:
    std::unordered_map<std::string, int> sounds;

public:
    SoundManager();
    ~SoundManager();

    void Load(const std::string& key, const std::string& path);
    void Play(const std::string& key, bool loop = false);
    void Stop(const std::string& key);
    void StopAll();
    void Clear();
};
''',

    'SoundManager.cpp': '''#include "SoundManager.h"

SoundManager::SoundManager() {}
SoundManager::~SoundManager() { Clear(); }

void SoundManager::Load(const std::string& key, const std::string& path) {
    if (sounds.find(key) == sounds.end()) {
        int handle = LoadSoundMem(path.c_str());
        if (handle != -1) {
            sounds[key] = handle;
        }
    }
}

void SoundManager::Play(const std::string& key, bool loop) {
    auto it = sounds.find(key);
    if (it != sounds.end()) {
        PlaySoundMem(it->second, DX_PLAYTYPE_BACK | (loop ? DX_PLAYTYPE_LOOP : 0));
    }
}

void SoundManager::Stop(const std::string& key) {
    auto it = sounds.find(key);
    if (it != sounds.end()) {
        StopSoundMem(it->second);
    }
}

void SoundManager::StopAll() {
    InitSoundMem(); // DXLib specific to stop all sounds
}

void SoundManager::Clear() {
    InitSoundMem();
    sounds.clear();
}
''',

    'EffectManager.h': '''#pragma once
#include <vector>

// 簡易エフェクト管理クラス (基本形式)
class Effect {
public:
    float x, y;
    int lifeTimer;
    int maxLife;
    
    Effect(float x, float y, int life) : x(x), y(y), lifeTimer(life), maxLife(life) {}
    virtual ~Effect() {}
    virtual void Update() { if (lifeTimer > 0) lifeTimer--; }
    virtual void Draw() = 0;
    bool IsDead() const { return lifeTimer <= 0; }
};

class EffectManager {
private:
    std::vector<Effect*> effects;

public:
    EffectManager();
    ~EffectManager();

    void AddEffect(Effect* effect);
    void Update();
    void Draw();
    void Clear();
};
''',

    'EffectManager.cpp': '''#include "EffectManager.h"

EffectManager::EffectManager() {}
EffectManager::~EffectManager() { Clear(); }

void EffectManager::AddEffect(Effect* effect) {
    effects.push_back(effect);
}

void EffectManager::Update() {
    auto it = effects.begin();
    while (it != effects.end()) {
        (*it)->Update();
        if ((*it)->IsDead()) {
            delete *it;
            it = effects.erase(it);
        } else {
            ++it;
        }
    }
}

void EffectManager::Draw() {
    for (auto eff : effects) {
        eff->Draw();
    }
}

void EffectManager::Clear() {
    for (auto eff : effects) {
        delete eff;
    }
    effects.clear();
}
'''
}

for name, content in files.items():
    with open(os.path.join(DIR, name), 'w', encoding='utf-8') as f:
        f.write(content)

# Update vcxproj
proj = os.path.join(DIR, 'TeamGame.vcxproj')
ET.register_namespace('', 'http://schemas.microsoft.com/developer/msbuild/2003')
tree = ET.parse(proj)
root = tree.getroot()
ns = {'ms': 'http://schemas.microsoft.com/developer/msbuild/2003'}

# Find or create ItemGroups for ClCompile and ClInclude
compile_group = None
include_group = None

for ig in root.findall('ms:ItemGroup', ns):
    if ig.find('ms:ClCompile', ns) is not None:
        compile_group = ig
    if ig.find('ms:ClInclude', ns) is not None:
        include_group = ig

if compile_group is None:
    compile_group = ET.SubElement(root, 'ItemGroup')
if include_group is None:
    include_group = ET.SubElement(root, 'ItemGroup')

existing_cpps = [c.get('Include') for c in compile_group.findall('ms:ClCompile', ns)]
existing_hs = [c.get('Include') for c in include_group.findall('ms:ClInclude', ns)]

for name in files.keys():
    if name.endswith('.cpp') and name not in existing_cpps:
        el = ET.SubElement(compile_group, 'ClCompile')
        el.set('Include', name)
    elif name.endswith('.h') and name not in existing_hs:
        el = ET.SubElement(include_group, 'ClInclude')
        el.set('Include', name)

tree.write(proj, encoding='utf-8', xml_declaration=True)

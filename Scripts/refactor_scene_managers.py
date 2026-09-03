import os
import xml.etree.ElementTree as ET

DIR = r'C:\Users\student\Desktop\TeamGame\TeamGame\TeamGame'

files = {
    'Scene.h': '''#pragma once
#include "ObjectManager.h"
#include "ColliderManager.h"

class Scene
{
protected:
    ObjectManager* objectManager;
    ColliderManager* colliderManager;

public:
    Scene();
    virtual ~Scene();

    virtual void Init() {}
    virtual void Update();
    virtual void Draw();
    virtual void Finalize() {}

    ObjectManager* GetObjectManager() const { return objectManager; }
    ColliderManager* GetColliderManager() const { return colliderManager; }
};
''',

    'Scene.cpp': '''#include "Scene.h"

Scene::Scene()
{
    objectManager = new ObjectManager();
    colliderManager = new ColliderManager();
}

Scene::~Scene()
{
    delete objectManager;
    delete colliderManager;
}

void Scene::Update()
{
    objectManager->Update();
    colliderManager->CheckAllCollisions();
    objectManager->RemoveDestroyedObjects();
}

void Scene::Draw()
{
    objectManager->Draw();
}
''',

    'SceneManager.h': '''#pragma once
#include "Scene.h"
#include <memory>

class SceneManager
{
private:
    std::shared_ptr<Scene> currentScene;
    std::shared_ptr<Scene> nextScene;

    SceneManager();
    ~SceneManager();

public:
    static SceneManager& GetInstance()
    {
        static SceneManager instance;
        return instance;
    }

    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;

    void ChangeScene(std::shared_ptr<Scene> newScene);
    void Update();
    void Draw();

    std::shared_ptr<Scene> GetCurrentScene() const { return currentScene; }
};
''',

    'ObjectManager.h': '''#pragma once
#include <vector>

class Object2D;

class ObjectManager
{
private:
    std::vector<Object2D*> objects;

public:
    ObjectManager();
    ~ObjectManager();

    void AddObject(Object2D* obj);
    void Update();
    void Draw();
    void RemoveDestroyedObjects();
    void Clear();
};
''',

    'ColliderManager.h': '''#pragma once
#include "Collider.h"
#include <vector>

class ColliderManager
{
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

    'Object2D.cpp': '''#include "Object2D.h"
#include "SceneManager.h"
#include "Scene.h"
#include "ObjectManager.h"

Object2D::Object2D() : position(0.0f, 0.0f), width(0.0f), height(0.0f), isActive(true)
{
    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (scene)
    {
        scene->GetObjectManager()->AddObject(this);
    }
}

Object2D::~Object2D()
{
}
''',

    'Character.cpp': '''#include "Character.h"
#include "Collider.h"
#include "SceneManager.h"
#include "Scene.h"
#include "ColliderManager.h"

Character::Character(float startX, float startY, float rad) : speed(0.0f), radius(rad)
{
    position = Vector2(startX, startY);
    width = rad * 2.0f;
    height = rad * 2.0f;

    collider = new Collider(this, rad);
    
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

    'TitleScene.h': '''#pragma once
#include "Scene.h"

class TitleScene : public Scene
{
public:
    TitleScene();
    ~TitleScene() override;

    void Update() override;
    void Draw() override;
};
''',

    'TitleScene.cpp': '''#include "TitleScene.h"
#include "GameScene.h"
#include "SceneManager.h"
#include "InputManager.h"
#include "DxLib.h"

TitleScene::TitleScene() {}
TitleScene::~TitleScene() {}

void TitleScene::Update()
{
    Scene::Update(); // Update objects in scene

    if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_RETURN))
    {
        SceneManager::GetInstance().ChangeScene(std::make_shared<GameScene>());
    }
}

void TitleScene::Draw()
{
    Scene::Draw();
    DrawString(100, 100, "=== タイトル画面 ===", GetColor(255, 255, 255));
    DrawString(100, 150, "Enterキーを押してスタート", GetColor(200, 200, 200));
}
''',

    'GameScene.h': '''#pragma once
#include "Scene.h"

class GameScene : public Scene
{
public:
    GameScene();
    ~GameScene() override;

    void Init() override;
    void Update() override;
    void Draw() override;
};
''',

    'GameScene.cpp': '''#include "GameScene.h"
#include "ResultScene.h"
#include "SceneManager.h"
#include "InputManager.h"
#include "Player.h"
#include "Enemy.h"
#include "DxLib.h"

GameScene::GameScene() {}
GameScene::~GameScene() {}

void GameScene::Init()
{
    Scene::Init();
    new Player(320.0f, 240.0f);
    new Enemy(400.0f, 100.0f);
}

void GameScene::Update()
{
    Scene::Update(); // 自身の持つobjectManagerやcolliderManagerが実行される

    if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_SPACE))
    {
        SceneManager::GetInstance().ChangeScene(std::make_shared<ResultScene>());
    }
}

void GameScene::Draw()
{
    Scene::Draw();
    DrawString(10, 10, "=== ゲーム画面 === (Spaceキーでリザルトへ)", GetColor(255, 255, 255));
}
''',

    'ResultScene.h': '''#pragma once
#include "Scene.h"

class ResultScene : public Scene
{
public:
    ResultScene();
    ~ResultScene() override;

    void Update() override;
    void Draw() override;
};
''',

    'ResultScene.cpp': '''#include "ResultScene.h"
#include "TitleScene.h"
#include "SceneManager.h"
#include "InputManager.h"
#include "DxLib.h"

ResultScene::ResultScene() {}
ResultScene::~ResultScene() {}

void ResultScene::Update()
{
    Scene::Update();

    if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_RETURN))
    {
        SceneManager::GetInstance().ChangeScene(std::make_shared<TitleScene>());
    }
}

void ResultScene::Draw()
{
    Scene::Draw();
    DrawString(100, 100, "=== リザルト画面 ===", GetColor(255, 255, 255));
    DrawString(100, 150, "Enterキーを押してタイトルに戻る", GetColor(200, 200, 200));
}
'''
}

for name, content in files.items():
    with open(os.path.join(DIR, name), 'w', encoding='utf-8') as f:
        f.write(content)

# Update vcxproj and filters
proj = os.path.join(DIR, 'TeamGame.vcxproj')
filters = os.path.join(DIR, 'TeamGame.vcxproj.filters')

ET.register_namespace('', 'http://schemas.microsoft.com/developer/msbuild/2003')
ns = {'ms': 'http://schemas.microsoft.com/developer/msbuild/2003'}

# Delete BaseScene.h and BaseScene.cpp (if exists) from vcxproj and filters
def remove_from_xml(tree, filename):
    root = tree.getroot()
    for ig in root.findall('ms:ItemGroup', ns):
        for el in ig:
            if el.get('Include') == filename:
                ig.remove(el)

# Add Scene.h and Scene.cpp
def add_to_xml(tree, filename, is_filter=False):
    root = tree.getroot()
    compile_group = None
    include_group = None
    for ig in root.findall('ms:ItemGroup', ns):
        if ig.find('ms:ClCompile', ns) is not None:
            compile_group = ig
        if ig.find('ms:ClInclude', ns) is not None:
            include_group = ig
            
    # fallback
    if compile_group is None:
        compile_group = ET.SubElement(root, 'ItemGroup')
    if include_group is None:
        include_group = ET.SubElement(root, 'ItemGroup')

    if filename.endswith('.cpp'):
        if not any(c.get('Include') == filename for c in compile_group.findall('ms:ClCompile', ns)):
            el = ET.SubElement(compile_group, 'ClCompile')
            el.set('Include', filename)
            if is_filter:
                filt = ET.SubElement(el, 'Filter')
                filt.text = 'ソース ファイル'
    else:
        if not any(c.get('Include') == filename for c in include_group.findall('ms:ClInclude', ns)):
            el = ET.SubElement(include_group, 'ClInclude')
            el.set('Include', filename)
            if is_filter:
                filt = ET.SubElement(el, 'Filter')
                filt.text = 'ヘッダー ファイル'

# Vcxproj
tree = ET.parse(proj)
remove_from_xml(tree, 'BaseScene.h')
add_to_xml(tree, 'Scene.h')
add_to_xml(tree, 'Scene.cpp')
tree.write(proj, encoding='utf-8', xml_declaration=True)

# Filters
tree_f = ET.parse(filters)
remove_from_xml(tree_f, 'BaseScene.h')
add_to_xml(tree_f, 'Scene.h', True)
add_to_xml(tree_f, 'Scene.cpp', True)
tree_f.write(filters, encoding='utf-8', xml_declaration=True)

# Also physically delete BaseScene.h
base_scene_path = os.path.join(DIR, 'BaseScene.h')
if os.path.exists(base_scene_path):
    os.remove(base_scene_path)

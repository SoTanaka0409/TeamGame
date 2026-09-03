import os

DIR = r'C:\Users\student\Desktop\TeamGame\TeamGame\TeamGame'

files = {
    'ObjectManager.h': '''#pragma once
#include <vector>

class Object2D;

class ObjectManager
{
private:
    std::vector<Object2D*> objects;

    ObjectManager();
    ~ObjectManager();

public:
    static ObjectManager& GetInstance()
    {
        static ObjectManager instance;
        return instance;
    }

    ObjectManager(const ObjectManager&) = delete;
    ObjectManager& operator=(const ObjectManager&) = delete;

    void AddObject(Object2D* obj);
    void Update();
    void Draw();
    void RemoveDestroyedObjects();
    void Clear();
};
''',

    'ObjectManager.cpp': '''#include "ObjectManager.h"
#include "Object2D.h"

ObjectManager::ObjectManager()
{
}

ObjectManager::~ObjectManager()
{
    Clear();
}

void ObjectManager::AddObject(Object2D* obj)
{
    objects.push_back(obj);
}

void ObjectManager::Update()
{
    for (auto obj : objects)
    {
        if (obj->IsActive())
        {
            obj->Update();
        }
    }
}

void ObjectManager::Draw()
{
    for (auto obj : objects)
    {
        if (obj->IsActive())
        {
            obj->Draw();
        }
    }
}

void ObjectManager::RemoveDestroyedObjects()
{
    auto it = objects.begin();
    while (it != objects.end())
    {
        if (!(*it)->IsActive())
        {
            delete *it;
            it = objects.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void ObjectManager::Clear()
{
    for (auto obj : objects)
    {
        delete obj;
    }
    objects.clear();
}
''',

    'Object2D.h': '''#pragma once

class Object2D
{
protected:
    float x, y;
    float width, height;
    bool isActive;

public:
    Object2D();
    virtual ~Object2D();

    virtual void Update() = 0;
    virtual void Draw() = 0;

    float GetX() const
    {
        return x;
    }
    float GetY() const
    {
        return y;
    }
    float GetWidth() const
    {
        return width;
    }
    float GetHeight() const
    {
        return height;
    }

    bool IsActive() const
    {
        return isActive;
    }
    void SetActive(bool active)
    {
        isActive = active;
    }
};
''',

    'Object2D.cpp': '''#include "Object2D.h"
#include "ObjectManager.h"

Object2D::Object2D() : x(0.0f), y(0.0f), width(0.0f), height(0.0f), isActive(true)
{
    ObjectManager::GetInstance().AddObject(this);
}

Object2D::~Object2D()
{
}
'''
}

for name, content in files.items():
    with open(os.path.join(DIR, name), 'w', encoding='utf-8') as f:
        f.write(content)

# Update main.cpp
main_path = os.path.join(DIR, 'main.cpp')
with open(main_path, 'r', encoding='utf-8') as f:
    main_code = f.read()

main_code = main_code.replace('ObjectManager objManager;', '')
main_code = main_code.replace('objManager.AddObject(std::make_shared<Player>(320.0f, 240.0f));', 'new Player(320.0f, 240.0f);')
main_code = main_code.replace('objManager.Update();', 'ObjectManager::GetInstance().Update();')
main_code = main_code.replace('objManager.Draw();', 'ObjectManager::GetInstance().Draw();')

with open(main_path, 'w', encoding='utf-8') as f:
    f.write(main_code)

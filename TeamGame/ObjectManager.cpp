#include "ObjectManager.h"
#include "Object2D.h"

ObjectManager::ObjectManager()
{
}

ObjectManager::~ObjectManager()
{
    Clear();
}

void ObjectManager::AddObject(Object2D *obj)
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

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
    if (!obj) return;
    for (auto existing : objects)
    {
        if (existing == obj) return;
    }
    objects.push_back(obj);
}

void ObjectManager::Update()
{
    size_t count = objects.size();
    for (size_t i = 0; i < count; ++i)
    {
        if (i < objects.size() && objects[i] && objects[i]->IsActive())
        {
            objects[i]->Update();
        }
    }
}

void ObjectManager::Draw()
{
    size_t count = objects.size();
    for (size_t i = 0; i < count; ++i)
    {
        if (i < objects.size() && objects[i] && objects[i]->IsActive())
        {
            objects[i]->Draw();
        }
    }
}

void ObjectManager::RemoveDestroyedObjects()
{
    auto it = objects.begin();
    while (it != objects.end())
    {
        if (*it == nullptr)
        {
            it = objects.erase(it);
        }
        else if ((*it)->IsMarkedForDeletion())
        {
            delete *it;
            it = objects.erase(it);
        }
        else if (!(*it)->IsActive() && (*it)->GetObjectTag() != ObjectTag::Player && (*it)->GetObjectTag() != ObjectTag::Enemy)
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

#include "Object2D.h"
#include "ObjectManager.h"
#include "Scene.h"
#include "SceneManager.h"

Object2D::Object2D(ObjectTag tag)
    : position(0.0f, 0.0f), width(0.0f), height(0.0f), isActive(true), objectTag(tag)
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

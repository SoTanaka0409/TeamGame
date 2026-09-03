#include "Character.h"
#include "ColliderManager.h"
#include "Scene.h"
#include "SceneManager.h"

Character::Character(float startX, float startY, float rad)
    : speed(0.0f), radius(rad), myColliderManager(nullptr)
{
    position = Vector2(startX, startY);
    width = rad * 2.0f;
    height = rad * 2.0f;

    collider = new CircleCollider(this, rad, "CharacterBody");

    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (scene)
    {
        myColliderManager = scene->GetColliderManager();
        myColliderManager->AddCollider(collider);
    }
}

Character::~Character()
{
    if (myColliderManager)
    {
        myColliderManager->RemoveCollider(collider);
    }
    delete collider;
}

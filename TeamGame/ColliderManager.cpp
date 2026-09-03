#include "ColliderManager.h"

ColliderManager::ColliderManager()
{
}
ColliderManager::~ColliderManager()
{
    Clear();
}

void ColliderManager::AddCollider(Collider *collider)
{
    colliders.push_back(collider);
}

void ColliderManager::RemoveCollider(Collider *collider)
{
    auto it = colliders.begin();
    while (it != colliders.end())
    {
        if (*it == collider)
        {
            it = colliders.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void ColliderManager::CheckAllCollisions()
{
    for (size_t i = 0; i < colliders.size(); ++i)
    {
        for (size_t j = i + 1; j < colliders.size(); ++j)
        {
            if (colliders[i]->IsCollision(colliders[j]))
            {
                // Handle collision callback here if needed
            }
        }
    }
}

void ColliderManager::Clear()
{
    colliders.clear();
}

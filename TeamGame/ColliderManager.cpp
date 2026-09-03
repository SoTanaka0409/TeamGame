#include "ColliderManager.h"
#include "Object2D.h"

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
            Collider *otherCollider =
                (itPrev->first == collider) ? itPrev->second : itPrev->first;
            Object2D *otherOwner = otherCollider->GetOwner();
            Object2D *thisOwner = collider->GetOwner();

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
    std::set<std::pair<Collider *, Collider *>> currentCollisions;

    for (size_t i = 0; i < colliders.size(); ++i)
    {
        for (size_t j = i + 1; j < colliders.size(); ++j)
        {
            Object2D *ownerA = colliders[i]->GetOwner();
            Object2D *ownerB = colliders[j]->GetOwner();

            if (!ownerA || !ownerB || !ownerA->IsActive() ||
                !ownerB->IsActive())
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

    for (auto &pair : currentCollisions)
    {
        bool isEnter =
            (previousCollisions.find(pair) == previousCollisions.end());

        Collider *colA = pair.first;
        Collider *colB = pair.second;
        Object2D *objA = colA->GetOwner();
        Object2D *objB = colB->GetOwner();

        if (!objA || !objB)
            continue;

        if (isEnter)
        {
            if (objA->IsActive())
                objA->OnCollisionEnter(colB);
            if (objB->IsActive())
                objB->OnCollisionEnter(colA);
        }
        else
        {
            if (objA->IsActive())
                objA->OnCollisionStay(colB);
            if (objB->IsActive())
                objB->OnCollisionStay(colA);
        }
    }

    for (auto &pair : previousCollisions)
    {
        if (currentCollisions.find(pair) == currentCollisions.end())
        {
            Collider *colA = pair.first;
            Collider *colB = pair.second;
            Object2D *objA = colA->GetOwner();
            Object2D *objB = colB->GetOwner();

            if (objA && objA->IsActive())
                objA->OnCollisionExit(colB);
            if (objB && objB->IsActive())
                objB->OnCollisionExit(colA);
        }
    }

    previousCollisions = currentCollisions;
}

void ColliderManager::Clear()
{
    colliders.clear();
    previousCollisions.clear();
}

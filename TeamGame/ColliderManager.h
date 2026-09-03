#pragma once
#include "Collider.h"
#include <set>
#include <utility>
#include <vector>

class ColliderManager
{
  private:
    std::vector<Collider *> colliders;
    std::set<std::pair<Collider *, Collider *>> previousCollisions;

  public:
    ColliderManager();
    ~ColliderManager();

    void AddCollider(Collider *collider);
    void RemoveCollider(Collider *collider);
    void CheckAllCollisions();
    void Clear();
};

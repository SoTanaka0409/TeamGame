#pragma once
#include "Collider.h"
#include <vector>

class ColliderManager
{
  private:
    std::vector<Collider *> colliders;

  public:
    ColliderManager();
    ~ColliderManager();

    void AddCollider(Collider *collider);
    void RemoveCollider(Collider *collider);
    void CheckAllCollisions();
    void Clear();
};

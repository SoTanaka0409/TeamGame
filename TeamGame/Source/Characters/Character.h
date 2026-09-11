#pragma once
#include "CircleCollider.h"
#include "Object2D.h"

#include "Status.h"

class Character : public Object2D
{
  protected:
    class ColliderManager *myColliderManager;
    CircleCollider *collider; // 今回は標準でCircleを持つように
    float radius;

  public:
    Status status;

    Character(ObjectTag tag, float startX, float startY, float radius);
    virtual ~Character();

    CircleCollider *GetCollider() const
    {
        return collider;
    }
};

#pragma once
#include "CircleCollider.h"
#include "Object2D.h"

class Character : public Object2D
{
  protected:
    class ColliderManager *myColliderManager;
    CircleCollider *collider; // 今回は標準でCircleを持つように
    float speed;
    float radius;

  public:
    Character(ObjectTag tag, float startX, float startY, float radius);
    virtual ~Character();

    CircleCollider *GetCollider() const
    {
        return collider;
    }
};

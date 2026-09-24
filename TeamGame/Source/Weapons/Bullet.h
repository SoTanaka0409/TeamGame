#pragma once
#include "CircleCollider.h"
#include "Object2D.h"
#include "Vector2.h"

class ColliderManager;

class Bullet : public Object2D
{
  private:
    CircleCollider *collider;
    ColliderManager *myColliderManager;
    Vector2 velocity;
    float radius;
    float maxRange;
    Vector2 startPos;
    int teamId;

  public:
    Bullet(float startX, float startY, const Vector2 &dir, float speed, float range, float radius, int tId = 0);
    virtual ~Bullet();

    void Update() override;
    void Draw() override;

    void OnCollisionEnter(Collider *otherCollider) override;
};

#pragma once
#include "CircleCollider.h"
#include "Object2D.h"
#include "Vector2.h"

class ColliderManager;

class EnemyBullet : public Object2D
{
  private:
    CircleCollider *collider;
    ColliderManager *myColliderManager;
    Vector2 velocity;
    float radius;

  public:
    EnemyBullet(float startX, float startY, const Vector2 &dir, float speed);
    virtual ~EnemyBullet();

    void Update() override;
    void Draw() override;

    void OnCollisionEnter(Collider *otherCollider) override;
};

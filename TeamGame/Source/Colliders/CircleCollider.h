#pragma once
#include "Collider.h"

class CircleCollider : public Collider
{
  private:
    float radius;

  public:
    CircleCollider(Object2D *owner, float radius, const std::string &tag = "");
    ~CircleCollider() override;

    float GetRadius() const
    {
        return radius;
    }
    void SetRadius(float r)
    {
        radius = r;
    }

    bool IsCollision(Collider *other) const override;
    bool IsCollisionWithCircle(const CircleCollider *circle) const override;
    bool IsCollisionWithRect(const RectCollider *rect) const override;
};

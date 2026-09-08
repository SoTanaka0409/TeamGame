#pragma once
#include "Collider.h"

class RectCollider : public Collider
{
  private:
    float width, height;

  public:
    RectCollider(Object2D *owner, float width, float height,
                 const std::string &tag = "");
    ~RectCollider() override;

    float GetWidth() const
    {
        return width;
    }
    float GetHeight() const
    {
        return height;
    }

    bool IsCollision(Collider *other) const override;
    bool IsCollisionWithCircle(const CircleCollider *circle) const override;
    bool IsCollisionWithRect(const RectCollider *rect) const override;
};

#include "RectCollider.h"
#include "CircleCollider.h"

RectCollider::RectCollider(Object2D *owner, float width, float height,
                           const std::string &tag)
    : Collider(owner, tag), width(width), height(height)
{
}

RectCollider::~RectCollider()
{
}

bool RectCollider::IsCollision(Collider *other) const
{
    return other->IsCollisionWithRect(this);
}

bool RectCollider::IsCollisionWithCircle(const CircleCollider *circle) const
{
    // Circle側の判定に任せる
    return circle->IsCollisionWithRect(this);
}

bool RectCollider::IsCollisionWithRect(const RectCollider *rect) const
{
    float l1 = GetWorldX() - width / 2;
    float r1 = GetWorldX() + width / 2;
    float t1 = GetWorldY() - height / 2;
    float b1 = GetWorldY() + height / 2;

    float l2 = rect->GetWorldX() - rect->GetWidth() / 2;
    float r2 = rect->GetWorldX() + rect->GetWidth() / 2;
    float t2 = rect->GetWorldY() - rect->GetHeight() / 2;
    float b2 = rect->GetWorldY() + rect->GetHeight() / 2;

    return l1 < r2 && r1 > l2 && t1 < b2 && b1 > t2;
}

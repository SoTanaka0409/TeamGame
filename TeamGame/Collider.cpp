#include "Collider.h"
#include "Object2D.h"
#include <cmath>

Collider::Collider(Object2D *owner, float radius)
    : type(ColliderType::Circle), owner(owner), radius(radius), width(0),
      height(0), offsetX(0), offsetY(0)
{
}

Collider::Collider(Object2D *owner, float w, float h)
    : type(ColliderType::Rectangle), owner(owner), radius(0), width(w),
      height(h), offsetX(0), offsetY(0)
{
}

Collider::~Collider()
{
}

float Collider::GetWorldX() const
{
    return owner ? owner->GetX() + offsetX : offsetX;
}
float Collider::GetWorldY() const
{
    return owner ? owner->GetY() + offsetY : offsetY;
}

bool Collider::IsCollision(Collider *other) const
{
    if (!other)
        return false;

    if (type == ColliderType::Circle && other->type == ColliderType::Circle)
    {
        float dx = GetWorldX() - other->GetWorldX();
        float dy = GetWorldY() - other->GetWorldY();
        float distSq = dx * dx + dy * dy;
        float rSum = radius + other->radius;
        return distSq <= rSum * rSum;
    }
    else if (type == ColliderType::Rectangle &&
             other->type == ColliderType::Rectangle)
    {
        float l1 = GetWorldX() - width / 2;
        float r1 = GetWorldX() + width / 2;
        float t1 = GetWorldY() - height / 2;
        float b1 = GetWorldY() + height / 2;

        float l2 = other->GetWorldX() - other->width / 2;
        float r2 = other->GetWorldX() + other->width / 2;
        float t2 = other->GetWorldY() - other->height / 2;
        float b2 = other->GetWorldY() + other->height / 2;

        return l1 < r2 && r1 > l2 && t1 < b2 && b1 > t2;
    }
    // Circle vs Rect would go here (simplified for now)
    return false;
}

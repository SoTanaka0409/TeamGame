#include "Collider.h"
#include "Object2D.h"

Collider::Collider(Object2D *owner, const std::string &tag)
    : owner(owner), offsetX(0.0f), offsetY(0.0f), tag(tag)
{
}

Collider::~Collider()
{
}

float Collider::GetWorldX() const
{
    return owner ? owner->GetPosition().x + offsetX : offsetX;
}

float Collider::GetWorldY() const
{
    return owner ? owner->GetPosition().y + offsetY : offsetY;
}

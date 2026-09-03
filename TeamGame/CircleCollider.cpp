#include "CircleCollider.h"
#include "RectCollider.h"

CircleCollider::CircleCollider(Object2D *owner, float radius,
                               const std::string &tag)
    : Collider(owner, tag), radius(radius)
{
}

CircleCollider::~CircleCollider()
{
}

bool CircleCollider::IsCollision(Collider *other) const
{
    // 相手が何型か分からないので、相手に「自分（Circle）と当たっているか？」と聞き返す（ダブルディスパッチ）
    return other->IsCollisionWithCircle(this);
}

bool CircleCollider::IsCollisionWithCircle(const CircleCollider *circle) const
{
    float dx = GetWorldX() - circle->GetWorldX();
    float dy = GetWorldY() - circle->GetWorldY();
    float distSq = dx * dx + dy * dy;
    float rSum = radius + circle->GetRadius();
    return distSq <= rSum * rSum;
}

bool CircleCollider::IsCollisionWithRect(const RectCollider *rect) const
{
    // 今回は簡易的に実装（本来は円と矩形の正確な判定が必要）
    float thisX = GetWorldX();
    float thisY = GetWorldY();
    float rectL = rect->GetWorldX() - rect->GetWidth() / 2;
    float rectR = rect->GetWorldX() + rect->GetWidth() / 2;
    float rectT = rect->GetWorldY() - rect->GetHeight() / 2;
    float rectB = rect->GetWorldY() + rect->GetHeight() / 2;

    float testX = thisX;
    float testY = thisY;

    if (thisX < rectL)
        testX = rectL;
    else if (thisX > rectR)
        testX = rectR;

    if (thisY < rectT)
        testY = rectT;
    else if (thisY > rectB)
        testY = rectB;

    float distX = thisX - testX;
    float distY = thisY - testY;
    float distance = (distX * distX) + (distY * distY);

    return distance <= (radius * radius);
}

#pragma once
#include <string>

class Object2D;
class CircleCollider;
class RectCollider;

class Collider
{
  protected:
    Object2D *owner;
    float offsetX, offsetY;
    std::string
        tag; // コライダーを識別するタグ（"Body", "WeakPoint", "Weapon"など）

  public:
    Collider(Object2D *owner, const std::string &tag = "");
    virtual ~Collider();

    Object2D *GetOwner() const
    {
        return owner;
    }
    std::string GetTag() const
    {
        return tag;
    }
    void SetTag(const std::string &t)
    {
        tag = t;
    }

    float GetWorldX() const;
    float GetWorldY() const;

    void SetOffset(float x, float y)
    {
        offsetX = x;
        offsetY = y;
    }

    // ダブルディスパッチ用関数群
    virtual bool IsCollision(Collider *other) const = 0;
    virtual bool IsCollisionWithCircle(const CircleCollider *circle) const = 0;
    virtual bool IsCollisionWithRect(const RectCollider *rect) const = 0;
};

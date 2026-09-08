#pragma once
#include "Vector2.h"

enum class ObjectTag
{
    None,
    Player,
    Enemy,
    PlayerWeapon,
    EnemyWeapon,
    Object
};

class Collider;

class Object2D
{
  protected:
    Vector2 position;
    float width, height;
    bool isActive;
    ObjectTag objectTag;

  public:
    Object2D(ObjectTag tag = ObjectTag::None);
    virtual ~Object2D();

    ObjectTag GetObjectTag() const { return objectTag; }

    virtual void Update() = 0;
    virtual void Draw() = 0;

    // 引数を Object2D* から Collider* に変更
    virtual void OnCollisionEnter(Collider *otherCollider)
    {
    }
    virtual void OnCollisionStay(Collider *otherCollider)
    {
    }
    virtual void OnCollisionExit(Collider *otherCollider)
    {
    }

    Vector2 GetPosition() const
    {
        return position;
    }
    void SetPosition(const Vector2 &pos)
    {
        position = pos;
    }

    float GetWidth() const
    {
        return width;
    }
    float GetHeight() const
    {
        return height;
    }

    bool IsActive() const
    {
        return isActive;
    }
    void SetActive(bool active)
    {
        isActive = active;
    }
};

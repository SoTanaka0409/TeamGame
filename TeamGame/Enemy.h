#pragma once
#include "Character.h"

class Enemy : public Character
{
  private:
    int hp;

  public:
    Enemy(float startX, float startY);
    virtual ~Enemy();

    void Update() override;
    void Draw() override;

    void Damage();

    void OnCollisionEnter(Collider *otherCollider) override;
    void OnCollisionStay(Collider *otherCollider) override;
    void OnCollisionExit(Collider *otherCollider) override;
};

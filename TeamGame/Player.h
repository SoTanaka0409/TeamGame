#pragma once
#include "Character.h"
#include <vector>
class Weapon;

class Player : public Character
{
  private:
    int damageColorTimer;
    Vector2 facingDir;
    std::vector<Weapon *> weapons;
    int currentWeaponIndex;

  public:
    Vector2 GetFacingDir() const
    {
        return facingDir;
    }
    Player(float startX, float startY);
    virtual ~Player();

    void Update() override;
    void Draw() override;

    // 引数が Collider* に変更
    void OnCollisionEnter(Collider *otherCollider) override;
    void OnCollisionStay(Collider *otherCollider) override;
    void OnCollisionExit(Collider *otherCollider) override;
};

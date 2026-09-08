#pragma once
#include "Character.h"
#include <vector>
class Weapon;

class Player : public Character
{
  private:
    class Stage* currentStage = nullptr;
    float cellSize = 1.0f;
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

    void SetStage(class Stage* s, float cSize) { currentStage = s; cellSize = cSize; }
    void Update() override;
    void Draw() override;

    // 引数が Collider* に変更
    void OnCollisionEnter(Collider *otherCollider) override;
    void OnCollisionStay(Collider *otherCollider) override;
    void OnCollisionExit(Collider *otherCollider) override;
};

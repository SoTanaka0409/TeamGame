#pragma once
#include "Character.h"
#include "Vector2.h"

enum class EnemyAIState
{
    PATROL,
    ALERT,
    INVESTIGATE
};

class Enemy : public Character
{
  private:
    int hp;
    int damageColorTimer;
    class Stage *currentStage;
    float cellSize;
    class Player *targetPlayer;

    EnemyAIState aiState;
    Vector2 facingDir;
    Vector2 moveDir;
    Vector2 lastKnownPos;
    int patrolChangeTimer;
    int investigateTimer;
    int shootCooldown;

  public:
    Enemy(float startX, float startY);
    virtual ~Enemy();

    void SetStage(class Stage *s, float cSize)
    {
        currentStage = s;
        cellSize = cSize;
    }
    void SetTargetPlayer(class Player *p)
    {
        targetPlayer = p;
    }

    void OnHearGunshot(const Vector2 &soundPos);

    void Update() override;
    void Draw() override;

    void Damage();
    bool CheckLineOfSightToPlayer() const;

    void MoveSmart(const Vector2 &desiredDir);

    void OnCollisionEnter(Collider *otherCollider) override;
    void OnCollisionStay(Collider *otherCollider) override;
    void OnCollisionExit(Collider *otherCollider) override;
};

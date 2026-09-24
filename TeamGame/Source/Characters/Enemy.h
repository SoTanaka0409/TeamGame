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
    int damageColorTimer;
    class Stage *currentStage;
    float cellSize;
    class Character *targetCharacter;

    EnemyAIState aiState;
    Vector2 facingDir;
    Vector2 moveDir;
    Vector2 lastKnownPos;
    int patrolChangeTimer;
    int investigateTimer;
    float currentInvestigateVolume;
    int shootCooldown;
    int strafeDirection;
    int strafeTimer;

  public:
    Enemy(float startX, float startY, int tId = 1);
    ~Enemy();

    void Update() override;
    void Draw() override;
    void OnCollisionEnter(Collider *otherCollider) override;
    void OnCollisionStay(Collider *otherCollider) override;
    void OnCollisionExit(Collider *otherCollider) override;

    void SetStage(class Stage *s, float cSize)
    {
        currentStage = s;
        cellSize = cSize;
    }
    
    // Nearest enemy logic
    void UpdateTarget();

    void OnHearGunshot(const Vector2 &soundPos, float maxDistance);
    void Damage();

    void StealthKill();
    bool IsAlerted() const { return aiState == EnemyAIState::ALERT; }
    EnemyAIState GetAIState() const { return aiState; }
    bool CheckLineOfSightToTarget() const;

    void MoveSmart(const Vector2 &desiredDir);
};

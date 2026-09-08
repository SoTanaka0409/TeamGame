#pragma once
#include "Character.h"
#include <vector>
#include <cmath>
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
    float GetX() const { return position.x; }
    float GetY() const { return position.y; }
    float GetLightAngle() const { return std::atan2(facingDir.y, facingDir.x); }
    Player(float startX, float startY);
    virtual ~Player();

    void SetStage(class Stage* s, float cSize) { currentStage = s; cellSize = cSize; }
    void TakeDamage();
    void Update() override;
    void Draw() override;

    // 引数が Collider* に変更
    void OnCollisionEnter(Collider *otherCollider) override;
    void OnCollisionStay(Collider *otherCollider) override;
    void OnCollisionExit(Collider *otherCollider) override;

    // 懐中電灯・スポットライト
    void RenderLightMask(int rectX, int rectY, int rectW, int rectH, float startDrawX, float startDrawY) const;

    bool IsLightOn() const { return m_isLightOn; }
    void ToggleLight() { m_isLightOn = !m_isLightOn; }
    bool IsInBush() const { return m_isInBush; }

  private:
    float m_lightAngle = 0.0f;  // 向いている角度
    bool m_isLightOn = true;    // 懐中電灯スイッチ
    bool m_isInBush = false;    // 草むらに隠れているか
    bool m_prevMouseRight = false; // 右クリック判定

    // 懐中電灯パラメーター
    float m_maxSpotDistCells = 14.0f;
    float m_closeRadiusCells = 2.0f;
    float m_fanAngleHalf = 0.5236f;
};

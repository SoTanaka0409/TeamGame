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
    bool m_isRemote = false;

  public:
    Vector2 GetFacingDir() const
    {
        return facingDir;
    }
    void SetRemote(bool remote) { m_isRemote = remote; }
    Player(float startX, float startY);
    virtual ~Player();

    void SetStage(class Stage* s, float cSize) { currentStage = s; cellSize = cSize; }
    void SetFacingDir(const Vector2& dir) { facingDir = dir; }
    void SetLightState(bool on) { m_isLightOn = on; }
    void SetBushState(bool inBush) { m_isInBush = inBush; }
    void SetCurrentWeaponIndex(int index) { currentWeaponIndex = index; }

    void Update() override;
    void Draw() override;

    // 引数めECollider* に変更
    void OnCollisionEnter(Collider *otherCollider) override;
    void OnCollisionStay(Collider *otherCollider) override;
    void OnCollisionExit(Collider *otherCollider) override;

    // 懐中電灯・スポットライチE
    void RenderLightMask(int rectX, int rectY, int rectW, int rectH, float startDrawX, float startDrawY) const;

    bool IsLightOn() const { return m_isLightOn; }
    void ToggleLight() { m_isLightOn = !m_isLightOn; }
    bool IsInBush() const { return m_isInBush; }

  private:
    float m_lightAngle = 0.0f;  // 向いてぁE角度
    bool m_isLightOn = true;    // 懐中電灯スイチE
    bool m_isInBush = false;    // 草Eらに隠れてぁEぁE
    bool m_prevMouseRight = false; // 右クリチE判?E
    bool m_isAimLocked = false; 
    bool m_prevE = false; // Eキーの押しっぱなし判定用?E

    // 懐中電灯パラメーター
    float m_maxSpotDistCells = 14.0f;
    float m_closeRadiusCells = 2.0f;
    float m_fanAngleHalf = 0.5236f;
};

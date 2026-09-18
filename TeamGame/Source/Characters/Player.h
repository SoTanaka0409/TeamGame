#pragma once
#include "Character.h"
#include <vector>
#include <cmath>
enum class PlayerInputType
{
    KEYBOARD_MOUSE,
    GAMEPAD_1
};

#include "../Weapons/Weapon.h"
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
    
    // ガジェット（スキル）の保持
    class Skill* currentSkill = nullptr;

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
    
    // 弾薬の回復（現在持っている武器の弾を回復する）
    void AddAmmo(int amount);

    
    void Update() override;
    void Draw() override;
    void DrawUI(int screenX, int screenY);

    // 引数が Collider* に変更
    void OnCollisionEnter(Collider *otherCollider) override;
    void OnCollisionStay(Collider *otherCollider) override;
    void OnCollisionExit(Collider *otherCollider) override;

    // 懐中電灯・スポットライト
    void RenderLightMask(int rectX, int rectY, int rectW, int rectH, float startDrawX, float startDrawY) const;

    bool IsLightOn() const { return m_isLightOn; }
    void ToggleLight() { m_isLightOn = !m_isLightOn; }
    bool IsInBush() const { return m_isInBush; }
    void SetInBush(bool val) { m_isInBush = val; }
    void SetFacingDir(const Vector2& dir) { facingDir = dir; }
    void SetRemote(bool val) { isRemote = val; }
    
    // スキルのセット
    void SetSkill(class Skill* skill) { currentSkill = skill; }
    
    void SetInputType(PlayerInputType type) { m_inputType = type; }
    PlayerInputType GetInputType() const { return m_inputType; }
    bool IsRemote() const { return isRemote; }

  private:
    float m_lightAngle = 0.0f;  // 向いている角度
    bool m_isLightOn = true;    // 懐中電灯スイッチ
    bool m_isInBush = false;    // 草むらに隠れているか
    bool m_prevMouseRight = false; // 右クリック判定
    bool isRemote = false;
    PlayerInputType m_inputType = PlayerInputType::KEYBOARD_MOUSE;

    // 懐中電灯パラメーター
    float m_maxSpotDistCells = 14.0f;
    float m_closeRadiusCells = 2.0f;
    float m_fanAngleHalf = 0.5236f;
};

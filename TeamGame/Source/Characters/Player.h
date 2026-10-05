#pragma once
#include "Character.h"
#include <vector>
#include <cmath>

/**
 * @brief プレイヤーの操作入力タイプ
 */
enum class PlayerInputType
{
    KEYBOARD_MOUSE, ///< キーボード＋マウス操作
    GAMEPAD_1       ///< ゲームパッド1操作
};

#include "../Weapons/Weapon.h"
class Weapon;

/**
 * @brief プレイヤーキャラクタークラス
 * @details ユーザーが操作するプレイヤーキャラクター。移動・エイム・射撃・スキル使用・UI描画を管理します。
 */
class Player : public Character
{
private:
    class Stage* currentStage = nullptr; ///< 現在のステージ
    float cellSize = 1.0f;               ///< ステージのマス目サイズ
    int damageColorTimer;                ///< 被弾イエロー点滅タイマー
    Vector2 facingDir;                   ///< エイム向きベクトル
    std::vector<Weapon *> weapons;       ///< 所持武器リスト
    int currentWeaponIndex;              ///< 現在装備中の武器インデックス
    int autoPingTimer;                   ///< オートピン送信タイマー
    bool m_isMoving = false;             ///< 移動中フラグ
    
    class Skill* currentSkill = nullptr; ///< 現在装備中のスキル（ガジェット）

public:
    /**
     * @brief エイム向きベクトルの取得
     */
    Vector2 GetFacingDir() const { return facingDir; }

    /**
     * @brief X座標の取得
     */
    float GetX() const { return position.x; }

    /**
     * @brief Y座標の取得
     */
    float GetY() const { return position.y; }

    /**
     * @brief ライト照射角度の取得
     */
    float GetLightAngle() const { return std::atan2(facingDir.y, facingDir.x); }

    /**
     * @brief コンストラクタ
     * @param startX 初期X座標
     * @param startY 初期Y座標
     */
    Player(float startX, float startY);

    /**
     * @brief デストラクタ
     */
    virtual ~Player();

    /**
     * @brief ステージポインタおよびマスサイズの設定
     */
    void SetStage(class Stage* s, float cSize) { currentStage = s; cellSize = cSize; }

    /**
     * @brief ダメージ受信処理
     */
    void TakeDamage(int amount = 1);
    
    /**
     * @brief 弾薬の回復・補充
     * @param amount 補充量
     */
    void AddAmmo(int amount);

    /**
     * @brief フレーム更新処理
     */
    void Update() override;

    /**
     * @brief 描画処理
     */
    void Draw() override;

    /**
     * @brief UI描画処理（HPゲージ、弾薬、スキル状態）
     */
    void DrawUI(int screenX, int screenY);

    /**
     * @brief 衝突開始時イベント
     */
    void OnCollisionEnter(Collider *otherCollider) override;

    /**
     * @brief 衝突継続時イベント
     */
    void OnCollisionStay(Collider *otherCollider) override;

    /**
     * @brief 衝突終了時イベント
     */
    void OnCollisionExit(Collider *otherCollider) override;

    /**
     * @brief ライト・視界マスクのレンダリング
     */
    void RenderLightMask(int rectX, int rectY, int rectW, int rectH, float startDrawX, float startDrawY) const;

    /**
     * @brief 被弾時の血しぶきオーバーレイ描画
     */
    void RenderBloodSplatterOverlay(int screenWidth = 1920, int screenHeight = 1080) const;

    /**
     * @brief ライトがONか確認
     */
    bool IsLightOn() const { return m_isLightOn; }

    /**
     * @brief ライトのON/OFF切り替え
     */
    void ToggleLight() { m_isLightOn = !m_isLightOn; }

    /**
     * @brief ブッシュに隠れているか確認
     */
    bool IsInBush() const { return m_isInBush; }

    /**
     * @brief ブッシュ潜伏状態の設定
     */
    void SetInBush(bool val) { m_isInBush = val; }

    /**
     * @brief エイム向きベクトルの設定
     */
    void SetFacingDir(const Vector2& dir) { facingDir = dir; }

    /**
     * @brief リモートプレイヤーフラグの設定
     */
    void SetRemote(bool val) { isRemote = val; }
    
    /**
     * @brief スキルのセット
     */
    void SetSkill(class Skill* skill) { currentSkill = skill; }
    
    /**
     * @brief 操作入力タイプの設定
     */
    void SetInputType(PlayerInputType type) { m_inputType = type; }

    /**
     * @brief 現在の操作入力タイプの取得
     */
    PlayerInputType GetInputType() const { return m_inputType; }

    /**
     * @brief リモートプレイヤーかどうか
     */
    bool IsRemote() const { return isRemote; }

    /**
     * @brief 視点固定（エイムロック）状態かどうか
     */
    bool IsAimLocked() const { return isAimLocked; }

private:
    float m_lightAngle = 0.0f;  ///< ライト照射角度
    bool m_isLightOn = true;    ///< ライトON/OFF
    bool m_prevMouseRight = false; ///< 前フレーム右クリック状態
    bool isRemote = false;      ///< リモートプレイヤーフラグ
    bool isAimLocked = false;   ///< 視点固定（エイムロック）フラグ
    bool prevAimLockKey = false; ///< 前フレームの視点固定キー状態
    PlayerInputType m_inputType = PlayerInputType::KEYBOARD_MOUSE; ///< 操作タイプ

    float m_maxSpotDistCells = 14.0f; ///< 照射最大距離（セル）
    float m_closeRadiusCells = 2.0f;  ///< 周囲可視半径（セル）
    float m_fanAngleHalf = 0.5236f;   ///< ライト半角

    mutable int m_flickerTimer = 0;          ///< チラつきタイマー
    mutable bool m_isMoonlightFlicker = false; ///< 月光チラつきフラグ
};

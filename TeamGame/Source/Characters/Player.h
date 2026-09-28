#pragma once
#include "Character.h"
#include <vector>
#include <cmath>

/**
 * @brief プレイヤーの入力タイプを表す列挙型
 */
enum class PlayerInputType
{
    KEYBOARD_MOUSE, ///< キーボードとマウスでの操作
    GAMEPAD_1       ///< ゲームパッドでの操作
};

#include "../Weapons/Weapon.h"
class Weapon;

/**
 * @brief プレイヤークラス
 * @details ユーザーが操作するキャラクターを表します。武器の切り替え、移動、スキルの使用などを行います。
 */
class Player : public Character
{
  private:
    class Stage* currentStage = nullptr; ///< 現在のステージのポインタ
    float cellSize = 1.0f;               ///< ステージのセルサイズ
    int damageColorTimer;                ///< ダメージ時の色変化タイマー
    Vector2 facingDir;                   ///< プレイヤーが向いている方向
    std::vector<Weapon *> weapons;       ///< 所持している武器のリスト
    int currentWeaponIndex;              ///< 現在装備している武器のインデックス
    int autoPingTimer;                   ///< オートピンのタイマー
    bool m_isMoving = false;             ///< 現在移動中かどうかのフラグ
    
    // ガジェット（スキル）の保持
    class Skill* currentSkill = nullptr; ///< 現在装備しているスキル（ガジェット）

  public:
    /**
     * @brief プレイヤーの向いている方向を取得する
     * @return 向いている方向のベクトル
     */
    Vector2 GetFacingDir() const
    {
        return facingDir;
    }

    /**
     * @brief X座標を取得する
     * @return プレイヤーのX座標
     */
    float GetX() const { return position.x; }

    /**
     * @brief Y座標を取得する
     * @return プレイヤーのY座標
     */
    float GetY() const { return position.y; }

    /**
     * @brief ライト（向いている方向）の角度を取得する
     * @return ライトの角度（ラジアン）
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
     * @brief ステージ情報を設定する
     * @param s ステージへのポインタ
     * @param cSize セルサイズ
     */
    void SetStage(class Stage* s, float cSize) { currentStage = s; cellSize = cSize; }

    /**
     * @brief ダメージを受ける処理
     */
    void TakeDamage();
    
    /**
     * @brief 弾薬を回復する（現在持っている武器の弾を回復する）
     * @param amount 回復量
     */
    void AddAmmo(int amount);

    
    /**
     * @brief 毎フレームの更新処理
     */
    void Update() override;

    /**
     * @brief 描画処理
     */
    void Draw() override;

    /**
     * @brief UIの描画処理
     * @param screenX 画面上のX座標
     * @param screenY 画面上のY座標
     */
    void DrawUI(int screenX, int screenY);

    /**
     * @brief 衝突開始時の処理
     * @param otherCollider 衝突した相手のコライダー
     */
    void OnCollisionEnter(Collider *otherCollider) override;

    /**
     * @brief 衝突中の処理
     * @param otherCollider 衝突した相手のコライダー
     */
    void OnCollisionStay(Collider *otherCollider) override;

    /**
     * @brief 衝突終了時の処理
     * @param otherCollider 衝突した相手のコライダー
     */
    void OnCollisionExit(Collider *otherCollider) override;

    /**
     * @brief 懐中電灯・スポットライトのマスクを描画する
     * @param rectX 描画領域のX座標
     * @param rectY 描画領域のY座標
     * @param rectW 描画領域の幅
     * @param rectH 描画領域の高さ
     * @param startDrawX 描画開始X座標
     * @param startDrawY 描画開始Y座標
     */
    void RenderLightMask(int rectX, int rectY, int rectW, int rectH, float startDrawX, float startDrawY) const;

    /**
     * @brief 画面に血しぶき（ダメージ演出）のオーバーレイを描画する
     * @param screenWidth 画面の幅
     * @param screenHeight 画面の高さ
     */
    void RenderBloodSplatterOverlay(int screenWidth = 1920, int screenHeight = 1080) const;

    /**
     * @brief ライトがオンになっているか確認する
     * @return オンならtrue
     */
    bool IsLightOn() const { return m_isLightOn; }

    /**
     * @brief ライトのオン/オフを切り替える
     */
    void ToggleLight() { m_isLightOn = !m_isLightOn; }

    /**
     * @brief 草むらに隠れているか確認する
     * @return 隠れていればtrue
     */
    bool IsInBush() const { return m_isInBush; }

    /**
     * @brief 草むらに隠れている状態を設定する
     * @param val 設定値
     */
    void SetInBush(bool val) { m_isInBush = val; }

    /**
     * @brief プレイヤーの向きを設定する
     * @param dir 向く方向のベクトル
     */
    void SetFacingDir(const Vector2& dir) { facingDir = dir; }

    /**
     * @brief リモートプレイヤーかどうかを設定する
     * @param val リモートプレイヤーならtrue
     */
    void SetRemote(bool val) { isRemote = val; }
    
    /**
     * @brief スキルをセットする
     * @param skill セットするスキルのポインタ
     */
    void SetSkill(class Skill* skill) { currentSkill = skill; }
    
    /**
     * @brief 入力タイプを設定する
     * @param type 入力タイプ
     */
    void SetInputType(PlayerInputType type) { m_inputType = type; }

    /**
     * @brief 現在の入力タイプを取得する
     * @return 入力タイプ
     */
    PlayerInputType GetInputType() const { return m_inputType; }

    /**
     * @brief リモートプレイヤーかどうかを取得する
     * @return リモートプレイヤーならtrue
     */
    bool IsRemote() const { return isRemote; }

  private:
    float m_lightAngle = 0.0f;  ///< 向いている角度
    bool m_isLightOn = true;    ///< 懐中電灯スイッチ
    bool m_isInBush = false;    ///< 草むらに隠れているか
    bool m_prevMouseRight = false; ///< 右クリック判定の履歴
    bool isRemote = false;      ///< リモート操作プレイヤーかどうかのフラグ
    PlayerInputType m_inputType = PlayerInputType::KEYBOARD_MOUSE; ///< 操作入力タイプ

    // 懐中電灯・環境光パラメーター
    float m_maxSpotDistCells = 14.0f; ///< スポットライトの最大距離(セル単位)
    float m_closeRadiusCells = 2.0f;  ///< 周囲の近距離視界の半径(セル単位)
    float m_fanAngleHalf = 0.5236f;   ///< ライトの半角

    // 月明かり・チラつきランダム環境光タイマー
    mutable int m_flickerTimer = 0;          ///< ライトのちらつきタイマー
    mutable bool m_isMoonlightFlicker = false; ///< 月明かりのちらつきフラグ
};

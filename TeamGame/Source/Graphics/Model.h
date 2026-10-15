#pragma once
#include <string>
#include <memory>
#include "DxLib.h"
#include "Animation.h"
#include "../Core/Vector2.h"

/**
 * @brief 3Dモデル描画・管理クラス
 * @details MV1モデルの読み込み、座標変換（位置・スケール・回転）、アニメーション管理、描画を一元管理します。
 */
class Model
{
private:
    int m_handle;                               ///< 3Dモデルハンドル
    VECTOR m_position;                          ///< 3Dワールド位置
    VECTOR m_rotation;                          ///< 回転角度(ラジアン)
    VECTOR m_scale;                             ///< 拡大縮小倍率
    float m_facingAngle;                        ///< 照準方向角度 (ラジアン)
    float m_pitchAngle;                         ///< ピッチ角 (ラジアン、デフォルト0.0f)
    float m_tiltAngle;                          ///< 画面手前方向への2.5Dチルト角度 (ラジアン、デフォルト約24度)
    std::unique_ptr<Animation> m_animation;     ///< アニメーション制御コンポーネント

    void SetupMaterials();                      ///< Blenderモデルのマテリアル明度補正
    void UpdateMatrix();                        ///< スケール・向き・2.5Dチルト・位置の行列合成更新

public:
    static constexpr float DEFAULT_SCALE = 0.3f; ///< 3Dモデルの共通標準スケール

    /**
     * @brief ファイルパスから直接ロードするコンストラクタ
     */
    explicit Model(const std::string& path);

    /**
     * @brief 既存のモデルハンドルから複製するコンストラクタ (ResourceManager等向け)
     */
    explicit Model(int rawHandle);

    ~Model();

    // コピー禁止
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    /**
     * @brief モデルが正常にロードされているか
     */
    bool IsLoaded() const { return m_handle != -1; }

    /**
     * @brief モデルハンドルの取得
     */
    int GetHandle() const { return m_handle; }

    /**
     * @brief アニメーション管理インスタンスの取得
     */
    Animation* GetAnimation() const { return m_animation.get(); }

    // --- トランスフォーム設定 ---

    /**
     * @brief 3D座標の設定
     */
    void SetPosition(float x, float y, float z);

    /**
     * @brief 2Dワールド座標からの設定 (Z=0)
     */
    void SetPosition2D(const Vector2& pos, float z = 0.0f);

    /**
     * @brief 2Dスクリーン座標からの設定 (DxLibの3D投影変換に合わせてY反転配置)
     */
    void SetScreenPosition(float screenX, float screenY);

    /**
     * @brief 回転角度の設定 (ラジアン指定)
     */
    void SetRotation(float rx, float ry, float rz);

    /**
     * @brief エイム向きベクトル(Vector2)からモデルのY軸回転角度を自動設定
     */
    void SetFacingDir(const Vector2& dir);

    /**
     * @brief 画面手前方向への2.5Dチルト角度の設定 (ラジアン、デフォルト約24度)
     */
    void SetTiltAngle(float tilt);
    float GetTiltAngle() const { return m_tiltAngle; }

    /**
     * @brief 見下ろし角度（ピッチ角）の設定
     */
    void SetPitchAngle(float pitch);
    float GetPitchAngle() const { return m_pitchAngle; }

    /**
     * @brief 拡大縮小倍率の設定
     */
    void SetScale(float sx, float sy, float sz);

    /**
     * @brief 均等スケール設定
     */
    void SetScaleUniform(float s);

    // --- モーション再生用API (PlayerやEnemyから直感的に呼べる関数群) ---

    /**
     * @brief 待機モーション (Idle) を再生
     */
    void PlayIdle(float speed = 1.0f);

    /**
     * @brief 歩行モーション (Walk) を再生
     */
    void PlayWalk(float speed = 1.0f);

    /**
     * @brief 射撃モーション (Shoot) を再生
     */
    void PlayShoot(float speed = 1.0f);

    /**
     * @brief 死亡・やられモーション (Die) を再生
     */
    void PlayDie(float speed = 1.0f);

    /**
     * @brief 任意のモーションを指定して再生
     */
    void PlayMotion(MotionType motion, bool loop = true, float speed = 1.0f);

    /**
     * @brief 現在再生中のモーション種別を取得
     */
    MotionType GetCurrentMotion() const;

    /**
     * @brief 現在のモーションが終了したか確認
     */
    bool IsMotionFinished() const;

    // --- フレーム更新および描画 ---

    /**
     * @brief 毎フレームのアニメーション更新
     */
    void Update(float deltaTime = 1.0f);

    /**
     * @brief 3Dモデルを描画
     */
    void Draw() const;
};

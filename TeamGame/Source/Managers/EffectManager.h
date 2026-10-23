#pragma once
#include <vector>
#include <cmath>

/**
 * @brief 単一エフェクトを管理する基底クラス (抽象クラス)
 * @details 全てのエフェクトはこれを継承してUpdateとDrawを実装する。
 */
class Effect
{
  public:
    float x, y;
    int lifeTimer;
    int maxLife;

    /**
     * @brief コンストラクタ
     * @param x X座標
     * @param y Y座標
     * @param life 寿命フレーム数
     */
    Effect(float x, float y, int life)
        : x(x), y(y), lifeTimer(life), maxLife(life)
    {
    }
    
    /**
     * @brief デストラクタ
     */
    virtual ~Effect()
    {
    }
    
    /**
     * @brief 更新処理
     * @details 毎フレーム呼ばれ、寿命タイマーを減算する。
     */
    virtual void Update()
    {
        if (lifeTimer > 0)
            lifeTimer--;
    }
    
    /**
     * @brief 描画処理
     * @details 派生クラスで実装する。
     */
    virtual void Draw() = 0;
    
    /**
     * @brief エフェクトが終了したかどうかの判定
     * @return bool 終了している場合はtrue
     */
    bool IsDead() const
    {
        return lifeTimer <= 0;
    }

    virtual bool IsGroundLayer() const
    {
        return false;
    }
};

/**
 * @brief 出血パーティクルエフェクト
 */
class BloodParticle : public Effect
{
  public:
    float vx, vy;
    float size;
    unsigned int color;

    /**
     * @brief コンストラクタ
     * @param startX 初期X座標
     * @param startY 初期Y座標
     * @param velX X方向速度
     * @param velY Y方向速度
     * @param pSize パーティクルのサイズ
     * @param life 寿命フレーム数
     * @param col 色
     */
    BloodParticle(float startX, float startY, float velX, float velY, float pSize, int life, unsigned int col)
        : Effect(startX, startY, life), vx(velX), vy(velY), size(pSize), color(col)
    {
    }

    /**
     * @brief 更新処理
     */
    void Update() override;
    
    /**
     * @brief 描画処理
     */
    void Draw() override;
};

/**
 * @brief ナイフの斬撃エフェクト
 */
class KnifeSlashEffect : public Effect
{
  public:
    float angle;
    
    /**
     * @brief コンストラクタ
     * @param startX X座標
     * @param startY Y座標
     * @param dirAngle 斬撃の角度
     */
    KnifeSlashEffect(float startX, float startY, float dirAngle)
        : Effect(startX, startY, 12), angle(dirAngle)
    {
    }

    /**
     * @brief 描画処理
     */
    void Draw() override;
};

/**
 * @brief マズルフラッシュエフェクト（銃口の発火）
 */
class MuzzleFlashEffect : public Effect
{
  public:
    float angle;
    float flashRadius;

    /**
     * @brief コンストラクタ
     * @param startX X座標
     * @param startY Y座標
     * @param dirAngle 角度
     * @param radius フラッシュの半径
     */
    MuzzleFlashEffect(float startX, float startY, float dirAngle, float radius = 18.0f)
        : Effect(startX, startY, 6), angle(dirAngle), flashRadius(radius)
    {
    }

    /**
     * @brief 描画処理
     */
    void Draw() override;
};

/**
 * @brief 火花パーティクルエフェクト
 */
class SparkParticle : public Effect
{
  public:
    float vx, vy;
    float size;
    unsigned int color;

    /**
     * @brief コンストラクタ
     * @param startX 初期X座標
     * @param startY 初期Y座標
     * @param velX X方向速度
     * @param velY Y方向速度
     * @param pSize パーティクルのサイズ
     * @param life 寿命フレーム数
     * @param col 色
     */
    SparkParticle(float startX, float startY, float velX, float velY, float pSize, int life, unsigned int col)
        : Effect(startX, startY, life), vx(velX), vy(velY), size(pSize), color(col)
    {
    }

    /**
     * @brief 更新処理
     */
    void Update() override;
    
    /**
     * @brief 描画処理
     */
    void Draw() override;
};

/**
 * @brief 風のスジ・突風気流パーティクル
 */
class WindGustParticle : public Effect
{
  public:
    float vx, vy;
    float length;
    float thickness;
    float angle;
    unsigned int color;

    WindGustParticle(float startX, float startY, float velX, float velY, float len, float thick, int life, unsigned int col)
        : Effect(startX, startY, life), vx(velX), vy(velY), length(len), thickness(thick), color(col)
    {
        angle = static_cast<float>(std::atan2(vy, vx));
    }

    void Update() override;
    void Draw() override;
};

/**
 * @brief 突風・風の衝撃波エフェクト（Clean time用）
 */
class WindShockwaveEffect : public Effect
{
  public:
    float maxRadius;
    unsigned int color;
    float startAngle;

    WindShockwaveEffect(float startX, float startY, float maxR = 300.0f, int life = 24, unsigned int col = 0)
        : Effect(startX, startY, life), maxRadius(maxR), color(col), startAngle(0.0f)
    {
    }

    void Update() override;
    void Draw() override;
};

/**
 * @brief ソナー波（音波ピン探知）エフェクト
 */
class SonarWaveEffect : public Effect
{
  public:
    float maxRadius;
    unsigned int color;

    SonarWaveEffect(float startX, float startY, float maxR = 350.0f, int life = 35, unsigned int col = 0)
        : Effect(startX, startY, life), maxRadius(maxR), color(col)
    {
    }

    void Draw() override;
};

/**
 * @brief 全てのエフェクトを管理するクラス
 * @details パーティクルや斬撃などのエフェクトを生成・更新・描画・破棄する機能を提供する。
 */
/**
 * @brief 肉片・骨片・残虐ゴア破片パーティクル
 */
class FleshGoreParticle : public Effect
{
  public:
    float vx, vy;
    float size;
    float rotAngle;
    float rotSpeed;
    unsigned int color;

    FleshGoreParticle(float startX, float startY, float velX, float velY, float pSize, int life, unsigned int col)
        : Effect(startX, startY, life), vx(velX), vy(velY), size(pSize), rotAngle(0.0f), color(col)
    {
        rotSpeed = ((rand() % 100) / 100.0f - 0.5f) * 0.4f;
    }

    void Update() override;
    void Draw() override;
};

/**
 * @brief 地面にこびり付く残虐な血糊・スプラッター痕エフェクト（長時間残留）
 */
class BloodStainEffect : public Effect
{
  public:
    float baseRadius;
    struct Drop {
        float ox, oy;
        float r;
    };
    std::vector<Drop> drops;
    unsigned int color;

    BloodStainEffect(float startX, float startY, float radius = 35.0f, int life = 300);

    bool IsGroundLayer() const override { return true; }

    void Update() override;
    void Draw() override;
};

class EffectManager
{
  private:
    std::vector<Effect *> effects;
    int gunFlashTimer = 0;
    float lastFlashWorldX = 0.0f;
    float lastFlashWorldY = 0.0f;

  public:
    /**
     * @brief コンストラクタ
     */
    EffectManager();
    
    /**
     * @brief デストラクタ
     */
    ~EffectManager();

    /**
     * @brief 汎用的なエフェクトの追加
     * @param effect 追加するエフェクトのポインタ
     */
    void AddEffect(Effect *effect);
    
    /**
     * @brief 出血エフェクトの追加
     * @param worldX ワールドX座標
     * @param worldY ワールドY座標
     * @param count 発生させるパーティクル数 (デフォルト: 10)
     */
    void AddBloodEffect(float worldX, float worldY, int count = 10);
    
    /**
     * @brief ナイフ斬撃エフェクトの追加
     * @param worldX ワールドX座標
     * @param worldY ワールドY座標
     * @param dirAngle 斬撃の角度
     */
    void AddKnifeSlashEffect(float worldX, float worldY, float dirAngle);
    
    /**
     * @brief マズルフラッシュエフェクトの追加
     * @param worldX ワールドX座標
     * @param worldY ワールドY座標
     * @param dirAngle 角度
     * @param radius フラッシュの半径 (デフォルト: 18.0f)
     */
    void AddMuzzleFlashEffect(float worldX, float worldY, float dirAngle, float radius = 18.0f);
    
    /**
     * @brief ソナー音波探知エフェクトの追加
     * @param worldX ワールドX座標
     * @param worldY ワールドY座標
     * @param color 波の色
     */
    void AddSonarWaveEffect(float worldX, float worldY, unsigned int color = 0);

    /**
     * @brief 風の衝撃波・突風エフェクトの追加（主婦スキル Clean time 用）
     * @param worldX 中心X座標
     * @param worldY 中心Y座標
     * @param maxRadius 最大半径
     */
    void AddWindShockwaveEffect(float worldX, float worldY, float maxRadius = 300.0f);
    /**
     * @brief 指揮者用 残虐自爆・肉片飛散ゴアエフェクト
     */
    void AddCruelExplosionEffect(float worldX, float worldY, float blastRadius = 90.0f);

    
    /**
     * @brief 画面全体のガンフラッシュ効果のトリガー
     * @param worldX 発火源のワールドX座標
     * @param worldY 発火源のワールドY座標
     * @param duration フラッシュの持続フレーム数 (デフォルト: 5)
     */
    void TriggerGunFlash(float worldX, float worldY, int duration = 5)
    {
        gunFlashTimer = duration;
        lastFlashWorldX = worldX;
        lastFlashWorldY = worldY;
    }
    
    /**
     * @brief ガンフラッシュタイマーの取得
     */
    int GetGunFlashTimer() const { return gunFlashTimer; }
    
    /**
     * @brief 最新のフラッシュ発生位置のX座標を取得
     */
    float GetLastFlashWorldX() const { return lastFlashWorldX; }
    
    /**
     * @brief 最新のフラッシュ発生位置のY座標を取得
     */
    float GetLastFlashWorldY() const { return lastFlashWorldY; }
    
    /**
     * @brief エフェクトの更新
     * @details 各エフェクトの更新処理を呼び出し、寿命が尽きたものを削除する。
     */
    void Update();
    
    /**
     * @brief エフェクトの描画
     * @details 画面全体のフラッシュ効果と、各エフェクトの描画処理を呼び出す。
     */
    void Draw();
    void DrawGround();
    void DrawForeground();
    
    /**
     * @brief 全てのエフェクトをクリア
     * @details 保持している全てのエフェクトをメモリから解放する。
     */
    void Clear();
};

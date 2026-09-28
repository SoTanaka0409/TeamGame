#pragma once
#include "Character.h"
#include "PathfindingComponent.h"
#include "Vector2.h"

/**
 * @brief 敵のAIステートを表す列挙型
 */
enum class EnemyAIState
{
    PATROL,     ///< 巡回状態
    ALERT,      ///< 警戒状態
    INVESTIGATE ///< 調査状態
};

/**
 * @brief 敵キャラクターのクラス
 * @details プレイヤーを追跡、攻撃、巡回するAIを持ちます。
 */
class Enemy : public Character
{
private:
    PathfindingComponent pathfinder; ///< 経路探索用コンポーネント

  private:
    int damageColorTimer;             ///< ダメージ時の色変化タイマー
    class Stage *currentStage;        ///< 現在のステージのポインタ
    float cellSize;                   ///< ステージのセルサイズ
    class Character *targetCharacter; ///< ターゲットとなるキャラクターのポインタ

    EnemyAIState aiState;             ///< 現在のAIステート
    Vector2 facingDir;                ///< 向いている方向
    Vector2 moveDir;                  ///< 移動方向
    Vector2 lastKnownPos;             ///< 最後に確認したターゲットの位置
    int patrolChangeTimer;            ///< 巡回方向を変更するタイマー
    int investigateTimer;             ///< 調査状態のタイマー
    int autoPingTimer;                ///< オートピンタイマー
    float currentInvestigateVolume;   ///< 調査対象の音量
    int shootCooldown;                ///< 射撃クールダウン
    int strafeDirection;              ///< 横移動の方向
    int strafeTimer;                  ///< 横移動のタイマー

  public:
    /**
     * @brief コンストラクタ
     * @param startX 初期X座標
     * @param startY 初期Y座標
     * @param tId チームID (デフォルトは1)
     */
    Enemy(float startX, float startY, int tId = 1);

    /**
     * @brief デストラクタ
     */
    ~Enemy();

    /**
     * @brief 毎フレームの更新処理
     */
    void Update() override;

    /**
     * @brief 描画処理
     */
    void Draw() override;

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
     * @brief ステージ情報を設定する
     * @param s ステージへのポインタ
     * @param cSize セルサイズ
     */
    void SetStage(class Stage *s, float cSize)
    {
        currentStage = s;
        cellSize = cSize;
    }
    
    /**
     * @brief 最も近い敵（ターゲット）を更新するロジック
     */
    void UpdateTarget();

    /**
     * @brief 銃声などの音を聞いた時の処理
     * @param soundPos 音の発生位置
     * @param maxDistance 音が聞こえる最大距離
     */
    void OnHearGunshot(const Vector2 &soundPos, float maxDistance);

    /**
     * @brief ダメージを受けた時の処理
     */
    void Damage();

    /**
     * @brief ステルスキルを受けた時の処理
     */
    void StealthKill();

    /**
     * @brief 現在警戒状態かどうかを確認する
     * @return 警戒状態ならtrue、それ以外ならfalse
     */
    bool IsAlerted() const { return aiState == EnemyAIState::ALERT; }

    /**
     * @brief 現在のAIステートを取得する
     * @return 現在のAIステート
     */
    EnemyAIState GetAIState() const { return aiState; }

    /**
     * @brief ターゲットとの間に視線が通っているか確認する
     * @param target 対象のキャラクター
     * @return 視線が通っていればtrue、障害物があればfalse
     */
    bool CheckLineOfSightToTarget(Character* target) const;

    /**
     * @brief 賢く移動する処理（障害物回避など）
     * @param desiredDir 希望する移動方向
     */
    void MoveSmart(const Vector2 &desiredDir);
};

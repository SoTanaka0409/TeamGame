#pragma once
#include "Character.h"
#include "PathfindingComponent.h"
#include "Vector2.h"

/**
 * @brief 敵AIの行動ステートを表す列挙型
 */
enum class EnemyAIState
{
    PATROL,     ///< 巡回状態（開始・リスポーン直後は中央へ巡界移動）
    ALERT,      ///< 警戒・戦闘状態（ターゲット追跡および射撃）
    INVESTIGATE ///< 調査状態（銃声や最終確認位置へ移動）
};

/**
 * @brief 敵・味方ボットキャラクターの制御クラス
 * @details 索敵、A*経路案内、チーム識別付き攻撃、銃声反応を管理します。
 */
class Enemy : public Character
{
private:
    PathfindingComponent pathfinder; ///< A*経路探索コンポーネント
    int moveToCenterTimer;           ///< 中央移動タイマー（5秒＝300フレーム）

private:
    int damageColorTimer;             ///< 被弾時イエロー点滅タイマー
    class Stage *currentStage;        ///< ステージポインタ
    float cellSize;                   ///< 1マスのサイズ
    class Character *targetCharacter; ///< 現在ロックオン中のターゲット

    EnemyAIState aiState;             ///< 現在のAIステート
    Vector2 facingDir;                ///< 現在向いている方向
    Vector2 moveDir;                  ///< 移動ベクトル
    Vector2 lastKnownPos;             ///< 最後に確認した音・敵の位置
    int patrolChangeTimer;            ///< 巡回方向切り替えタイマー
    int investigateTimer;             ///< 調査状態の滞在タイマー
    int shootCooldown;                ///< 射撃クールダウン
    int aimDelayTimer;                ///< エイムディレイタイマー
    int strafeDirection;              ///< かに歩き（横移動）方向
    int strafeTimer;                  ///< かに歩き切り替えタイマー
    float effectiveRangeCells = 10.0f;///< 攻撃有効射程（10セル）
    float sightRangeCells = 14.0f;    ///< 索敵視界（14セル: プレイヤーのライト照射範囲14セルに統一）

public:
    /**
     * @brief コンストラクタ
     * @param startX 初期X座標
     * @param startY 初期Y座標
     * @param tId チームID (0:味方ボット, 1:敵ボット)
     */
    Enemy(float startX, float startY, int tId = 1);

    /**
     * @brief デストラクタ
     */
    ~Enemy();

    /**
     * @brief フレーム更新処理（AI意思決定・移動・攻撃）
     */
    void Update() override;

    /**
     * @brief 描画処理（カメラ座標変換適用・チーム別カラー表示）
     */
    void Draw() override;

    /**
     * @brief 衝突開始イベントハンドラ
     */
    void OnCollisionEnter(Collider *otherCollider) override;

    /**
     * @brief 衝突継続イベントハンドラ
     */
    void OnCollisionStay(Collider *otherCollider) override;

    /**
     * @brief 衝突終了イベントハンドラ
     */
    void OnCollisionExit(Collider *otherCollider) override;

    /**
     * @brief ステージ情報のセットアップ
     * @param s ステージへのポインタ
     * @param cSize セルサイズ
     */
    void SetStage(class Stage *s, float cSize)
    {
        currentStage = s;
        cellSize = cSize;
    }
    
    /**
     * @brief 視界内の最も近い別チームキャラクターを索敵・ターゲットロックする
     */
    void UpdateTarget();

    /**
     * @brief 攻撃有効射程（ワールド距離）の取得
     */
    float GetEffectiveRange() const { return effectiveRangeCells * cellSize; }

    /**
     * @brief 攻撃有効射程（セル数）の取得
     */
    float GetEffectiveRangeCells() const { return effectiveRangeCells; }

    /**
     * @brief 索敵視界距離（ワールド距離）の取得
     */
    float GetSightRange() const { return sightRangeCells * cellSize; }

    /**
     * @brief 攻撃有効射程（セル数）の設定
     */
    void SetEffectiveRangeCells(float cells) { effectiveRangeCells = cells; }

    /**
     * @brief 銃声を感知した際の処理（敵チームの銃声のみ反応）
     * @param soundPos 銃声の発生ワールド位置
     * @param loudness 到達音量/距離
     * @param shooterTeamId 発砲者のチームID
     */
    void OnHearGunshot(const Vector2 &soundPos, float loudness = 1.0f, int shooterTeamId = -1);

    /**
     * @brief ダメージ受信処理
     */
    void Damage();

    /**
     * @brief ステルスキル受信処理
     */
    void StealthKill();

    /**
     * @brief 警戒・戦闘状態かどうか
     */
    bool IsAlerted() const { return aiState == EnemyAIState::ALERT; }

    /**
     * @brief 現在のAIステート取得
     */
    EnemyAIState GetAIState() const { return aiState; }

    /**
     * @brief ターゲットキャラクターの明示的設定
     */
    void SetTargetCharacter(class Character *c) { targetCharacter = c; }
    void SetTargetPlayer(class Character *p) { targetCharacter = p; }

    /**
     * @brief 中央移動タイマーを5秒（300フレーム）にリセット
     */
    void ResetMoveToCenter() { moveToCenterTimer = 300; }

    /**
     * @brief 指定ターゲットへの視線（Line of Sight）遮蔽チェック
     * @param target 対象キャラクター（Nullの場合は現在のターゲット）
     * @return 視線が通っていればtrue
     */
    bool CheckLineOfSightToTarget(Character* target = nullptr) const;

    /**
     * @brief 視認プレイヤーへの視線チェック
     */
    bool CheckLineOfSightToPlayer() const { return CheckLineOfSightToTarget(); }

    /**
     * @brief 障害物壁滑り付き移動処理
     * @param desiredDir 移動希望ベクトル
     */
    void MoveSmart(const Vector2 &desiredDir);
};

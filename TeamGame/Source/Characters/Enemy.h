#pragma once
#include "Character.h"
#include "PathfindingComponent.h"
#include "Vector2.h"

/**
 * @brief 敵AIの行動ステートを表す列挙型
 */
enum class EnemyAIState
{
    PATROL,     ///< 巡回状態（開始・リスポーン直後は中央へ巡回移動）
    ALERT,      ///< 警戒・戦闘状態（ターゲット追跡および射撃）
    INVESTIGATE,///< 調査状態（銃声や最終確認位置へ移動）
    RETREAT     ///< 撤退・遮蔽退避状態（低HP時やデバフ時に最寄りの壁裏へ避難）
};

/**
 * @brief Botキャラクターの性格・思考パラメータ
 */
struct BotPersonality
{
    float retreatHpRatio = 0.35f;      ///< 撤退開始HP割合（0.0fなら死ぬまで撤退しない）
    float aggression = 1.0f;           ///< 突進・接近度合 (0.5:引き気味, 2.0:猪突猛進)
    float teamworkDistance = 0.0f;     ///< 味方との理想距離（0なら単独行動、300なら味方随伴）
    bool focusLowHpTarget = false;     ///< 瀕死の敵（HP最小）を優先狙撃するか（悪魔）
    bool preferBush = false;           ///< 草むら（ブッシュ）潜伏志向（反逆者）
    bool defensiveChokePoint = false;  ///< チョークポイント待ち伏せ・迎撃志向（主婦）
};

/**
 * @brief 武器ごとの交戦間合い・移動戦術
 */
struct WeaponAITactic
{
    float optimalRangeMin = 250.0f;    ///< 最適交戦距離(下限)
    float optimalRangeMax = 450.0f;    ///< 最適交戦距離(上限)
    float disengageDistance = 150.0f;  ///< 接近されすぎた時の後退開始距離
    bool allowRetreatKiting = false;   ///< 近距離で敵と逆方向へ後退（引き撃ち・カイト）するか（スナイパー）
    bool circleStrafe = false;         ///< 敵の周囲を旋回・回り込み移動するか（SMG）
    bool recklessRush = false;         ///< 最短距離でゼロ距離接射へ肉薄するか（ショットガン）
};

/**
 * @brief 敵・味方ボットキャラクターの制御クラス
 * @details 索敵、A*経路案内、チーム識別付き攻撃、銃声反応を管理します。
 */
class Enemy : public Character
{
private:
    PathfindingComponent pathfinder; ///< A*経路探索コンポーネント
    int moveToCenterTimer;           ///< 散開移動タイマー（10秒＝600フレーム。旧: 中央集合タイマー）
    Vector2 dispersalTarget;         ///< 散開先ウェイポイント（マップ全域に分散）
    bool hasDispersalTarget = false; ///< 散開先が決定済みか
    float baseMoveSpeed = 0.0f;      ///< 初期移動速度（鈍足デバフ判定の基準）

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
    float sightRangeCells = 8.5f;     ///< 索敵視界（8.5セル: プレイヤーのカメラ画面サイズ・ライト照射範囲に合わせて同期）
    class Weapon* equippedWeapon = nullptr; ///< 装備武器
    bool wasPanic = false;                  ///< 直前フレームでパニック状態だったかどうかのフラグ
    int m_characterId = 1;                  ///< キャラクターID (1:指揮者, 2:反逆者, 3:バカ, 4:主婦, 5:悪魔)
    BotPersonality personality;             ///< キャラクター固有の性格・思考ルーチン
    WeaponAITactic weaponTactic;             ///< 装備武器に基づく交戦間合い・射撃戦術
    int retreatTimer = 0;                   ///< 撤退行動継続タイマー
    Vector2 coverTargetPos;                 ///< 目指す遮蔽ポイント

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
     * @brief キャラクターIDの設定（性格パラメータを自動初期化）
     */
    void SetCharacterId(int id);
    int GetCharacterId() const { return m_characterId; }

    /**
     * @brief キャラクター性格および武器戦術の更新・取得
     */
    void UpdatePersonality();
    void UpdateWeaponTactic();
    const BotPersonality& GetPersonality() const { return personality; }
    void SetPersonality(const BotPersonality& p) { personality = p; }
    const WeaponAITactic& GetWeaponTactic() const { return weaponTactic; }
    void SetWeaponTactic(const WeaponAITactic& t) { weaponTactic = t; }

    /**
     * @brief 敵から射線を切れる最寄りの壁裏（遮蔽物）を検索
     * @param threatPos 脅威となる敵の位置
     * @return 遮蔽位置（見つからない場合は現在位置）
     */
    Vector2 FindCoverPosition(const Vector2& threatPos);

    /**
     * @brief 装備武器の設定
     */
    void SetEquippedWeapon(class Weapon* w);

    /**
     * @brief 装備武器の取得
     */
    class Weapon* GetEquippedWeapon() const { return equippedWeapon; }

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
    float GetEffectiveRange() const;

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
    void Damage(int amount = 1, const std::string& attackerName = "", int attackerTeamId = -1);

    /**
     * @brief ステルスキル受信処理
     */
    void StealthKill(const std::string& attackerName = "", int attackerTeamId = -1);

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
     * @brief 散開タイマーを10秒（600フレーム）にリセットし、次回マップ全域のランダム地点を散開先に再選定する
     * @details 以前は全ボットが中央へ集合していたが、戦略性向上のためマップ全域へ散開する仕様に変更。
     */
    void ResetMoveToCenter() { moveToCenterTimer = 600; hasDispersalTarget = false; }

    /**
     * @brief マップ全域から散開先ウェイポイントをランダムに選定する（区画ローテーション＋草むら優先）
     */
    void PickDispersalTarget();

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
     * @brief 壁・他キャラを避けながら移動する
     * @param desiredDir 希望移動方向
     * @param speedScale 移動速度倍率（戦闘中のカニ歩きなどで減速させる用途）
     */
    void MoveSmart(const Vector2 &desiredDir, float speedScale = 1.0f);

    /**
     * @brief HomingKillスキルによる特攻状態を開始
     */
    void StartHomingSuicide(Character* enemyTarget, float speedMultiplier = 1.6f);

    /**
     * @brief 特攻状態を解除・リセットする
     */
    void StopHomingSuicide();

    /**
     * @brief 特攻状態中かどうか
     */
    bool IsHomingSuicide() const { return isHomingSuicide; }

private:
    bool isHomingSuicide = false;
    int suicideTimer = 0;
};

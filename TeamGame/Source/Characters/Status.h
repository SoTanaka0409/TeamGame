#pragma once

/**
 * @brief キャラクターのステータス（体力、速度、攻撃力など）を管理するクラス
 * @details 基本値に加え、アイテムやスキルによるボーナス値を考慮して最終的なステータスを計算します。
 */
class Status
{
private:
    // --- 体力関連 ---
    int baseMaxHp;          ///< 基本の最大体力
    int itemMaxHpBonus;     ///< アイテムによる最大体力ボーナス
    int skillMaxHpBonus;    ///< スキルによる最大体力ボーナス
    int currentHp;          ///< 現在の体力

    // --- 速度関連 ---
    float baseSpeed;        ///< 基本の移動速度
    float itemSpeedBonus;   ///< アイテムによる移動速度ボーナス
    float skillSpeedBonus;  ///< スキルによる移動速度ボーナス
    float speedMultiplier;  ///< 最終的な速度の乗数

    // --- 攻撃力関連 ---
    int baseAttack;         ///< 基本の攻撃力
    int itemAttackBonus;    ///< アイテムによる攻撃力ボーナス
    int skillAttackBonus;   ///< スキルによる攻撃力ボーナス
    float attackMultiplier; ///< 最終的な攻撃力の乗数

public:
    /**
     * @brief コンストラクタ
     */
    Status();

    /**
     * @brief デストラクタ
     */
    ~Status() = default;

    /**
     * @brief ステータスを初期化する
     * @param hp 初期最大体力
     * @param speed 初期移動速度
     * @param attack 初期攻撃力
     */
    void Init(int hp, float speed, int attack);

    /**
     * @brief 現在の最大体力を取得する（基本値＋ボーナス）
     * @return 最終的な最大体力
     */
    int GetMaxHp() const;

    /**
     * @brief 現在の体力を取得する
     * @return 現在の体力
     */
    int GetCurrentHp() const;

    /**
     * @brief 体力を回復する
     * @param amount 回復量
     * @details 最大体力を超えて回復することはありません。
     */
    void Heal(int amount);

    /**
     * @brief ダメージを受ける
     * @param amount ダメージ量
     * @details 体力が0未満になることはありません。
     */
    void TakeDamage(int amount);

    /**
     * @brief 死亡しているかどうかを判定する
     * @return 体力が0以下ならtrue、それ以外ならfalse
     */
    bool IsDead() const;
    
    /**
     * @brief アイテムによる最大体力ボーナスを設定する
     * @param bonus 加算するボーナス値
     */
    void SetItemMaxHpBonus(int bonus);

    /**
     * @brief スキルによる最大体力ボーナスを設定する
     * @param bonus 加算するボーナス値
     */
    void SetSkillMaxHpBonus(int bonus);

    /**
     * @brief 現在の移動速度を取得する（基本値＋ボーナス）×乗数
     * @return 最終的な移動速度
     */
    float GetSpeed() const;

    /**
     * @brief 基本の移動速度を設定する
     * @param speed 基本の移動速度
     */
    void SetBaseSpeed(float speed);

    /**
     * @brief アイテムによる移動速度ボーナスを設定する
     * @param bonus 加算するボーナス値
     */
    void SetItemSpeedBonus(float bonus);

    /**
     * @brief スキルによる移動速度ボーナスを設定する
     * @param bonus 加算するボーナス値
     */
    void SetSkillSpeedBonus(float bonus);

    /**
     * @brief 速度の乗数を設定する
     * @param multiplier 乗数値
     */
    void SetSpeedMultiplier(float multiplier);

    /**
     * @brief 現在の攻撃力を取得する（基本値＋ボーナス）×乗数
     * @return 最終的な攻撃力
     */
    int GetAttack() const;

    /**
     * @brief 基本の攻撃力を設定する
     * @param attack 基本の攻撃力
     */
    void SetBaseAttack(int attack);

    /**
     * @brief アイテムによる攻撃力ボーナスを設定する
     * @param bonus 加算するボーナス値
     */
    void SetItemAttackBonus(int bonus);

    /**
     * @brief スキルによる攻撃力ボーナスを設定する
     * @param bonus 加算するボーナス値
     */
    void SetSkillAttackBonus(int bonus);

    /**
     * @brief 攻撃力の乗数を設定する
     * @param multiplier 乗数値
     */
    void SetAttackMultiplier(float multiplier);
};

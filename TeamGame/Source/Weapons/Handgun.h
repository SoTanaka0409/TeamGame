#pragma once
#include "Weapon.h"

/**
 * @brief ハンドガンクラス
 * @details 単発で弾を発射する標準的な武器
 */
class Handgun : public Weapon
{
  public:
    /**
     * @brief コンストラクタ
     * @details ハンドガンのデータを初期化する
     */
    Handgun();
    /**
     * @brief 弾を発射する
     * @param pos 発射位置
     * @param dir 発射方向
     * @param teamId チームID
     * @param additionalSpread 追加の拡散角度
     * @details 弾を1発生成し、指定方向に発射する
     */
    void Fire(const Vector2 &pos, const Vector2 &dir, int teamId, float additionalSpread) override;
};

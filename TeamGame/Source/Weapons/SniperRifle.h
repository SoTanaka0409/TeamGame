#pragma once
#include "Weapon.h"

/**
 * @brief スナイパーライフルクラス
 * @details 射程が長く高威力だが、移動時のブレが大きい武器
 */
class SniperRifle : public Weapon
{
  public:
    /**
     * @brief コンストラクタ
     * @details スナイパーライフルのデータを初期化する
     */
    SniperRifle();
    /**
     * @brief 弾を発射する
     * @param pos 発射位置
     * @param dir 発射方向
     * @param teamId チームID
     * @param additionalSpread 追加の拡散角度
     * @details 弾を1発生成し、高速で発射する
     */
    void Fire(const Vector2 &pos, const Vector2 &dir, int teamId, float additionalSpread) override;
    /**
     * @brief 移動時のブレペナルティを取得する
     * @return ブレの大きさ
     * @details スナイパー特有の大きなブレペナルティを返す
     */
    float GetMoveSpreadPenalty() const override { return 30.0f; } // 大きなブレペナルティ
};

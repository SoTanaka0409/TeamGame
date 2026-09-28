#pragma once
#include "Weapon.h"

/**
 * @brief ショットガンクラス
 * @details 複数の弾を扇状に同時発射する近接向け武器
 */
class Shotgun : public Weapon
{
  public:
    /**
     * @brief コンストラクタ
     * @details ショットガンのデータを初期化する
     */
    Shotgun();
    /**
     * @brief 弾を発射する
     * @param pos 発射位置
     * @param dir 発射方向
     * @param teamId チームID
     * @param additionalSpread 追加の拡散角度
     * @details 複数の弾を扇状に生成し発射する
     */
    void Fire(const Vector2 &pos, const Vector2 &dir, int teamId, float additionalSpread) override;
};

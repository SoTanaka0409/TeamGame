#pragma once
#include "CircleCollider.h"
#include "Object2D.h"
#include "Vector2.h"

class ColliderManager;

/**
 * @brief 敵の弾クラス
 * @details プレイヤーに向かって飛んでくる敵専用の弾オブジェクト
 */
class EnemyBullet : public Object2D
{
  private:
    CircleCollider *collider;
    ColliderManager *myColliderManager;
    Vector2 velocity;
    float radius;
    float maxRange;
    Vector2 startPos;

  public:
    /**
     * @brief コンストラクタ
     * @param startX 初期位置X
     * @param startY 初期位置Y
     * @param dir 進行方向
     * @param speed 弾の速度
     * @param range 最大射程距離（デフォルト1500）
     * @details 弾の初期パラメータを設定し、コライダーを登録する
     */
    EnemyBullet(float startX, float startY, const Vector2 &dir, float speed, float range = 1500.0f);
    virtual ~EnemyBullet();

    /**
     * @brief 毎フレームの更新処理
     * @details 移動処理、射程や画面外のチェック、壁との衝突判定を行う
     */
    void Update() override;
    /**
     * @brief 描画処理
     * @details 敵の弾の見た目を赤っぽく描画する
     */
    void Draw() override;

    /**
     * @brief 衝突時の処理
     * @param otherCollider 衝突した相手のコライダー
     * @details プレイヤーに当たった場合ダメージを与え、自身を消滅させる
     */
    void OnCollisionEnter(Collider *otherCollider) override;
};

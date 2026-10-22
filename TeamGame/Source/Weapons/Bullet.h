#pragma once
#include "CircleCollider.h"
#include "Object2D.h"
#include "Vector2.h"
#include "BulletVisualConfig.h"

class ColliderManager;

/**
 * @brief プレイヤーや味方の弾クラス
 * @details まっすぐ飛んで敵にダメージを与える弾オブジェクト。弾丸の見た目（コア色、グロー、トレール）の変更に対応。
 */
class Bullet : public Object2D
{
  private:
    CircleCollider *collider;
    ColliderManager *myColliderManager;
    Vector2 velocity;
    Vector2 startPos;
    float radius;
    float maxRange;
    int teamId;
    int damage;
    std::string shooterName;
    bool isPanicBullet;
    BulletVisualConfig visualConfig; ///< 弾丸の描画アセット・演出設定

  public:
    /**
     * @brief コンストラクタ
     * @param startX 初期位置X
     * @param startY 初期位置Y
     * @param dir 進行方向
     * @param speed 弾の速度
     * @param range 最大射程距離
     * @param radius 弾の当たり判定の半径
     * @param tId チームID（デフォルト0）
     * @param dmg ダメージ量（デフォルト1）
     * @param sName 射撃者名
     * @param isPanic 錯乱状態フラグ
     * @param visual 弾丸の描画アセット設定（省略時はデフォルト）
     * @details 弾の初期パラメータを設定し、コライダーを登録する
     */
    Bullet(float startX, float startY, const Vector2 &dir, float speed, float range, float radius,
           int tId = 0, int dmg = 1, const std::string& sName = "", bool isPanic = false,
           const BulletVisualConfig& visual = BulletVisualConfig());
    virtual ~Bullet();

    /**
     * @brief 毎フレームの更新処理
     * @details 移動処理、射程や画面外のチェック、壁との衝突判定を行う
     */
    void Update() override;
    /**
     * @brief 描画処理
     * @details 弾の見た目（コア、グロー、トレール）を描画する
     */
    void Draw() override;

    /**
     * @brief 衝突時の処理
     * @param otherCollider 衝突した相手のコライダー
     * @details 敵やプレイヤーに当たった場合ダメージを与え、自身を消滅させる
     */
    void OnCollisionEnter(Collider *otherCollider) override;

    const BulletVisualConfig& GetVisualConfig() const { return visualConfig; }
    void SetVisualConfig(const BulletVisualConfig& config) { visualConfig = config; }
};

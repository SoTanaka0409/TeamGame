#pragma once
#include "Collider.h"
#include <set>
#include <utility>
#include <vector>

/**
 * @brief 当たり判定を管理するクラス
 * @details ゲーム内のすべてのコライダーを管理し、衝突判定とコールバックの処理を行う。
 */
class ColliderManager
{
  private:
    std::vector<Collider *> colliders;
    std::set<std::pair<Collider *, Collider *>> previousCollisions;

  public:
    /**
     * @brief コンストラクタ
     * @details ColliderManagerのインスタンスを初期化する。
     */
    ColliderManager();

    /**
     * @brief デストラクタ
     * @details メモリリソースをクリーンアップする。
     */
    ~ColliderManager();

    /**
     * @brief コライダーの追加
     * @param collider 追加するコライダーのポインタ
     * @details 管理リストに新しいコライダーを追加する。
     */
    void AddCollider(Collider *collider);

    /**
     * @brief コライダーの削除
     * @param collider 削除するコライダーのポインタ
     * @details 指定されたコライダーを管理リストから削除する。
     */
    void RemoveCollider(Collider *collider);

    /**
     * @brief 全コライダー間の衝突判定
     * @details 登録された全コライダー間の衝突判定を一括で行い、OnCollisionEnter, OnCollisionStay, OnCollisionExitイベントを発行する。
     */
    void CheckAllCollisions();

    /**
     * @brief 管理リストのクリア
     * @details 全てのコライダーと履歴をリストから削除する。
     */
    void Clear();
};

#pragma once
#include "Vector2.h"

/**
 * @brief オブジェクトの分類タグを表す列挙型
 */
enum class ObjectTag
{
    None,
    Player,
    Enemy,
    PlayerWeapon,
    EnemyWeapon,
    Object,
    Item
};

class Collider;

/**
 * @brief ゲーム内全2Dオブジェクトの抽象基底クラス
 */
class Object2D
{
protected:
    Vector2 position;              ///< ワールド座標
    float width, height;           ///< サイズ
    bool isActive;                 ///< アクティブ状態フラグ
    bool markedForDeletion = false;///< 永久削除フラグ
    ObjectTag objectTag;           ///< オブジェクト識別タグ

public:
    /**
     * @brief コンストラクタ
     * @param tag オブジェクト種別タグ
     */
    Object2D(ObjectTag tag = ObjectTag::None);

    /**
     * @brief 仮想デストラクタ
     */
    virtual ~Object2D();

    /**
     * @brief オブジェクトタグの取得
     */
    ObjectTag GetObjectTag() const { return objectTag; }

    /**
     * @brief フレーム更新処理（純粋仮想関数）
     */
    virtual void Update() = 0;

    /**
     * @brief 描画処理（純粋仮想関数）
     */
    virtual void Draw() = 0;

    /**
     * @brief 衝突判定進入時イベント
     */
    virtual void OnCollisionEnter(Collider *otherCollider) {}

    /**
     * @brief 衝突判定滞在時イベント
     */
    virtual void OnCollisionStay(Collider *otherCollider) {}

    /**
     * @brief 衝突判定退出時イベント
     */
    virtual void OnCollisionExit(Collider *otherCollider) {}

    /**
     * @brief ワールド座標の取得
     */
    Vector2 GetPosition() const { return position; }

    /**
     * @brief ワールド座標の設定
     */
    void SetPosition(const Vector2 &pos) { position = pos; }

    /**
     * @brief 横幅の取得
     */
    float GetWidth() const { return width; }

    /**
     * @brief 縦幅の取得
     */
    float GetHeight() const { return height; }

    /**
     * @brief アクティブ状態の取得
     */
    bool IsActive() const { return isActive; }

    /**
     * @brief アクティブ状態の設定
     */
    void SetActive(bool active) { isActive = active; }

    /**
     * @brief 永久削除フラグを立てる（メモリ解放）
     */
    void DestroyPermanently() { markedForDeletion = true; }

    /**
     * @brief 永久削除フラグが立っているか
     */
    bool IsMarkedForDeletion() const { return markedForDeletion; }
};

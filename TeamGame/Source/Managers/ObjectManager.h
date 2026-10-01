#pragma once
#include <vector>

class Object2D;

/**
 * @brief オブジェクトを管理するクラス
 * @details ゲーム内のObject2Dインスタンスの追加、更新、描画、削除を管理する。
 */
class ObjectManager
{
  private:
    std::vector<Object2D *> objects;

  public:
    /**
     * @brief コンストラクタ
     * @details ObjectManagerのインスタンスを初期化する。
     */
    ObjectManager();

    /**
     * @brief デストラクタ
     * @details 管理しているオブジェクトを解放し、リソースをクリーンアップする。
     */
    ~ObjectManager();

    /**
     * @brief オブジェクトの追加
     * @param obj 追加するObject2Dのポインタ
     * @details 新しいオブジェクトを管理リストに追加する。
     */
    void AddObject(Object2D *obj);

    /**
     * @brief オブジェクトの更新
     * @details 管理しているすべてのオブジェクトの更新処理を行う。
     */
    void Update();

    /**
     * @brief オブジェクトの描画
     * @details 管理しているすべてのオブジェクトの描画処理を行う。
     */
    void Draw();

    /**
     * @brief 破棄されたオブジェクトの削除
     * @details 破棄フラグが立っているオブジェクトをリストから削除し、メモリを解放する。
     */
    void RemoveDestroyedObjects();

    /**
     * @brief すべてのオブジェクトのクリア
     * @details 管理しているすべてのオブジェクトを削除し、リストを空にする。
     */
    void Clear();

    /**
     * @brief オブジェクトリストの取得
     * @return std::vector<Object2D*>& 現在管理しているオブジェクトのリストへの定数参照
     * @details 現在管理しているオブジェクトのリストを取得する。
     */
    const std::vector<Object2D *>& GetObjects() const { return objects; }
};

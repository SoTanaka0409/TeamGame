#pragma once
#include "Scene.h"
#include <memory>

/**
 * @brief シーンを管理するシングルトンクラス
 * @details ゲーム内のシーン（タイトル、ゲームプレイ、リザルト等）の遷移と更新・描画を管理する。
 */
class SceneManager
{
  private:
    std::shared_ptr<Scene> currentScene;
    std::shared_ptr<Scene> nextScene;

    /**
     * @brief コンストラクタ
     * @details プライベートに設定され、外部からのインスタンス化を防ぐ。
     */
    SceneManager();

    /**
     * @brief デストラクタ
     * @details シーンマネージャの破棄時に呼ばれる。
     */
    ~SceneManager();

  public:
    /**
     * @brief インスタンスの取得
     * @return SceneManager& シーンマネージャのシングルトンインスタンス
     * @details シーンマネージャの唯一のインスタンスを取得する。
     */
    static SceneManager &GetInstance()
    {
        static SceneManager instance;
        return instance;
    }

    SceneManager(const SceneManager &) = delete;
    SceneManager &operator=(const SceneManager &) = delete;

    /**
     * @brief シーンの変更
     * @param newScene 遷移先の新しいシーン
     * @details 次のフレームの初めにシーンを切り替えるよう予約する。
     */
    void ChangeScene(std::shared_ptr<Scene> newScene);

    /**
     * @brief シーンの更新
     * @details 現在のシーンの更新処理を行う。必要に応じてシーンの切り替えも処理する。
     */
    void Update();

    /**
     * @brief シーンの描画
     * @details 現在のシーンの描画処理を行う。
     */
    void Draw();

    /**
     * @brief 現在のシーンの取得
     * @return std::shared_ptr<Scene> 現在アクティブなシーン
     * @details 現在実行中のシーンオブジェクトを取得する。
     */
    std::shared_ptr<Scene> GetCurrentScene() const
    {
        return currentScene;
    }
};

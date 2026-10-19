#pragma once
#include "Scene.h"

/**
 * @brief リザルト画面を管理するシーンクラス
 * @details 試合結果の詳細（キル数、スコアなど）の表示を行い、次のシーンへの遷移を制御します。
 */
class ResultScene : public Scene
{
  public:
    /**
     * @brief コンストラクタ
     * @details リザルトシーンの生成と初期化を行います。
     */
    ResultScene();

    /**
     * @brief デストラクタ
     * @details リソースの解放を行います。
     */
    ~ResultScene() override;

    /**
     * @brief 更新処理
     * @details 毎フレーム呼ばれ、入力待ち等の更新処理を行います。
     */
    void Update() override;

    /**
     * @brief 描画処理
     * @details 毎フレーム呼ばれ、リザルト情報やUIを描画します。
     */
    void Draw() override;
};

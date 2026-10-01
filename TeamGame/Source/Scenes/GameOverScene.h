#pragma once
#include "Scene.h"

/**
 * @brief ゲームオーバー画面を管理するシーンクラス
 * @details プレイヤー敗北時（ゲームオーバー）の演出、メニューの表示、リトライやタイトル画面への遷移を制御します。
 */
class GameOverScene : public Scene
{
  private:
    int menuCursor = 0;              ///< メニューのカーソル位置
    float animTimer = 0.0f;          ///< 演出用のアニメーションタイマー

    bool prevUp = false;             ///< 前フレームでの上入力状態
    bool prevDown = false;           ///< 前フレームでの下入力状態
    bool prevEnter = false;          ///< 前フレームでの決定入力状態

  public:
    /**
     * @brief コンストラクタ
     * @details ゲームオーバーシーンの生成と初期化を行います。
     */
    GameOverScene();

    /**
     * @brief デストラクタ
     * @details リソースの解放を行います。
     */
    ~GameOverScene() override;

    /**
     * @brief 初期化処理
     * @details アニメーションタイマーやメニューの状態などを初期化します。
     */
    void Init() override;

    /**
     * @brief 更新処理
     * @details 毎フレーム呼ばれ、メニューの入力処理やアニメーションの進行を行います。
     */
    void Update() override;

    /**
     * @brief 描画処理
     * @details 毎フレーム呼ばれ、ゲームオーバーテキストやメニュー項目を描画します。
     */
    void Draw() override;
};

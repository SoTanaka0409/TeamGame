#pragma once
#include "Scene.h"
#include "../UI/UITextWindow.h"

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

    // ========================================================
    // 【2D画像配列キャッシュ & 番号（インデックス）管理】
    // 一度読み込まれた画像は配列にキャッシュされ、番号（インデックス）で呼び出せます。
    // ========================================================
    int gameOverBgIndex = -1;        ///< ゲームオーバー画面背景画像の配列番号
    int windowFrameIndex = -1;       ///< UITextWindow枠画像の配列番号

    // UITextWindow による敗北メッセージ枠描画
    UI::UITextWindow* defeatWindow = nullptr; ///< 敗北パネル用テキストウィンドウ

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

#pragma once
#include "Scene.h"
#include "../UI/UITextWindow.h"
#include <string>

/**
 * @brief タイトルシーンを管理するクラス
 * @details ゲーム起動時のタイトル画面、設定画面などの状態を管理し、各種入力を受け付けます。
 */
class TitleScene : public Scene
{
  public:
    /**
     * @brief コンストラクタ
     */
    TitleScene();

    /**
     * @brief デストラクタ
     */
    ~TitleScene() override;

    /**
     * @brief タイトルシーンの状態定義
     */
    enum class TitleState { 
        MAIN,               ///< メイン画面
        SETTINGS            ///< 設定画面
    };
    
    TitleState state = TitleState::MAIN; ///< 現在のタイトル画面の状態
    int cursor = 0;                      ///< メニューのカーソル位置
    int waitTimer = 0;                   ///< 待機時間計測用のタイマー
    
    bool prevUp = false;                 ///< 前フレームでの上入力状態
    bool prevDown = false;               ///< 前フレームでの下入力状態
    bool prevEnter = false;              ///< 前フレームでの決定入力状態

    // ========================================================
    // 【2D画像配列キャッシュ & 番号（インデックス）管理】
    // ========================================================
    int titleBgIndex = -1;       ///< タイトル背景画像の配列番号
    int titleLogoIndex = -1;     ///< タイトルロゴ画像の配列番号
    int titleLogoIndex2 = -1;    ///< タイトルロゴ画像の配列番号
    int settingsBgIndex = -1;    ///< 設定画面背景画像の配列番号
    int windowFrameIndex = -1;   ///< UITextWindow用枠画像の配列番号

    // UITextWindow による設定枠描画
    mutable class UI::UITextWindow* settingsWindow = nullptr; ///< 設定画面用テキストウィンドウ

    /**
     * @brief タイトルシーンの更新処理
     */
    void Update() override;

    /**
     * @brief タイトルシーンの描画処理
     */
    void Draw() override;
};

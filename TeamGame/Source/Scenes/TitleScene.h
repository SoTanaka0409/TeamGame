#pragma once
#include "Scene.h"

/**
 * @brief タイトルシーンを管理するクラス
 * @details ゲーム起動時のタイトル画面、モード選択、ネットワーク接続設定などの状態を管理し、各種入力を受け付けます。
 */
class TitleScene : public Scene
{
  public:
    /**
     * @brief コンストラクタ
     * @details タイトルシーンの初期化を行います。
     */
    TitleScene();

    /**
     * @brief デストラクタ
     * @details タイトルシーンの終了処理を行い、確保したリソースを解放します。
     */
    ~TitleScene() override;

    /**
     * @brief タイトルシーンの状態定義
     * @details メイン画面、モード選択、LAN参加等の画面状態を表す列挙型です。
     */
    enum class TitleState { 
        MAIN,           ///< メイン画面
        MODE_SELECT,    ///< プレイモード選択画面
        JOIN_SELECT,    ///< LAN参加先選択画面
        SETTINGS,       ///< 設定画面
        WAITING,        ///< 待機中画面
        JOINING_LAN     ///< LANへの接続試行中画面
    };
    
    TitleState state = TitleState::MAIN; ///< 現在のタイトル画面の状態
    int cursor = 0;                      ///< メニューのカーソル位置
    int waitTimer = 0;                   ///< 待機時間計測用のタイマー
    
    int udpHandle = -1;                  ///< UDP通信用のハンドル
    char ipBuffer[64] = "192.168.1.";    ///< 入力するIPアドレスのバッファ
    
    bool prevUp = false;                 ///< 前フレームでの上入力状態
    bool prevDown = false;               ///< 前フレームでの下入力状態
    bool prevEnter = false;              ///< 前フレームでの決定入力状態

    /**
     * @brief タイトルシーンの更新処理
     * @details 毎フレーム呼ばれ、現在の状態（state）に応じた入力処理、メニュー遷移、ネットワーク接続処理等を行います。
     */
    void Update() override;

    /**
     * @brief タイトルシーンの描画処理
     * @details 毎フレーム呼ばれ、現在の状態（state）に応じたUIやテキストを描画します。
     */
    void Draw() override;
};

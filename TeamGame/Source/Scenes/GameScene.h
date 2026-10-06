#pragma once
#include "Scene.h"
#include "StageManager.h"
#include "DebugManager.h"
#include <vector>

class Enemy;

/**
 * @brief ゲームのプレイモード定義
 * @details ソロプレイ、ローカル協力プレイ、ネットワークプレイなどのプレイ方法を表します。
 */
enum class PlayMode
{
    SOLO,           ///< ソロプレイ
    LOCAL_COOP,     ///< ローカルでの協力プレイ（画面分割など）
    NETWORK_HOST,   ///< ネットワークプレイでのホスト
    NETWORK_CLIENT  ///< ネットワークプレイでのクライアント
};

/**
 * @brief ゲームの進行状態定義
 * @details プレイ中、ポーズ中、設定画面中などの状態を表します。
 */
enum class GameState
{
    PLAYING,        ///< ゲームプレイ中
    PAUSED,         ///< ポーズメニューを開いている状態
    SETTINGS        ///< 設定画面を開いている状態
};

/**
 * @brief ゲーム本編シーンを管理するクラス
 * @details プレイヤーや敵の制御、ステージの管理、ネットワークパケットの処理、勝敗判定など、ゲームのメインロジックを担当します。
 */
class GameScene : public Scene
{
  private:
    class Player *player;                ///< ローカルプレイヤー
    class Player *remotePlayer;          ///< リモートプレイヤーまたは2Pプレイヤー
    std::vector<Enemy*> enemies;         ///< 敵キャラクターのリスト
    StageManager stageManager;           ///< ステージの生成や管理を行うマネージャー

    PlayMode currentPlayMode = PlayMode::SOLO; ///< 現在のプレイモード
    GameState state = GameState::PLAYING;      ///< 現在のゲーム状態
    int pauseMenuCursor = 0;             ///< ポーズメニューのカーソル位置
    int settingsMenuCursor = 0;          ///< 設定メニューのカーソル位置

    bool prevEsc = false;                ///< 前フレームでのESC入力状態
    bool prevUp = false;                 ///< 前フレームでの上入力状態
    bool prevDown = false;               ///< 前フレームでの下入力状態
    bool prevLeft = false;               ///< 前フレームでの左入力状態
    bool prevRight = false;              ///< 前フレームでの右入力状態
    bool prevEnter = false;              ///< 前フレームでの決定入力状態

    float gameTimer = 0.0f;              ///< 経過時間のタイマー
    float introTimer = 0.0f;             ///< カメラ演出用のタイマー
    float brightTimer = 0.0f;            ///< 15秒間明るくなるイベントの残り時間（フレーム数）
    float nextBrightInterval = 1200.0f;  ///< 次の明るい時間帯が発生するまでのタイマー
    float sonarPingTimer = 900.0f;       ///< 15秒周期の相互音波ピン探知タイマー（900フレーム＝15秒）
    float sonarNotificationTimer = 0.0f; ///< 音波ピン発生時のUI通知表示タイマー
    int totalEnemiesSpawned = 0;         ///< スポーンした敵の総数
    int itemSpawnTimer = 0;              ///< アイテムスポーン用タイマー
    int trapSpawnTimer = 0;              ///< トラップスポーン用タイマー
    int horrorEffectTimer = 0;           ///< ホラー演出用のタイマー
    int team0Kills = 0;                  ///< チーム0（味方）のキル数
    int team1Kills = 0;                  ///< チーム1（敵）のキル数
    bool isCleared = false;              ///< クリア済みフラグ

    /**
     * @brief ネットワークパケットの処理
     * @details 受信したパケットを解析し、他のプレイヤーの移動やステージ生成などをゲームに反映させます。
     */
    void ProcessNetworkPackets();

  public:
    /**
     * @brief コンストラクタ
     * @param mode プレイモード（デフォルトはSOLO）
     * @details プレイモードを設定し、初期化の準備を行います。
     */
    GameScene(PlayMode mode = PlayMode::SOLO);

    /**
     * @brief ホラー演出をトリガーする
     * @param frames ホラー演出を継続するフレーム数
     * @details 指定したフレーム数だけ画面にホラー演出効果を適用します。
     */
    void TriggerHorrorEffect(int frames) { horrorEffectTimer = frames; }

    /**
     * @brief デストラクタ
     * @details ゲームシーン終了時のクリーンアップを行います。
     */
    ~GameScene() override;

    /**
     * @brief 初期化処理
     * @details プレイヤーの生成、ステージの構築、敵のスポーンなど、ゲーム開始に必要な初期化を行います。
     */
    void Init() override;

    /**
     * @brief 更新処理
     * @details 毎フレーム呼ばれ、キャラクターの移動、当たり判定、ゲームの勝敗判定、入力処理などを行います。
     */
    void Update() override;

    /**
     * @brief 描画処理
     * @details 毎フレーム呼ばれ、ステージやキャラクター、UIなどの描画を行います。
     */
    void Draw() override;

    /**
     * @brief 敵をランダムにスポーンさせる
     * @param count スポーンさせる敵の数
     * @details ステージ上のランダムな位置（壁などを避ける）に指定された数の敵キャラクターを配置します。
     */
    void SpawnEnemiesRandomly(int count);

    /**
     * @brief 全ての敵をクリア（無効化）する
     * @details 現在アクティブな全ての敵キャラクターを無効（非アクティブ）状態にします。
     */
    void ClearEnemies();

    /**
     * @brief アクティブな敵の数を取得する
     * @return 現在アクティブな敵の数
     * @details フィールド上に存在し、まだ倒されていない敵キャラクターの数をカウントして返します。
     */
    int GetActiveEnemyCount() const;

    /**
     * @brief 現在のステージを取得する
     * @return ステージへの定数ポインタ
     * @details StageManagerが管理している現在のステージ情報を返します。
     */
    const class Stage* GetStage() const override
    {
        return &stageManager.GetCurrentStage();
    }
};

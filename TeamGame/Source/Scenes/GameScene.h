#pragma once
#include "Scene.h"
#include "StageManager.h"
#include "../Core/Vector2.h"
#include <vector>

class Enemy;

/**
 * @brief ゲームのプレイモード定義（シングルプレイ専用）
 */
enum class PlayMode
{
    SOLO           ///< ソロプレイ
};

/**
 * @brief ゲームの進行状態定義
 */
enum class GameState
{
    PLAYING,        ///< ゲームプレイ中
    PAUSED,         ///< ポーズメニューを開いている状態
    SETTINGS        ///< 設定画面を開いている状態
};

/**
 * @brief ゲーム本編シーンを管理するクラス
 * @details プレイヤーや敵の制御、ステージ管理、勝敗判定など、シングルプレイのメインロジックを担当します。
 */
class GameScene : public Scene
{
  private:
    class Player *player;                ///< プレイヤー
    std::vector<Enemy*> enemies;         ///< 敵キャラクターのリスト
    StageManager stageManager;           ///< ステージマネージャー

    PlayMode currentPlayMode = PlayMode::SOLO; ///< プレイモード
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
    float sonarPingTimer = 900.0f;       ///< 15秒周期の相互音波ピン探知タイマー
    float sonarNotificationTimer = 0.0f; ///< 音波ピン発生時のUI通知表示タイマー
    int totalEnemiesSpawned = 0;         ///< スポーンした敵の総数
    int itemSpawnTimer = 0;              ///< アイテムスポーン用タイマー
    int trapSpawnTimer = 0;              ///< トラップスポーン用タイマー
    int horrorEffectTimer = 0;           ///< ホラー演出用のタイマー
    bool isCleared = false;              ///< クリア済みフラグ
    Vector2 m_cameraOffset = Vector2(0.0f, 0.0f); ///< エイム方向に応じた動的カメラオフセット

  public:
    GameScene(PlayMode mode = PlayMode::SOLO);
    ~GameScene() override;

    void TriggerHorrorEffect(int frames) { horrorEffectTimer = frames; }
    void Init() override;
    void Update() override;
    void Draw() override;

    void SpawnEnemiesRandomly(int count);
    void ClearEnemies();
    int GetActiveEnemyCount() const;
    int GetActiveAllyCount() const;

    const class Stage* GetStage() const override
    {
        return &stageManager.GetCurrentStage();
    }
};

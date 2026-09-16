#pragma once
#include "Scene.h"
#include "StageManager.h"
#include "DebugManager.h"
#include <vector>

class Enemy;


enum class PlayMode
{
    SOLO,
    LOCAL_COOP,
    NETWORK_HOST,
    NETWORK_CLIENT
};
enum class GameState
{
    PLAYING,
    PAUSED,
    SETTINGS
};

class GameScene : public Scene
{
  private:
    class Player *player;
    class Player *remotePlayer;
    std::vector<Enemy*> enemies;
    StageManager stageManager;

    PlayMode currentPlayMode = PlayMode::SOLO;
    GameState state = GameState::PLAYING;
    int pauseMenuCursor = 0;
    int settingsMenuCursor = 0;

    bool prevEsc = false;
    bool prevUp = false;
    bool prevDown = false;
    bool prevEnter = false;

    float gameTimer = 0.0f;
    int totalEnemiesSpawned = 0;
    int team0Kills = 0;
    int team1Kills = 0;
    bool isCleared = false;

    void ProcessNetworkPackets();

  public:
    GameScene(PlayMode mode = PlayMode::SOLO);
    ~GameScene() override;

    void Init() override;
    void Update() override;
    void Draw() override;

    void SpawnEnemiesRandomly(int count);
    void ClearEnemies();
    int GetActiveEnemyCount() const;

    const class Stage* GetStage() const override
    {
        return &stageManager.GetCurrentStage();
    }
};

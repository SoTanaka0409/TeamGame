#pragma once
#include "Scene.h"
#include "StageManager.h"
#include "DebugManager.h"
#include <vector>

class Enemy;

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

    GameState state = GameState::PLAYING;
    int pauseMenuCursor = 0;
    int settingsMenuCursor = 0;

    bool prevEsc = false;
    bool prevUp = false;
    bool prevDown = false;
    bool prevEnter = false;

    void ProcessNetworkPackets();

  public:
    GameScene();
    ~GameScene() override;

    void Init() override;
    void Update() override;
    void Draw() override;

    void SpawnEnemiesRandomly(int count);
    void ClearEnemies();

    const class Stage* GetStage() const override
    {
        return &stageManager.GetCurrentStage();
    }
};

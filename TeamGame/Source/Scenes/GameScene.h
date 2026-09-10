#pragma once
#include "Scene.h"
#include "StageManager.h"
#include "DebugManager.h"
#include <vector>

class Enemy;

class GameScene : public Scene
{
  private:
    class Player *player;
    std::vector<Enemy*> enemies;
    StageManager stageManager;

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

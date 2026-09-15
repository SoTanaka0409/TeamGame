#pragma once
#include "Scene.h"
#include <string>

struct ClearStats
{
    float clearTimeSec = 0.0f;
    int defeatedEnemies = 0;
    int totalEnemies = 0;
    int rankScore = 0;
    std::string rankName = "S";
};

class ClearScene : public Scene
{
  private:
    ClearStats stats;
    int menuCursor = 0;
    float animTimer = 0.0f;

    bool prevUp = false;
    bool prevDown = false;
    bool prevEnter = false;

  public:
    ClearScene(const ClearStats& stats = ClearStats());
    ~ClearScene() override;

    void Init() override;
    void Update() override;
    void Draw() override;
};

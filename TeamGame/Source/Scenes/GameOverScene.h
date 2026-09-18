#pragma once
#include "Scene.h"

class GameOverScene : public Scene
{
  private:
    int menuCursor = 0;
    float animTimer = 0.0f;

    bool prevUp = false;
    bool prevDown = false;
    bool prevEnter = false;

  public:
    GameOverScene();
    ~GameOverScene() override;

    void Init() override;
    void Update() override;
    void Draw() override;
};

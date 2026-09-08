#pragma once
#include "Scene.h"
#include "Stage.h"

class GameScene : public Scene
{
  private:
    class Player *player;
    Stage stage;
    int themeIdx;
    int varIdx;

  public:
    GameScene();
    ~GameScene() override;

    void Init() override;
    void Update() override;
    void Draw() override;
};

#pragma once
#include "Scene.h"

class GameScene : public Scene
{
  private:
    class Player *player;

  public:
    GameScene();
    ~GameScene() override;

    void Init() override;
    void Update() override;
    void Draw() override;
};

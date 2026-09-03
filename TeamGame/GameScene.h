#pragma once
#include "BaseScene.h"

class GameScene : public BaseScene
{
  public:
    GameScene();
    ~GameScene() override;

    void Init() override;
    void Update() override;
    void Draw() override;
    void Finalize() override;
};

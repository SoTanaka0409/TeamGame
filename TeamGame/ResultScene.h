#pragma once
#include "BaseScene.h"

class ResultScene : public BaseScene
{
  public:
    ResultScene();
    ~ResultScene() override;

    void Init() override;
    void Update() override;
    void Draw() override;
    void Finalize() override;
};

#pragma once
#include "BaseScene.h"

class TitleScene : public BaseScene
{
  public:
    TitleScene();
    ~TitleScene() override;

    void Init() override;
    void Update() override;
    void Draw() override;
    void Finalize() override;
};

#pragma once
#include "Scene.h"

class TitleScene : public Scene
{
  public:
    TitleScene();
    ~TitleScene() override;

    void Update() override;
    void Draw() override;
};

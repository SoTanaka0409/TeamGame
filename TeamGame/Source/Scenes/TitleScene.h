#pragma once
#include "Scene.h"

class TitleScene : public Scene
{
  public:
    TitleScene();
    ~TitleScene() override;

    enum class TitleState { MAIN, SETTINGS, WAITING, JOINING_LAN };
    TitleState state = TitleState::MAIN;
    int cursor = 0;
    int waitTimer = 0; 
    
    int udpHandle = -1;
    
    bool prevUp = false;
    bool prevDown = false;
    bool prevEnter = false;

    void Update() override;
    void Draw() override;
};

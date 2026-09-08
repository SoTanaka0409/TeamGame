#pragma once
#include "Scene.h"
#include "Stage.h"

enum class GameState
{
    PLAYING,
    PAUSED,
    SETTINGS
};

class GameScene : public Scene
{
  private:
    class Player *player;
    class Player *remotePlayer;
    Stage stage;
    int themeIdx;
    int varIdx;

    void ProcessNetworkPackets();

    GameState state = GameState::PLAYING;
    int pauseMenuCursor = 0;
    int settingsMenuCursor = 0;
    bool isDebugView = false;
    
    // キー入力の押しっぱなし防止用
    bool prevEsc = false;
    bool prevUp = false;
    bool prevDown = false;
    bool prevEnter = false;

  public:
    GameScene();
    ~GameScene() override;

    void Init() override;
    void Update() override;
    void Draw() override;
};

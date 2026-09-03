#pragma once
#include "BaseScene.h"
#include <memory>

class SceneManager
{
  private:
    std::shared_ptr<BaseScene> currentScene;
    std::shared_ptr<BaseScene> nextScene;

    SceneManager();
    ~SceneManager();

  public:
    static SceneManager &GetInstance()
    {
        static SceneManager instance;
        return instance;
    }

    SceneManager(const SceneManager &) = delete;
    SceneManager &operator=(const SceneManager &) = delete;

    void ChangeScene(std::shared_ptr<BaseScene> newScene);
    void Update();
    void Draw();
};

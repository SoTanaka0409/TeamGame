#pragma once
#include "Scene.h"
#include <memory>

class SceneManager
{
  private:
    std::shared_ptr<Scene> currentScene;
    std::shared_ptr<Scene> nextScene;

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

    void ChangeScene(std::shared_ptr<Scene> newScene);
    void Update();
    void Draw();

    std::shared_ptr<Scene> GetCurrentScene() const
    {
        return currentScene;
    }
};

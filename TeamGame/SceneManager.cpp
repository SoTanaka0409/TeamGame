#include "SceneManager.h"

SceneManager::SceneManager() : currentScene(nullptr), nextScene(nullptr)
{
}

SceneManager::~SceneManager()
{
}

void SceneManager::ChangeScene(std::shared_ptr<BaseScene> newScene)
{
    nextScene = newScene;
}

void SceneManager::Update()
{
    if (nextScene)
    {
        if (currentScene)
        {
            currentScene->Finalize();
        }
        currentScene = nextScene;
        currentScene->Init();
        nextScene = nullptr;
    }
    if (currentScene)
    {
        currentScene->Update();
    }
}

void SceneManager::Draw()
{
    if (currentScene)
    {
        currentScene->Draw();
    }
}

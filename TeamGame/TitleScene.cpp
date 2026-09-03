#include "TitleScene.h"
#include "DxLib.h"
#include "GameScene.h"
#include "InputManager.h"
#include "SceneManager.h"

TitleScene::TitleScene()
{
}
TitleScene::~TitleScene()
{
}

void TitleScene::Update()
{
    Scene::Update(); // Update objects in scene

    if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_RETURN))
    {
        SceneManager::GetInstance().ChangeScene(std::make_shared<GameScene>());
    }
}

void TitleScene::Draw()
{
    Scene::Draw();
    DrawString(100, 100, "=== タイトル画面 ===", GetColor(255, 255, 255));
    DrawString(100, 150, "Enterキーを押してスタート", GetColor(200, 200, 200));
}

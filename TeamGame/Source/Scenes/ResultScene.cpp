#include "ResultScene.h"
#include "DxLib.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "TitleScene.h"

ResultScene::ResultScene()
{
}
ResultScene::~ResultScene()
{
}

void ResultScene::Update()
{
    Scene::Update();

    if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_RETURN))
    {
        SceneManager::GetInstance().ChangeScene(std::make_shared<TitleScene>());
    }
}

void ResultScene::Draw()
{
    Scene::Draw();
    DrawString(100, 100, "=== リザルト画面 ===", GetColor(255, 255, 255));
    DrawString(100, 150, "Enterキーを押してタイトルに戻る", GetColor(200, 200, 200));
}

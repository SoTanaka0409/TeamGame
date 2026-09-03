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

void TitleScene::Init()
{
}

void TitleScene::Update()
{
    // ENTERキーでゲームシーンへ
    if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_RETURN))
    {
        SceneManager::GetInstance().ChangeScene(std::make_shared<GameScene>());
    }
}

void TitleScene::Draw()
{
    DrawString(100, 100, "=== タイトル画面 ===", GetColor(255, 255, 255));
    DrawString(100, 150, "Enterキーを押してスタート", GetColor(200, 200, 200));
}

void TitleScene::Finalize()
{
}

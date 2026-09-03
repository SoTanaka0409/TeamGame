#include "GameScene.h"
#include "DxLib.h"
#include "InputManager.h"
#include "ObjectManager.h"
#include "Player.h"
#include "ResultScene.h"
#include "SceneManager.h"

GameScene::GameScene()
{
}
GameScene::~GameScene()
{
}

void GameScene::Init()
{
    ObjectManager::GetInstance().Clear();
    new Player(320.0f, 240.0f);
}

void GameScene::Update()
{
    ObjectManager::GetInstance().Update();

    // SPACEキーでリザルトへ
    if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_SPACE))
    {
        SceneManager::GetInstance().ChangeScene(
            std::make_shared<ResultScene>());
    }
}

void GameScene::Draw()
{
    ObjectManager::GetInstance().Draw();
    DrawString(10, 10, "=== ゲーム画面 === (Spaceキーでリザルトへ)",
               GetColor(255, 255, 255));
}

void GameScene::Finalize()
{
    ObjectManager::GetInstance().Clear();
}

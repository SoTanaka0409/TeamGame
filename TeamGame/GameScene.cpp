#include "GameScene.h"
#include "DxLib.h"
#include "Enemy.h"
#include "InputManager.h"
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
    Scene::Init();
    new Player(320.0f, 240.0f);
    new Enemy(400.0f, 100.0f);
}

void GameScene::Update()
{
    Scene::Update(); // 自身の持つobjectManagerやcolliderManagerが実行される

    if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_SPACE))
    {
        SceneManager::GetInstance().ChangeScene(
            std::make_shared<ResultScene>());
    }
}

void GameScene::Draw()
{
    Scene::Draw();
    DrawString(10, 10, "=== ゲーム画面 === (Spaceキーでリザルトへ)",
               GetColor(255, 255, 255));
}

#include "GameScene.h"
#include "DxLib.h"
#include "Enemy.h"
#include "InputManager.h"
#include "Player.h"
#include "ResultScene.h"
#include "SceneManager.h"

GameScene::GameScene() : player(nullptr)
{
}
GameScene::~GameScene()
{
}

void GameScene::Init()
{
    Scene::Init();
    player = new Player(320.0f, 240.0f);
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

#include <cmath>

void GameScene::Draw()
{
    // 背景を暗いグレーにして、グリッド線を描く（何もない空間だと視界が分かりにくいため）
    DrawBox(0, 0, 1920, 1080, GetColor(40, 40, 40), TRUE);
    for (int x = 0; x < 1920; x += 100)
        DrawLine(x, 0, x, 1080, GetColor(60, 60, 60), 1);
    for (int y = 0; y < 1080; y += 100)
        DrawLine(0, y, 1920, y, GetColor(60, 60, 60), 1);

    Scene::Draw();

    if (player && player->IsActive())
    {
        Vector2 pos = player->GetPosition();
        Vector2 dir = player->GetFacingDir();

        float facingAngle = std::atan2(dir.y, dir.x);
        float fov = 90.0f * (3.14159265f / 180.0f);

        // --- 1. ライトの範囲（グレー半透明）を描画 ---
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);
        float lightStart = facingAngle - fov / 2.0f;
        float lightStep = fov / 16.0f;
        float lightDist = 600.0f; // ライトが届く距離

        for (int i = 0; i < 16; ++i)
        {
            float a1 = lightStart + lightStep * i;
            float a2 = lightStart + lightStep * (i + 1);
            int x1 = static_cast<int>(pos.x + std::cos(a1) * lightDist);
            int y1 = static_cast<int>(pos.y + std::sin(a1) * lightDist);
            int x2 = static_cast<int>(pos.x + std::cos(a2) * lightDist);
            int y2 = static_cast<int>(pos.y + std::sin(a2) * lightDist);
            DrawTriangle(static_cast<int>(pos.x), static_cast<int>(pos.y), x1,
                         y1, x2, y2, GetColor(200, 200, 200), TRUE);
        }

        // --- 2. 視界以外の暗闇を描画 ---
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 240); // ほとんど真っ暗

        // 横と後ろの暗闇
        float angleStart = facingAngle + fov / 2.0f;
        float angleEnd = facingAngle + 2.0f * 3.14159265f - fov / 2.0f;
        int steps = 24;
        float angleStep = (angleEnd - angleStart) / steps;

        for (int i = 0; i < steps; ++i)
        {
            float a1 = angleStart + angleStep * i;
            float a2 = angleStart + angleStep * (i + 1);
            float dist = 3000.0f;
            int x1 = static_cast<int>(pos.x + std::cos(a1) * dist);
            int y1 = static_cast<int>(pos.y + std::sin(a1) * dist);
            int x2 = static_cast<int>(pos.x + std::cos(a2) * dist);
            int y2 = static_cast<int>(pos.y + std::sin(a2) * dist);
            DrawTriangle(static_cast<int>(pos.x), static_cast<int>(pos.y), x1,
                         y1, x2, y2, GetColor(0, 0, 0), TRUE);
        }

        // 正面だけどライトが届かない奥の暗闇
        for (int i = 0; i < 16; ++i)
        {
            float a1 = lightStart + lightStep * i;
            float a2 = lightStart + lightStep * (i + 1);
            float distFar = 3000.0f;
            int x1 = static_cast<int>(pos.x + std::cos(a1) * lightDist);
            int y1 = static_cast<int>(pos.y + std::sin(a1) * lightDist);
            int x2 = static_cast<int>(pos.x + std::cos(a2) * lightDist);
            int y2 = static_cast<int>(pos.y + std::sin(a2) * lightDist);

            int x3 = static_cast<int>(pos.x + std::cos(a1) * distFar);
            int y3 = static_cast<int>(pos.y + std::sin(a1) * distFar);
            int x4 = static_cast<int>(pos.x + std::cos(a2) * distFar);
            int y4 = static_cast<int>(pos.y + std::sin(a2) * distFar);

            DrawQuadrangle(x1, y1, x2, y2, x4, y4, x3, y3, GetColor(0, 0, 0),
                           TRUE);
        }

        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}

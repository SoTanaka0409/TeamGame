#include "Player.h"
#include "DxLib.h"
#include "InputManager.h"

Player::Player(float startX, float startY)
{
    x = startX;
    y = startY;
    radius = 20.0f;
    width = radius * 2.0f;
    height = radius * 2.0f;
    speed = 5.0f;
}

Player::~Player()
{
}

void Player::Update()
{
    if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_LEFT))
        x -= speed;
    if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_RIGHT))
        x += speed;
    if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_UP))
        y -= speed;
    if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_DOWN))
        y += speed;
}

void Player::Draw()
{
    DrawCircle(static_cast<int>(x), static_cast<int>(y),
               static_cast<int>(radius), GetColor(0, 255, 0), TRUE);
}

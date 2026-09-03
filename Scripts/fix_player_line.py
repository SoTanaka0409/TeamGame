import os

DIR = r'C:\Users\student\Desktop\TeamGame\TeamGame\TeamGame'

player_h_path = os.path.join(DIR, 'Player.h')
with open(player_h_path, 'r', encoding='utf-8') as f:
    player_h = f.read()

if 'Vector2 facingDir;' not in player_h:
    player_h = player_h.replace('int damageColorTimer;', 'int damageColorTimer;\n    Vector2 facingDir;')
    with open(player_h_path, 'w', encoding='utf-8') as f:
        f.write(player_h)

player_cpp_path = os.path.join(DIR, 'Player.cpp')

new_cpp = '''#include "Player.h"
#include "Enemy.h"
#include "InputManager.h"
#include "DxLib.h"
#include <cmath>

Player::Player(float startX, float startY) : Character(startX, startY, 20.0f), damageColorTimer(0), facingDir(0.0f, -1.0f)
{
    speed = 5.0f;
    collider->SetTag("Player");
}

Player::~Player()
{
}

void Player::Update()
{
    bool isMoving = false;
    Vector2 moveDir(0.0f, 0.0f);

    if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_LEFT))
    {
        moveDir.x -= 1.0f;
        isMoving = true;
    }
    if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_RIGHT))
    {
        moveDir.x += 1.0f;
        isMoving = true;
    }
    if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_UP))
    {
        moveDir.y -= 1.0f;
        isMoving = true;
    }
    if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_DOWN))
    {
        moveDir.y += 1.0f;
        isMoving = true;
    }

    if (isMoving)
    {
        facingDir = moveDir;

        // 斜め移動時の速度を一定にするための正規化
        float length = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
        position.x += (moveDir.x / length) * speed;
        position.y += (moveDir.y / length) * speed;
    }

    if (damageColorTimer > 0)
    {
        damageColorTimer--;
    }
}

void Player::Draw()
{
    unsigned int color = (damageColorTimer > 0) ? GetColor(255, 255, 0) : GetColor(0, 255, 0);
    DrawCircle(static_cast<int>(position.x), static_cast<int>(position.y), static_cast<int>(radius), color, TRUE);

    // 向いている方角を線で描画
    float lineLen = 50.0f; // 線の長さ
    float len = std::sqrt(facingDir.x * facingDir.x + facingDir.y * facingDir.y);
    float nx = facingDir.x;
    float ny = facingDir.y;
    
    if (len > 0.0001f)
    {
        nx /= len;
        ny /= len;
    }
    
    int x1 = static_cast<int>(position.x);
    int y1 = static_cast<int>(position.y);
    int x2 = static_cast<int>(position.x + nx * lineLen);
    int y2 = static_cast<int>(position.y + ny * lineLen);
    
    // 白い線を描画（太さ2）
    DrawLine(x1, y1, x2, y2, GetColor(255, 255, 255), 2);
}

void Player::OnCollisionEnter(Collider* otherCollider)
{
    Enemy* enemy = dynamic_cast<Enemy*>(otherCollider->GetOwner());
    if (enemy)
    {
        enemy->Damage();
        damageColorTimer = 30;
    }
}

void Player::OnCollisionStay(Collider* otherCollider)
{
}

void Player::OnCollisionExit(Collider* otherCollider)
{
}
'''
with open(player_cpp_path, 'w', encoding='utf-8') as f:
    f.write(new_cpp)

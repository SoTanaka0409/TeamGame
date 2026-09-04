#include "Player.h"
#include "Bullet.h"
#include "DxLib.h"
#include "Enemy.h"
#include "Handgun.h"
#include "InputManager.h"
#include "Shotgun.h"
#include <cmath>

Player::Player(float startX, float startY)
    : Character(startX, startY, 35.0f), damageColorTimer(0),
      facingDir(0.0f, -1.0f)
{
    speed = 5.0f;
    collider->SetTag("Player");
    weapons.push_back(new Handgun());
    weapons.push_back(new Shotgun());
    currentWeaponIndex = 0;
}

Player::~Player()
{
    for (auto w : weapons)
        delete w;
    weapons.clear();
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
        float length = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
        if (length >
            0.0001f) // 左右同時押しなどで0になった場合のゼロ除算(NaN)回避
        {
            facingDir = moveDir;
            position.x += (moveDir.x / length) * speed;
            position.y += (moveDir.y / length) * speed;
        }
    }

    if (damageColorTimer > 0)
    {
        damageColorTimer--;
    }

    if (!weapons.empty())
    {
        weapons[currentWeaponIndex]->Update();
    }

    // Qキーで武器チェンジ
    if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_Q))
    {
        currentWeaponIndex = (currentWeaponIndex + 1) % weapons.size();
    }

    // Zキーを押しっぱなしで発射（クールタイムはWeaponクラスが管理）
    if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_Z))
    {
        if (!weapons.empty())
        {
            weapons[currentWeaponIndex]->Fire(position, facingDir);
        }
    }
}

void Player::Draw()
{
    unsigned int color =
        (damageColorTimer > 0) ? GetColor(255, 255, 0) : GetColor(0, 255, 0);
    DrawCircle(static_cast<int>(position.x), static_cast<int>(position.y),
               static_cast<int>(radius), color, TRUE);

    // 向いている方角を線で描画
    float lineLen = 50.0f; // 線の長さ
    float len =
        std::sqrt(facingDir.x * facingDir.x + facingDir.y * facingDir.y);
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

    if (!weapons.empty())
    {
        DrawString(static_cast<int>(position.x) - 20,
                   static_cast<int>(position.y) - 30,
                   weapons[currentWeaponIndex]->GetName().c_str(),
                   GetColor(255, 255, 255));
    }
}

void Player::OnCollisionEnter(Collider *otherCollider)
{
    Enemy *enemy = dynamic_cast<Enemy *>(otherCollider->GetOwner());
    if (enemy)
    {
        enemy->Damage();
        damageColorTimer = 30;
    }
}

void Player::OnCollisionStay(Collider *otherCollider)
{
}

void Player::OnCollisionExit(Collider *otherCollider)
{
}

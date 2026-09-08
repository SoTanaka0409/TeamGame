import re
import os

player_h = r'C:\Users\student\Desktop\TeamGame\TeamGame\TeamGame\Source\Characters\Player.h'
with open(player_h, 'r', encoding='utf-8') as f:
    h_code = f.read()

# Add SetStage
if 'void SetStage' not in h_code:
    h_code = h_code.replace('void Update() override;', 'void SetStage(class Stage* s, float cSize) { currentStage = s; cellSize = cSize; }\n    void Update() override;')
    h_code = h_code.replace('private:', 'private:\n    class Stage* currentStage = nullptr;\n    float cellSize = 1.0f;')

with open(player_h, 'w', encoding='utf-8') as f:
    f.write(h_code)

player_cpp = r'C:\Users\student\Desktop\TeamGame\TeamGame\TeamGame\Source\Characters\Player.cpp'
with open(player_cpp, 'r', encoding='utf-8') as f:
    cpp_code = f.read()

if '#include "Stage.h"' not in cpp_code:
    cpp_code = cpp_code.replace('#include "Player.h"', '#include "Player.h"\n#include "Stage.h"')

new_update = '''void Player::Update()
{
    // マウスで視点移動（向きをマウスに合わせる）
    int mouseX, mouseY;
    GetMousePoint(&mouseX, &mouseY);
    float dx = mouseX - position.x;
    float dy = mouseY - position.y;
    float dirLen = std::sqrt(dx * dx + dy * dy);
    if (dirLen > 0.0001f)
    {
        facingDir.x = dx / dirLen;
        facingDir.y = dy / dirLen;
    }

    bool isMoving = false;
    Vector2 moveDir(0.0f, 0.0f);

    if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_LEFT) || InputManager::GetInstance().IsKeyHeld(KEY_INPUT_A))
    {
        moveDir.x -= 1.0f;
        isMoving = true;
    }
    if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_RIGHT) || InputManager::GetInstance().IsKeyHeld(KEY_INPUT_D))
    {
        moveDir.x += 1.0f;
        isMoving = true;
    }
    if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_UP) || InputManager::GetInstance().IsKeyHeld(KEY_INPUT_W))
    {
        moveDir.y -= 1.0f;
        isMoving = true;
    }
    if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_DOWN) || InputManager::GetInstance().IsKeyHeld(KEY_INPUT_S))
    {
        moveDir.y += 1.0f;
        isMoving = true;
    }

    if (isMoving)
    {
        float length = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
        if (length > 0.0001f)
        {
            float velX = (moveDir.x / length) * speed;
            float velY = (moveDir.y / length) * speed;
            
            // X軸の移動と衝突判定
            if (currentStage)
            {
                float nextX = position.x + velX;
                // ピクセル座標からグリッド座標に変換
                int gridX = static_cast<int>(nextX / cellSize);
                int gridY = static_cast<int>(position.y / cellSize);
                if (!currentStage->IsSolidWall(gridX, gridY))
                {
                    position.x = nextX;
                }
                
                // Y軸の移動と衝突判定
                float nextY = position.y + velY;
                gridX = static_cast<int>(position.x / cellSize);
                gridY = static_cast<int>(nextY / cellSize);
                if (!currentStage->IsSolidWall(gridX, gridY))
                {
                    position.y = nextY;
                }
            }
            else
            {
                position.x += velX;
                position.y += velY;
            }
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

    // 左クリックまたはZキーで発射
    if (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_Z) || (GetMouseInput() & MOUSE_INPUT_LEFT))
    {
        if (!weapons.empty())
        {
            weapons[currentWeaponIndex]->Fire(position, facingDir);
        }
    }
}'''

cpp_code = re.sub(r'void Player::Update\(\)[\s\S]*?\}\n\nvoid Player::Draw', new_update + '\n\nvoid Player::Draw', cpp_code)

with open(player_cpp, 'w', encoding='utf-8') as f:
    f.write(cpp_code)

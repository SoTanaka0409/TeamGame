import os

DIR = r'C:\Users\student\Desktop\TeamGame\TeamGame\TeamGame'

# Fix Character.h
char_h_path = os.path.join(DIR, 'Character.h')
with open(char_h_path, 'r', encoding='utf-8') as f:
    char_h = f.read()

if 'ColliderManager* myColliderManager;' not in char_h:
    char_h = char_h.replace('CircleCollider* collider;', 'CircleCollider* collider;\n    class ColliderManager* myColliderManager;')
    with open(char_h_path, 'w', encoding='utf-8') as f:
        f.write(char_h)

# Fix Character.cpp
char_cpp_path = os.path.join(DIR, 'Character.cpp')
with open(char_cpp_path, 'r', encoding='utf-8') as f:
    char_cpp = f.read()

new_char_cpp = '''#include "Character.h"
#include "SceneManager.h"
#include "Scene.h"
#include "ColliderManager.h"

Character::Character(float startX, float startY, float rad) : speed(0.0f), radius(rad), myColliderManager(nullptr)
{
    position = Vector2(startX, startY);
    width = rad * 2.0f;
    height = rad * 2.0f;

    collider = new CircleCollider(this, rad, "CharacterBody");
    
    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (scene)
    {
        myColliderManager = scene->GetColliderManager();
        myColliderManager->AddCollider(collider);
    }
}

Character::~Character()
{
    if (myColliderManager)
    {
        myColliderManager->RemoveCollider(collider);
    }
    delete collider;
}
'''
with open(char_cpp_path, 'w', encoding='utf-8') as f:
    f.write(new_char_cpp)

# Fix Player.cpp
player_cpp_path = os.path.join(DIR, 'Player.cpp')
with open(player_cpp_path, 'r', encoding='utf-8') as f:
    player_cpp = f.read()

new_player_cpp = player_cpp.replace('''    if (isMoving)
    {
        facingDir = moveDir;

        // 斜め移動時の速度を一定にするための正規化
        float length = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
        position.x += (moveDir.x / length) * speed;
        position.y += (moveDir.y / length) * speed;
    }''', '''    if (isMoving)
    {
        float length = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
        if (length > 0.0001f) // 左右同時押しなどで0になった場合のゼロ除算(NaN)回避
        {
            facingDir = moveDir;
            position.x += (moveDir.x / length) * speed;
            position.y += (moveDir.y / length) * speed;
        }
    }''')

with open(player_cpp_path, 'w', encoding='utf-8') as f:
    f.write(new_player_cpp)

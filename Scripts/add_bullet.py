import os
import xml.etree.ElementTree as ET

DIR = r'C:\Users\student\Desktop\TeamGame\TeamGame\TeamGame'

bullet_h = '''#pragma once
#include "Object2D.h"
#include "CircleCollider.h"
#include "Vector2.h"

class ColliderManager;

class Bullet : public Object2D
{
private:
    CircleCollider* collider;
    ColliderManager* myColliderManager;
    Vector2 velocity;
    float radius;

public:
    Bullet(float startX, float startY, const Vector2& dir, float speed);
    virtual ~Bullet();

    void Update() override;
    void Draw() override;

    void OnCollisionEnter(Collider* otherCollider) override;
};
'''

bullet_cpp = '''#include "Bullet.h"
#include "Enemy.h"
#include "SceneManager.h"
#include "Scene.h"
#include "ColliderManager.h"
#include "DxLib.h"

Bullet::Bullet(float startX, float startY, const Vector2& dir, float speed) 
    : myColliderManager(nullptr), radius(5.0f)
{
    position = Vector2(startX, startY);
    width = radius * 2.0f;
    height = radius * 2.0f;
    velocity = Vector2(dir.x * speed, dir.y * speed);

    collider = new CircleCollider(this, radius, "PlayerBullet");

    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (scene)
    {
        myColliderManager = scene->GetColliderManager();
        myColliderManager->AddCollider(collider);
    }
}

Bullet::~Bullet()
{
    if (myColliderManager)
    {
        myColliderManager->RemoveCollider(collider);
    }
    delete collider;
}

void Bullet::Update()
{
    position.x += velocity.x;
    position.y += velocity.y;

    // 画面外に出たら消滅
    if (position.x < -100 || position.x > 2000 || position.y < -100 || position.y > 1200)
    {
        SetActive(false);
    }
}

void Bullet::Draw()
{
    DrawCircle(static_cast<int>(position.x), static_cast<int>(position.y), static_cast<int>(radius), GetColor(0, 255, 255), TRUE);
}

void Bullet::OnCollisionEnter(Collider* otherCollider)
{
    // 敵に当たったらダメージを与えて自身も消滅
    if (otherCollider->GetTag() == "Enemy")
    {
        Enemy* enemy = dynamic_cast<Enemy*>(otherCollider->GetOwner());
        if (enemy)
        {
            enemy->Damage();
        }
        SetActive(false);
    }
}
'''

with open(os.path.join(DIR, 'Bullet.h'), 'w', encoding='utf-8') as f:
    f.write(bullet_h)
with open(os.path.join(DIR, 'Bullet.cpp'), 'w', encoding='utf-8') as f:
    f.write(bullet_cpp)

player_cpp_path = os.path.join(DIR, 'Player.cpp')
with open(player_cpp_path, 'r', encoding='utf-8') as f:
    player_cpp = f.read()

if 'Bullet.h' not in player_cpp:
    player_cpp = player_cpp.replace('#include "InputManager.h"', '#include "InputManager.h"\n#include "Bullet.h"')
    
    update_code = '''    if (damageColorTimer > 0)
    {
        damageColorTimer--;
    }

    // Zキーで弾を発射
    if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_Z))
    {
        new Bullet(position.x, position.y, facingDir, 15.0f);
    }
}'''
    player_cpp = player_cpp.replace('    if (damageColorTimer > 0)\n    {\n        damageColorTimer--;\n    }\n}', update_code)
    with open(player_cpp_path, 'w', encoding='utf-8') as f:
        f.write(player_cpp)


# Update vcxproj
proj = os.path.join(DIR, 'TeamGame.vcxproj')
filters = os.path.join(DIR, 'TeamGame.vcxproj.filters')

ET.register_namespace('', 'http://schemas.microsoft.com/developer/msbuild/2003')
ns = {'ms': 'http://schemas.microsoft.com/developer/msbuild/2003'}

tree = ET.parse(proj)
root = tree.getroot()
compile_group = None
include_group = None
for ig in root.findall('ms:ItemGroup', ns):
    if ig.find('ms:ClCompile', ns) is not None:
        compile_group = ig
    if ig.find('ms:ClInclude', ns) is not None:
        include_group = ig

if 'Bullet.cpp' not in [c.get('Include') for c in compile_group.findall('ms:ClCompile', ns)]:
    el = ET.SubElement(compile_group, 'ClCompile')
    el.set('Include', 'Bullet.cpp')
if 'Bullet.h' not in [c.get('Include') for c in include_group.findall('ms:ClInclude', ns)]:
    el = ET.SubElement(include_group, 'ClInclude')
    el.set('Include', 'Bullet.h')

tree.write(proj, encoding='utf-8', xml_declaration=True)

tree_f = ET.parse(filters)
root_f = tree_f.getroot()
compile_group_f = None
include_group_f = None
for ig in root_f.findall('ms:ItemGroup', ns):
    if ig.find('ms:ClCompile', ns) is not None:
        compile_group_f = ig
    elif ig.find('ms:ClInclude', ns) is not None:
        include_group_f = ig

if 'Bullet.cpp' not in [c.get('Include') for c in compile_group_f.findall('ms:ClCompile', ns)]:
    el = ET.SubElement(compile_group_f, 'ClCompile')
    el.set('Include', 'Bullet.cpp')
    filt = ET.SubElement(el, 'Filter')
    filt.text = 'ソース ファイル'
if 'Bullet.h' not in [c.get('Include') for c in include_group_f.findall('ms:ClInclude', ns)]:
    el = ET.SubElement(include_group_f, 'ClInclude')
    el.set('Include', 'Bullet.h')
    filt = ET.SubElement(el, 'Filter')
    filt.text = 'ヘッダー ファイル'

tree_f.write(filters, encoding='utf-8', xml_declaration=True)

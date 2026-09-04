import os
import xml.etree.ElementTree as ET

DIR = r'C:\Users\student\Desktop\TeamGame\TeamGame\TeamGame'

files = {
    'Weapon.h': '''#pragma once
#include "Vector2.h"
#include <string>

class Weapon
{
protected:
    int coolTimeTimer;
    int fireRate; // クールタイムの最大値（何フレームに1回撃てるか）
    std::string weaponName;

public:
    Weapon(const std::string& name, int fireRate) : weaponName(name), fireRate(fireRate), coolTimeTimer(0) {}
    virtual ~Weapon() {}

    virtual void Update() {
        if (coolTimeTimer > 0) coolTimeTimer--;
    }

    virtual void Fire(const Vector2& pos, const Vector2& dir) = 0;

    bool CanFire() const { return coolTimeTimer <= 0; }
    void ResetCoolTime() { coolTimeTimer = fireRate; }
    
    std::string GetName() const { return weaponName; }
};
''',

    'Handgun.h': '''#pragma once
#include "Weapon.h"

class Handgun : public Weapon
{
public:
    Handgun();
    void Fire(const Vector2& pos, const Vector2& dir) override;
};
''',

    'Handgun.cpp': '''#include "Handgun.h"
#include "Bullet.h"

Handgun::Handgun() : Weapon("Handgun", 15) // 15フレームに1回発射可能
{
}

void Handgun::Fire(const Vector2& pos, const Vector2& dir)
{
    if (!CanFire()) return;
    
    new Bullet(pos.x, pos.y, dir, 15.0f); // まっすぐ速い弾
    ResetCoolTime();
}
''',

    'Shotgun.h': '''#pragma once
#include "Weapon.h"

class Shotgun : public Weapon
{
public:
    Shotgun();
    void Fire(const Vector2& pos, const Vector2& dir) override;
};
''',

    'Shotgun.cpp': '''#include "Shotgun.h"
#include "Bullet.h"
#include <cmath>

Shotgun::Shotgun() : Weapon("Shotgun", 45) // 45フレームに1回発射可能（少し遅い）
{
}

void Shotgun::Fire(const Vector2& pos, const Vector2& dir)
{
    if (!CanFire()) return;

    // 現在向いている方向の角度を計算
    float baseAngle = std::atan2(dir.y, dir.x);
    
    // 3発扇状に発射（-15度、0度、+15度ぐらいのズレ）
    float angles[3] = { -0.25f, 0.0f, 0.25f }; 

    for (int i = 0; i < 3; ++i)
    {
        float angle = baseAngle + angles[i];
        Vector2 spreadDir(std::cos(angle), std::sin(angle));
        new Bullet(pos.x, pos.y, spreadDir, 12.0f);
    }
    
    ResetCoolTime();
}
'''
}

for name, content in files.items():
    with open(os.path.join(DIR, name), 'w', encoding='utf-8') as f:
        f.write(content)

# Update Player.h
player_h_path = os.path.join(DIR, 'Player.h')
with open(player_h_path, 'r', encoding='utf-8') as f:
    player_h = f.read()

if '#include <vector>' not in player_h:
    player_h = player_h.replace('#pragma once\n#include "Character.h"', '#pragma once\n#include "Character.h"\n#include <vector>\nclass Weapon;')
    player_h = player_h.replace('Vector2 facingDir;', 'Vector2 facingDir;\n    std::vector<Weapon*> weapons;\n    int currentWeaponIndex;')
    with open(player_h_path, 'w', encoding='utf-8') as f:
        f.write(player_h)

# Update Player.cpp
player_cpp_path = os.path.join(DIR, 'Player.cpp')
with open(player_cpp_path, 'r', encoding='utf-8') as f:
    player_cpp = f.read()

# Add Includes
if '"Handgun.h"' not in player_cpp:
    player_cpp = player_cpp.replace('#include "Bullet.h"', '#include "Bullet.h"\n#include "Handgun.h"\n#include "Shotgun.h"')

# Update Constructor
if 'currentWeaponIndex = 0;' not in player_cpp:
    player_cpp = player_cpp.replace('collider->SetTag("Player");', 'collider->SetTag("Player");\n    weapons.push_back(new Handgun());\n    weapons.push_back(new Shotgun());\n    currentWeaponIndex = 0;')

# Update Destructor
if 'weapons.clear();' not in player_cpp:
    player_cpp = player_cpp.replace('Player::~Player()\n{\n}', 'Player::~Player()\n{\n    for (auto w : weapons) delete w;\n    weapons.clear();\n}')

# Update Update
if 'currentWeaponIndex = (currentWeaponIndex + 1)' not in player_cpp:
    old_fire_code = '''    // Zキーで弾を発射
    if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_Z))
    {
        new Bullet(position.x, position.y, facingDir, 15.0f);
    }'''
    
    new_fire_code = '''    if (!weapons.empty())
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
    }'''
    player_cpp = player_cpp.replace(old_fire_code, new_fire_code)

# Update Draw
if 'weapons[currentWeaponIndex]->GetName().c_str()' not in player_cpp:
    old_draw = 'DrawLine(x1, y1, x2, y2, GetColor(255, 255, 255), 2);'
    new_draw = 'DrawLine(x1, y1, x2, y2, GetColor(255, 255, 255), 2);\n\n    if (!weapons.empty()) {\n        DrawString(static_cast<int>(position.x) - 20, static_cast<int>(position.y) - 30, weapons[currentWeaponIndex]->GetName().c_str(), GetColor(255,255,255));\n    }'
    player_cpp = player_cpp.replace(old_draw, new_draw)

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

cpp_files = ['Handgun.cpp', 'Shotgun.cpp']
h_files = ['Weapon.h', 'Handgun.h', 'Shotgun.h']

existing_cpps = [c.get('Include') for c in compile_group.findall('ms:ClCompile', ns)]
existing_hs = [c.get('Include') for c in include_group.findall('ms:ClInclude', ns)]

for f in cpp_files:
    if f not in existing_cpps:
        el = ET.SubElement(compile_group, 'ClCompile')
        el.set('Include', f)
for f in h_files:
    if f not in existing_hs:
        el = ET.SubElement(include_group, 'ClInclude')
        el.set('Include', f)

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

for f in cpp_files:
    if f not in existing_cpps:
        el = ET.SubElement(compile_group_f, 'ClCompile')
        el.set('Include', f)
        filt = ET.SubElement(el, 'Filter')
        filt.text = 'ソース ファイル'
for f in h_files:
    if f not in existing_hs:
        el = ET.SubElement(include_group_f, 'ClInclude')
        el.set('Include', f)
        filt = ET.SubElement(el, 'Filter')
        filt.text = 'ヘッダー ファイル'

tree_f.write(filters, encoding='utf-8', xml_declaration=True)

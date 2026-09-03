import os
import xml.etree.ElementTree as ET

DIR = r'C:\Users\student\Desktop\TeamGame\TeamGame\TeamGame'

player_h = '''#pragma once
#include "Object2D.h"

class Player : public Object2D {
private:
    float speed;
    float radius;

public:
    Player(float startX, float startY);
    virtual ~Player();

    void Update() override;
    void Draw() override;
};
'''

player_cpp = '''#include "Player.h"
#include "DxLib.h"

Player::Player(float startX, float startY) {
    x = startX;
    y = startY;
    radius = 20.0f;
    width = radius * 2.0f;
    height = radius * 2.0f;
    speed = 5.0f;
}

Player::~Player() {}

void Player::Update() {
    if (CheckHitKey(KEY_INPUT_LEFT) == 1)  x -= speed;
    if (CheckHitKey(KEY_INPUT_RIGHT) == 1) x += speed;
    if (CheckHitKey(KEY_INPUT_UP) == 1)    y -= speed;
    if (CheckHitKey(KEY_INPUT_DOWN) == 1)  y += speed;
}

void Player::Draw() {
    DrawCircle(static_cast<int>(x), static_cast<int>(y), static_cast<int>(radius), GetColor(0, 255, 0), TRUE);
}
'''

with open(os.path.join(DIR, 'Player.h'), 'w', encoding='utf-8') as f:
    f.write(player_h)
with open(os.path.join(DIR, 'Player.cpp'), 'w', encoding='utf-8') as f:
    f.write(player_cpp)

proj = os.path.join(DIR, 'TeamGame.vcxproj')
ET.register_namespace('', 'http://schemas.microsoft.com/developer/msbuild/2003')
tree = ET.parse(proj)
root = tree.getroot()
ns = {'ms': 'http://schemas.microsoft.com/developer/msbuild/2003'}

compile_group = None
include_group = None
for ig in root.findall('ms:ItemGroup', ns):
    if ig.find('ms:ClCompile', ns) is not None:
        compile_group = ig
    if ig.find('ms:ClInclude', ns) is not None:
        include_group = ig

if compile_group is not None and 'Player.cpp' not in [c.get('Include') for c in compile_group.findall('ms:ClCompile', ns)]:
    el = ET.SubElement(compile_group, 'ClCompile')
    el.set('Include', 'Player.cpp')
if include_group is not None and 'Player.h' not in [c.get('Include') for c in include_group.findall('ms:ClInclude', ns)]:
    el = ET.SubElement(include_group, 'ClInclude')
    el.set('Include', 'Player.h')

tree.write(proj, encoding='utf-8', xml_declaration=True)

main_cpp_path = os.path.join(DIR, 'main.cpp')
with open(main_cpp_path, 'r', encoding='utf-8') as f:
    main_code = f.read()

if 'Player.h' not in main_code:
    main_code = main_code.replace('#include "DxLib.h"', '#include "DxLib.h"\n#include "ObjectManager.h"\n#include "Player.h"\n#include <memory>')
    main_code = main_code.replace('SetDrawScreen(DX_SCREEN_BACK);', 'SetDrawScreen(DX_SCREEN_BACK);\n\n    ObjectManager objManager;\n    objManager.AddObject(std::make_shared<Player>(320.0f, 240.0f));')
    main_code = main_code.replace('// ゲームの更新・描画処理', '// ゲームの更新・描画処理\n        objManager.Update();')
    main_code = main_code.replace('DrawString(100, 100, "DXライブラリ 初期化成功！", GetColor(255, 255, 255));', 'DrawString(10, 10, "矢印キーで移動", GetColor(255, 255, 255));\n        objManager.Draw();')
    with open(main_cpp_path, 'w', encoding='utf-8') as f:
        f.write(main_code)

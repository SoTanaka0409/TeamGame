import os
import xml.etree.ElementTree as ET

DIR = r'C:\Users\student\Desktop\TeamGame\TeamGame\TeamGame'

files = {
    'InputManager.h': '''#pragma once

class InputManager
{
private:
    char currentKeys[256];
    char previousKeys[256];

    int currentMouse;
    int previousMouse;

    InputManager();
    ~InputManager();

public:
    static InputManager& GetInstance()
    {
        static InputManager instance;
        return instance;
    }

    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

    void Update();

    // キーボード用 (keyCode には KEY_INPUT_... を指定)
    bool IsKeyHeld(int keyCode) const;     // 連続で返す（押しっぱなし）
    bool IsKeyPressed(int keyCode) const;  // 1回だけ返す（押した瞬間）
    bool IsKeyReleased(int keyCode) const; // 離した瞬間

    // マウス用 (button には MOUSE_INPUT_LEFT 等を指定)
    bool IsMouseHeld(int button) const;     // 連続で返す（押しっぱなし）
    bool IsMousePressed(int button) const;  // 1回だけ返す（押した瞬間）
    bool IsMouseReleased(int button) const; // 離した瞬間
};
''',

    'InputManager.cpp': '''#include "InputManager.h"
#include "DxLib.h"
#include <cstring>

InputManager::InputManager() : currentMouse(0), previousMouse(0)
{
    std::memset(currentKeys, 0, sizeof(currentKeys));
    std::memset(previousKeys, 0, sizeof(previousKeys));
}

InputManager::~InputManager()
{
}

void InputManager::Update()
{
    // 前回フレームの状態を保存
    std::memcpy(previousKeys, currentKeys, sizeof(currentKeys));
    previousMouse = currentMouse;

    // 現在のフレームの状態を取得
    GetHitKeyStateAll(currentKeys);
    currentMouse = GetMouseInput();
}

bool InputManager::IsKeyHeld(int keyCode) const
{
    return currentKeys[keyCode] != 0;
}

bool InputManager::IsKeyPressed(int keyCode) const
{
    return currentKeys[keyCode] != 0 && previousKeys[keyCode] == 0;
}

bool InputManager::IsKeyReleased(int keyCode) const
{
    return currentKeys[keyCode] == 0 && previousKeys[keyCode] != 0;
}

bool InputManager::IsMouseHeld(int button) const
{
    return (currentMouse & button) != 0;
}

bool InputManager::IsMousePressed(int button) const
{
    return (currentMouse & button) != 0 && (previousMouse & button) == 0;
}

bool InputManager::IsMouseReleased(int button) const
{
    return (currentMouse & button) == 0 && (previousMouse & button) != 0;
}
'''
}

for name, content in files.items():
    with open(os.path.join(DIR, name), 'w', encoding='utf-8') as f:
        f.write(content)

# Update vcxproj
proj = os.path.join(DIR, 'TeamGame.vcxproj')
filters = os.path.join(DIR, 'TeamGame.vcxproj.filters')

ET.register_namespace('', 'http://schemas.microsoft.com/developer/msbuild/2003')
ns = {'ms': 'http://schemas.microsoft.com/developer/msbuild/2003'}

# vcxproj
tree = ET.parse(proj)
root = tree.getroot()
compile_group = None
include_group = None
for ig in root.findall('ms:ItemGroup', ns):
    if ig.find('ms:ClCompile', ns) is not None:
        compile_group = ig
    if ig.find('ms:ClInclude', ns) is not None:
        include_group = ig

cpp_files = ['InputManager.cpp']
h_files = ['InputManager.h']

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

# filters
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

# Update main.cpp to call InputManager::GetInstance().Update()
main_path = os.path.join(DIR, 'main.cpp')
with open(main_path, 'r', encoding='utf-8') as f:
    main_code = f.read()

if 'InputManager.h' not in main_code:
    main_code = main_code.replace('#include "TitleScene.h"', '#include "TitleScene.h"\n#include "InputManager.h"')
    main_code = main_code.replace('// シーンの更新と描画', '// 入力の更新\n        InputManager::GetInstance().Update();\n\n        // シーンの更新と描画')
    with open(main_path, 'w', encoding='utf-8') as f:
        f.write(main_code)

# Replace CheckHitKey with InputManager calls in GameScene and TitleScene
scenes = ['GameScene.cpp', 'TitleScene.cpp', 'ResultScene.cpp', 'Player.cpp']
for scene in scenes:
    path = os.path.join(DIR, scene)
    with open(path, 'r', encoding='utf-8') as f:
        code = f.read()
    
    if '#include "InputManager.h"' not in code:
        code = code.replace('#include "DxLib.h"', '#include "DxLib.h"\n#include "InputManager.h"')
    
    # Replace CheckHitKey(...) == 1 with IsKeyHeld
    code = code.replace('CheckHitKey(KEY_INPUT_LEFT) == 1', 'InputManager::GetInstance().IsKeyHeld(KEY_INPUT_LEFT)')
    code = code.replace('CheckHitKey(KEY_INPUT_RIGHT) == 1', 'InputManager::GetInstance().IsKeyHeld(KEY_INPUT_RIGHT)')
    code = code.replace('CheckHitKey(KEY_INPUT_UP) == 1', 'InputManager::GetInstance().IsKeyHeld(KEY_INPUT_UP)')
    code = code.replace('CheckHitKey(KEY_INPUT_DOWN) == 1', 'InputManager::GetInstance().IsKeyHeld(KEY_INPUT_DOWN)')
    
    # Replace CheckHitKey for transitions with IsKeyPressed (one time trigger)
    code = code.replace('CheckHitKey(KEY_INPUT_SPACE)', 'InputManager::GetInstance().IsKeyPressed(KEY_INPUT_SPACE)')
    code = code.replace('CheckHitKey(KEY_INPUT_RETURN)', 'InputManager::GetInstance().IsKeyPressed(KEY_INPUT_RETURN)')
    
    with open(path, 'w', encoding='utf-8') as f:
        f.write(code)

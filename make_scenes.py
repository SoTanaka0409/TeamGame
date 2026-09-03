import os
import xml.etree.ElementTree as ET

DIR = r'C:\Users\student\Desktop\TeamGame\TeamGame\TeamGame'

files = {
    'SceneManager.h': '''#pragma once
#include "BaseScene.h"
#include <memory>

class SceneManager
{
private:
    std::shared_ptr<BaseScene> currentScene;
    std::shared_ptr<BaseScene> nextScene;

    SceneManager();
    ~SceneManager();

public:
    static SceneManager& GetInstance()
    {
        static SceneManager instance;
        return instance;
    }
    
    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;

    void ChangeScene(std::shared_ptr<BaseScene> newScene);
    void Update();
    void Draw();
};
''',

    'SceneManager.cpp': '''#include "SceneManager.h"

SceneManager::SceneManager() : currentScene(nullptr), nextScene(nullptr)
{
}

SceneManager::~SceneManager()
{
}

void SceneManager::ChangeScene(std::shared_ptr<BaseScene> newScene)
{
    nextScene = newScene;
}

void SceneManager::Update()
{
    if (nextScene)
    {
        if (currentScene)
        {
            currentScene->Finalize();
        }
        currentScene = nextScene;
        currentScene->Init();
        nextScene = nullptr;
    }
    if (currentScene)
    {
        currentScene->Update();
    }
}

void SceneManager::Draw()
{
    if (currentScene)
    {
        currentScene->Draw();
    }
}
''',

    'TitleScene.h': '''#pragma once
#include "BaseScene.h"

class TitleScene : public BaseScene
{
public:
    TitleScene();
    ~TitleScene() override;

    void Init() override;
    void Update() override;
    void Draw() override;
    void Finalize() override;
};
''',

    'TitleScene.cpp': '''#include "TitleScene.h"
#include "GameScene.h"
#include "SceneManager.h"
#include "DxLib.h"

TitleScene::TitleScene() {}
TitleScene::~TitleScene() {}

void TitleScene::Init()
{
}

void TitleScene::Update()
{
    // ENTERキーでゲームシーンへ
    if (CheckHitKey(KEY_INPUT_RETURN))
    {
        SceneManager::GetInstance().ChangeScene(std::make_shared<GameScene>());
    }
}

void TitleScene::Draw()
{
    DrawString(100, 100, "=== タイトル画面 ===", GetColor(255, 255, 255));
    DrawString(100, 150, "Enterキーを押してスタート", GetColor(200, 200, 200));
}

void TitleScene::Finalize()
{
}
''',

    'GameScene.h': '''#pragma once
#include "BaseScene.h"

class GameScene : public BaseScene
{
public:
    GameScene();
    ~GameScene() override;

    void Init() override;
    void Update() override;
    void Draw() override;
    void Finalize() override;
};
''',

    'GameScene.cpp': '''#include "GameScene.h"
#include "ResultScene.h"
#include "SceneManager.h"
#include "ObjectManager.h"
#include "Player.h"
#include "DxLib.h"

GameScene::GameScene() {}
GameScene::~GameScene() {}

void GameScene::Init()
{
    ObjectManager::GetInstance().Clear();
    new Player(320.0f, 240.0f);
}

void GameScene::Update()
{
    ObjectManager::GetInstance().Update();

    // SPACEキーでリザルトへ
    if (CheckHitKey(KEY_INPUT_SPACE))
    {
        SceneManager::GetInstance().ChangeScene(std::make_shared<ResultScene>());
    }
}

void GameScene::Draw()
{
    ObjectManager::GetInstance().Draw();
    DrawString(10, 10, "=== ゲーム画面 === (Spaceキーでリザルトへ)", GetColor(255, 255, 255));
}

void GameScene::Finalize()
{
    ObjectManager::GetInstance().Clear();
}
''',

    'ResultScene.h': '''#pragma once
#include "BaseScene.h"

class ResultScene : public BaseScene
{
public:
    ResultScene();
    ~ResultScene() override;

    void Init() override;
    void Update() override;
    void Draw() override;
    void Finalize() override;
};
''',

    'ResultScene.cpp': '''#include "ResultScene.h"
#include "TitleScene.h"
#include "SceneManager.h"
#include "DxLib.h"

ResultScene::ResultScene() {}
ResultScene::~ResultScene() {}

void ResultScene::Init()
{
}

void ResultScene::Update()
{
    // ENTERキーでタイトルへ
    if (CheckHitKey(KEY_INPUT_RETURN))
    {
        SceneManager::GetInstance().ChangeScene(std::make_shared<TitleScene>());
    }
}

void ResultScene::Draw()
{
    DrawString(100, 100, "=== リザルト画面 ===", GetColor(255, 255, 255));
    DrawString(100, 150, "Enterキーを押してタイトルに戻る", GetColor(200, 200, 200));
}

void ResultScene::Finalize()
{
}
'''
}

for name, content in files.items():
    with open(os.path.join(DIR, name), 'w', encoding='utf-8') as f:
        f.write(content)

# Update main.cpp
main_path = os.path.join(DIR, 'main.cpp')
main_code = '''#include "DxLib.h"
#include "SceneManager.h"
#include "TitleScene.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // デスクトップの解像度を取得してフルスクリーンに近い形にする
    // GetSystemMetrics は Windows API
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // ウィンドウモードで起動設定
    ChangeWindowMode(TRUE);
    
    // ウィンドウサイズを画面サイズに合わせる
    SetGraphMode(screenWidth, screenHeight, 32);

    // ウィンドウタイトルの設定
    SetMainWindowText("TeamGame");

    // DXライブラリの文字列処理をUTF-8に設定
    SetUseCharCodeFormat(DX_CHARCODEFORMAT_UTF8);

    // DXライブラリの初期化
    if (DxLib_Init() == -1)
    {
        return -1; // エラーが起きたら直ちに終了
    }

    // 描画先を裏画面に設定
    SetDrawScreen(DX_SCREEN_BACK);

    // 最初のシーンをタイトルに設定
    SceneManager::GetInstance().ChangeScene(std::make_shared<TitleScene>());

    // メインループ
    while (ProcessMessage() == 0 && ClearDrawScreen() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0)
    {
        // シーンの更新と描画
        SceneManager::GetInstance().Update();
        SceneManager::GetInstance().Draw();

        // 裏画面の内容を表画面に反映
        ScreenFlip();
    }

    // DXライブラリの使用終了処理
    DxLib_End();

    return 0;
}
'''

with open(main_path, 'w', encoding='utf-8') as f:
    f.write(main_code)

# Add new files to vcxproj and filters
proj = os.path.join(DIR, 'TeamGame.vcxproj')
filters = os.path.join(DIR, 'TeamGame.vcxproj.filters')

ET.register_namespace('', 'http://schemas.microsoft.com/developer/msbuild/2003')

# vcxproj
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

cpp_files = ['TitleScene.cpp', 'GameScene.cpp', 'ResultScene.cpp']
h_files = ['TitleScene.h', 'GameScene.h', 'ResultScene.h']

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

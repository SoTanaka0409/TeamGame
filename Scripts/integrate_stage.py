import os
import re

gs_h = r'C:\Users\student\Desktop\TeamGame\TeamGame\TeamGame\Source\Scenes\GameScene.h'
with open(gs_h, 'r', encoding='utf-8') as f:
    h_code = f.read()

if '#include "Stage.h"' not in h_code:
    h_code = h_code.replace('#include "Scene.h"', '#include "Scene.h"\n#include "Stage.h"')
    
if 'Stage stage;' not in h_code:
    h_code = h_code.replace('class Player *player;', 'class Player *player;\n    Stage stage;\n    int themeIdx;\n    int varIdx;')

with open(gs_h, 'w', encoding='utf-8') as f:
    f.write(h_code)

gs_cpp = r'C:\Users\student\Desktop\TeamGame\TeamGame\TeamGame\Source\Scenes\GameScene.cpp'
with open(gs_cpp, 'r', encoding='utf-8') as f:
    cpp_code = f.read()

new_init = '''#include "StageGenerator.h"
#include <random>
#include <ctime>

void GameScene::Init()
{
    Scene::Init();
    
    // ステージ生成
    std::random_device rd;
    unsigned int seed = rd() ^ static_cast<unsigned int>(std::time(nullptr));
    StageConfig config;
    themeIdx = seed % 4;
    varIdx = seed % 4;
    config.theme = static_cast<ThemePattern>(themeIdx);
    config.variation = varIdx;
    
    ThemePattern currTheme = config.theme;
    int currVar = config.variation;
    
    stage = StageGenerator::Generate(config, seed, &currTheme, &currVar);
    
    // セルサイズ計算 (1920x1080画面に合わせる)
    float cellW = 1920.0f / stage.GetWidth();
    float cellH = 1080.0f / stage.GetHeight();
    float cellSize = (cellW < cellH) ? cellW : cellH;
    
    Point2D startGrid = stage.GetPlayerStartPos();
    float startX = (startGrid.x + 0.5f) * cellSize;
    float startY = (startGrid.y + 0.5f) * cellSize;
    
    player = new Player(startX, startY);
    
    // 敵のスポーン位置も追加できるが、とりあえず固定位置に1体
    new Enemy(startX + 200.0f, startY + 200.0f);
}'''

cpp_code = re.sub(r'void GameScene::Init\(\)[\s\S]*?\}', new_init, cpp_code)

new_draw_top = '''void GameScene::Draw()
{
    float cellW = 1920.0f / stage.GetWidth();
    float cellH = 1080.0f / stage.GetHeight();
    float cellSize = (cellW < cellH) ? cellW : cellH;
    
    float playerGridX = 0;
    float playerGridY = 0;
    float facingAngle = 0;
    
    if (player && player->IsActive())
    {
        Vector2 pos = player->GetPosition();
        Vector2 dir = player->GetFacingDir();
        playerGridX = pos.x / cellSize;
        playerGridY = pos.y / cellSize;
        facingAngle = std::atan2(dir.y, dir.x);
    }
    
    // ステージ描画
    std::string stageName = StageGenerator::GetFullStageName(static_cast<ThemePattern>(themeIdx), varIdx);
    stage.DrawFitToArea(0, 0, 1920, 1080, true, playerGridX, playerGridY, facingAngle, stageName.c_str(), -1);

    Scene::Draw();'''

# Replace the beginning of Draw() up to Scene::Draw();
cpp_code = re.sub(r'void GameScene::Draw\(\)[\s\S]*?Scene::Draw\(\);', new_draw_top, cpp_code)

with open(gs_cpp, 'w', encoding='utf-8') as f:
    f.write(cpp_code)

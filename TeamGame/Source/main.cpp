#include "DxLib.h"
#include "StageGenerator.h"
#include <string>
#include <cmath>
#include <ctime>
#include <random>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // 画面設定：1920 x 1080 (横長16:9 フルHD画面)
    SetOutApplicationLogValidFlag(FALSE);
    ChangeWindowMode(TRUE);

    const int SCREEN_WIDTH = 1920;
    const int SCREEN_HEIGHT = 1080;
    SetGraphMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32);
    SetMainWindowText("2D Horror Shooter - 4 Themes x 4 Stage Variations (16 Unique Stages)");

    SetUseCharCodeFormat(DX_CHARCODEFORMAT_UTF8);
    if (DxLib_Init() == -1)
    {
        return -1;
    }

    SetDrawScreen(DX_SCREEN_BACK);

    // 【Grass1.png 画像の読み込み】
    int hGrass = LoadGraph("Resource/Grass1.png");
    if (hGrass == -1) hGrass = LoadGraph("Resouce/Grass1.png");
    if (hGrass == -1) hGrass = LoadGraph("Grass1.png");

    // 1920x1080 横長アリーナ設定 (48 x 27 セル)
    StageGenConfig config;
    config.mapWidth = 48;
    config.mapHeight = 27;

    // 起動ごとのランダムシード値
    std::random_device rd;
    unsigned int seed = rd() ^ static_cast<unsigned int>(std::time(nullptr));
    std::mt19937 initRng(seed);

    // 初期テーマ (0〜3) と バリエーション (0〜3)
    int themeIdx = initRng() % 4;
    int varIdx = initRng() % 4;

    config.theme = static_cast<ThemePattern>(themeIdx);
    config.variation = varIdx;

    ThemePattern currTheme = config.theme;
    int currVariation = config.variation;

    Stage stage = StageGenerator::Generate(config, seed, &currTheme, &currVariation);
    if (hGrass != -1) stage.SetGrassGraph(hGrass);

    // プレイヤー P のデバッグ移動用座標
    Point2D startGrid = stage.GetPlayerStartPos();
    float playerX = startGrid.x + 0.5f;
    float playerY = startGrid.y + 0.5f;

    // デバッグ表示フラグ (デフォルトON)
    bool isDebugView = true;

    bool prevTabState = false;
    bool prevF1State = false;
    bool prevRState = false;
    bool prevTState = false;

    // メインループ
    while (ProcessMessage() == 0 && ClearDrawScreen() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0)
    {
        // ----------------------------------------------------
        // 1. デバッグプレイヤー P の移動処理 & 壁・草むら衝突判定
        // ----------------------------------------------------
        float moveSpeed = 0.20f;
        float moveX = 0.0f;
        float moveY = 0.0f;

        if (CheckHitKey(KEY_INPUT_LEFT) || CheckHitKey(KEY_INPUT_A))  moveX -= moveSpeed;
        if (CheckHitKey(KEY_INPUT_RIGHT) || CheckHitKey(KEY_INPUT_D)) moveX += moveSpeed;
        if (CheckHitKey(KEY_INPUT_UP) || CheckHitKey(KEY_INPUT_W))    moveY -= moveSpeed;
        if (CheckHitKey(KEY_INPUT_DOWN) || CheckHitKey(KEY_INPUT_S))  moveY += moveSpeed;

        if (moveX != 0.0f && moveY != 0.0f)
        {
            moveX *= 0.7071f;
            moveY *= 0.7071f;
        }

        // X軸移動 (壁・外枠・草むら衝突判定)
        float newPlayerX = playerX + moveX;
        int checkGridX = static_cast<int>(newPlayerX);
        int checkGridY = static_cast<int>(playerY);
        if (!stage.IsSolidWall(checkGridX, checkGridY))
        {
            playerX = newPlayerX;
        }

        // Y軸移動 (壁・外枠・草むら衝突判定)
        float newPlayerY = playerY + moveY;
        checkGridX = static_cast<int>(playerX);
        checkGridY = static_cast<int>(newPlayerY);
        if (!stage.IsSolidWall(checkGridX, checkGridY))
        {
            playerY = newPlayerY;
        }

        // ----------------------------------------------------
        // 2. マウス照準・前方向ライト角度の計算
        // ----------------------------------------------------
        int mouseX = 0, mouseY = 0;
        GetMousePoint(&mouseX, &mouseY);

        float cellW = static_cast<float>(SCREEN_WIDTH) / stage.GetWidth();
        float cellH = static_cast<float>(SCREEN_HEIGHT) / stage.GetHeight();
        float cellSize = (cellW < cellH) ? cellW : cellH;

        float playerPx = playerX * cellSize;
        float playerPy = playerY * cellSize;

        float lightAngle = std::atan2(mouseY - playerPy, mouseX - playerPx);

        // ----------------------------------------------------
        // 3. キー入力判定
        // ----------------------------------------------------
        bool currTabState = (CheckHitKey(KEY_INPUT_TAB) != 0);
        bool currF1State = (CheckHitKey(KEY_INPUT_F1) != 0);
        bool currRState = (CheckHitKey(KEY_INPUT_R) != 0);
        bool currTState = (CheckHitKey(KEY_INPUT_T) != 0);

        if ((currTabState && !prevTabState) || (currF1State && !prevF1State))
        {
            isDebugView = !isDebugView;
        }

        // 【Rキー: 同じテーマ内で別のステージバリエーション(1〜4)に切替】
        if (currRState && !prevRState)
        {
            varIdx = (varIdx + 1) % 4;
            config.variation = varIdx;
            seed = rd() ^ static_cast<unsigned int>(std::time(nullptr));
            stage = StageGenerator::Generate(config, seed, &currTheme, &currVariation);
            if (hGrass != -1) stage.SetGrassGraph(hGrass);
            startGrid = stage.GetPlayerStartPos();
            playerX = startGrid.x + 0.5f;
            playerY = startGrid.y + 0.5f;
        }

        // 【Tキー: テーマデザイン(1〜4)を次のテーマに切替】
        if (currTState && !prevTState)
        {
            themeIdx = (themeIdx + 1) % 4;
            varIdx = 0;
            config.theme = static_cast<ThemePattern>(themeIdx);
            config.variation = varIdx;
            seed = rd() ^ static_cast<unsigned int>(std::time(nullptr));
            stage = StageGenerator::Generate(config, seed, &currTheme, &currVariation);
            if (hGrass != -1) stage.SetGrassGraph(hGrass);
            startGrid = stage.GetPlayerStartPos();
            playerX = startGrid.x + 0.5f;
            playerY = startGrid.y + 0.5f;
        }

        prevTabState = currTabState;
        prevF1State = currF1State;
        prevRState = currRState;
        prevTState = currTState;

        // ----------------------------------------------------
        // 4. 1920×1080 画面全域（スクロールなし）描画
        // ----------------------------------------------------
        std::string fullStageName = StageGenerator::GetFullStageName(currTheme, currVariation);
        stage.DrawFitToArea(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, isDebugView, playerX, playerY, lightAngle, fullStageName.c_str(), hGrass);

        // ----------------------------------------------------
        // 5. スマート・オーバーレイHUD
        // ----------------------------------------------------
        // 左上：操作ガイド
        DrawBox(12, 12, 960, 80, GetColor(15, 20, 30), TRUE);
        DrawBox(12, 12, 960, 80, GetColor(0, 180, 240), FALSE);

        DrawString(22, 18, "[WASD/矢印]:移動  [R]:同テーマ内ステージ切替(1~4)  [T]:テーマ切替(1~4)  [F1]:切替", GetColor(255, 255, 255));
        
        std::string modeText = fullStageName;
        if (isDebugView)
        {
            modeText += "  【 DEBUG VIEW (4テーマ x 4ステージ = 全16パターン) 】";
            DrawString(22, 48, modeText.c_str(), GetColor(50, 255, 140));
        }
        else
        {
            modeText += "  【 HORROR DARKNESS (4テーマ x 4ステージ = 全16パターン) 】";
            DrawString(22, 48, modeText.c_str(), GetColor(255, 100, 100));
        }

        // 右上：情報
        DrawBox(SCREEN_WIDTH - 500, 12, SCREEN_WIDTH - 12, 80, GetColor(15, 20, 30), TRUE);
        DrawBox(SCREEN_WIDTH - 500, 12, SCREEN_WIDTH - 12, 80, GetColor(0, 180, 240), FALSE);

        std::string posStr = "ステージ構成: 4デザインテーマ x 各4ステージ (計16アリーナ)";
        std::string infoStr = "現在: " + fullStageName + " | Seed: " + std::to_string(seed);
        
        DrawString(SCREEN_WIDTH - 485, 20, posStr.c_str(), GetColor(255, 240, 100));
        DrawString(SCREEN_WIDTH - 485, 48, infoStr.c_str(), GetColor(50, 255, 140));

        // 右下：凡例
        if (isDebugView)
        {
            int legW = 520;
            int legH = 115;
            int legX = SCREEN_WIDTH - legW - 12;
            int legY = SCREEN_HEIGHT - legH - 12;

            DrawBox(legX, legY, legX + legW, legY + legH, GetColor(15, 20, 30), TRUE);
            DrawBox(legX, legY, legX + legW, legY + legH, GetColor(100, 150, 200), FALSE);

            DrawString(legX + 15, legY + 12, "凡例 : [小型緑P]操作プレイヤー   [★]中央スター", GetColor(220, 220, 220));
            DrawString(legX + 15, legY + 38, "       [Grass1.png]草むら   [Rキー]同テーマでステージ変更(1~4)", GetColor(220, 220, 220));
            DrawString(legX + 15, legY + 64, "       [Tキー]テーマ変更(1~4)   [計16パターン完全対応]", GetColor(220, 220, 220));
        }

        ScreenFlip();
    }

    DxLib_End();
    return 0;
}

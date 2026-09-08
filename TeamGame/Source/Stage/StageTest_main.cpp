#if 0 // Conflict disabled
#include "DxLib.h"
#include "StageManager.h"
#include "Player.h"
#include <string>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // 画面設定：1920 x 1080 (横長16:9 フルHD画面)
    SetOutApplicationLogValidFlag(FALSE);
    ChangeWindowMode(TRUE);

    const int SCREEN_WIDTH = 1920;
    const int SCREEN_HEIGHT = 1080;
    SetGraphMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32);
    SetMainWindowText("2D Horror Shooter - Bush Stealth & Right-Click Light Toggle (1920x1080)");

    SetUseCharCodeFormat(DX_CHARCODEFORMAT_UTF8);
    if (DxLib_Init() == -1)
    {
        return -1;
    }

    SetDrawScreen(DX_SCREEN_BACK);

    // 【1. StageManager と Player の生成・初期化】
    StageManager stageManager;
    stageManager.Initialize(48, 27);

    Player player;
    Point2D startGrid = stageManager.GetCurrentStage().GetPlayerStartPos();
    player.Initialize(startGrid.x + 0.5f, startGrid.y + 0.5f);

    // 【ご要望: 最初から暗闇モードに設定】 (isDebugView = false)
    bool isDebugView = false;

    bool prevTabState = false;
    bool prevF1State = false;
    bool prevRState = false;
    bool prevTState = false;

    // メインループ
    while (ProcessMessage() == 0 && ClearDrawScreen() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0)
    {
        const Stage& currStage = stageManager.GetCurrentStage();
        float cellW = static_cast<float>(SCREEN_WIDTH) / currStage.GetWidth();
        float cellH = static_cast<float>(SCREEN_HEIGHT) / currStage.GetHeight();
        float cellSize = (cellW < cellH) ? cellW : cellH;

        float mapPixelWidth = currStage.GetWidth() * cellSize;
        float mapPixelHeight = currStage.GetHeight() * cellSize;
        float startDrawX = (SCREEN_WIDTH - mapPixelWidth) / 2.0f;
        float startDrawY = (SCREEN_HEIGHT - mapPixelHeight) / 2.0f;

        // ----------------------------------------------------
        // 2. Player の更新 (移動・右クリックライトON/OFF・草むら隠れ・照準計算)
        // ----------------------------------------------------
        player.Update(currStage, SCREEN_WIDTH, SCREEN_HEIGHT, startDrawX, startDrawY, cellSize);

        // ----------------------------------------------------
        // 3. キー入力判定 (StageManager の制御)
        // ----------------------------------------------------
        bool currTabState = (CheckHitKey(KEY_INPUT_TAB) != 0);
        bool currF1State = (CheckHitKey(KEY_INPUT_F1) != 0);
        bool currRState = (CheckHitKey(KEY_INPUT_R) != 0);
        bool currTState = (CheckHitKey(KEY_INPUT_T) != 0);

        if ((currTabState && !prevTabState) || (currF1State && !prevF1State))
        {
            isDebugView = !isDebugView;
        }

        // 【Rキー: 同テーマ内で別のステージバリエーション(1〜4)に切替】
        if (currRState && !prevRState)
        {
            stageManager.NextVariation();
            Point2D newStart = stageManager.GetCurrentStage().GetPlayerStartPos();
            player.Initialize(newStart.x + 0.5f, newStart.y + 0.5f);
        }

        // 【Tキー: テーマデザイン(1〜4)を次のテーマに切替】
        if (currTState && !prevTState)
        {
            stageManager.NextTheme();
            Point2D newStart = stageManager.GetCurrentStage().GetPlayerStartPos();
            player.Initialize(newStart.x + 0.5f, newStart.y + 0.5f);
        }

        prevTabState = currTabState;
        prevF1State = currF1State;
        prevRState = currRState;
        prevTState = currTState;

        // ----------------------------------------------------
        // 4. 統合描画 (StageManager::Draw で描画)
        // ----------------------------------------------------
        stageManager.Draw(player, isDebugView, SCREEN_WIDTH, SCREEN_HEIGHT);

        // ----------------------------------------------------
        // 5. スマート・オーバーレイHUD
        // ----------------------------------------------------
        std::string fullStageName = stageManager.GetCurrentStageName();

        // 左上：操作ガイド
        DrawBox(12, 12, 1010, 80, GetColor(15, 20, 30), TRUE);
        DrawBox(12, 12, 1010, 80, GetColor(0, 180, 240), FALSE);

        DrawString(22, 18, "[WASD]:移動  [右クリック]:ライトON/OFF  [R]:ステージ切替(1~4)  [T]:テーマ切替(1~4)  [F1]:透視", GetColor(255, 255, 255));
        
        std::string modeText = fullStageName;
        if (isDebugView)
        {
            modeText += "  【 DEBUG VIEW (全透視デバッグ表示) 】";
            DrawString(22, 48, modeText.c_str(), GetColor(50, 255, 140));
        }
        else
        {
            modeText += "  【 HORROR DARKNESS MODE (本格ホラー暗闇) 】";
            DrawString(22, 48, modeText.c_str(), GetColor(255, 100, 100));
        }

        // 右上：情報
        DrawBox(SCREEN_WIDTH - 550, 12, SCREEN_WIDTH - 12, 80, GetColor(15, 20, 30), TRUE);
        DrawBox(SCREEN_WIDTH - 550, 12, SCREEN_WIDTH - 12, 80, GetColor(0, 180, 240), FALSE);

        std::string lightStateStr = std::string("ライト状態: ") + (player.IsLightOn() ? "【点灯 (ON)】" : "【消灯 (OFF)】");
        if (player.IsInBush()) lightStateStr += " | 【草むら隠れ中 (STEALTH)】";

        std::string infoStr = "現在: " + fullStageName + " | Seed: " + std::to_string(stageManager.GetCurrentSeed());
        
        DrawString(SCREEN_WIDTH - 535, 20, lightStateStr.c_str(), player.IsInBush() ? GetColor(50, 255, 140) : GetColor(255, 240, 100));
        DrawString(SCREEN_WIDTH - 535, 48, infoStr.c_str(), GetColor(200, 220, 255));

        // 右下：凡例
        if (isDebugView)
        {
            int legW = 530;
            int legH = 115;
            int legX = SCREEN_WIDTH - legW - 12;
            int legY = SCREEN_HEIGHT - legH - 12;

            DrawBox(legX, legY, legX + legW, legY + legH, GetColor(15, 20, 30), TRUE);
            DrawBox(legX, legY, legX + legW, legY + legH, GetColor(100, 150, 200), FALSE);

            DrawString(legX + 15, legY + 12, "凡例 : [草むら(Grass1)]進入可能・隠れられる・光透過", GetColor(220, 220, 220));
            DrawString(legX + 15, legY + 38, "       [水場]光透過   [壁・木箱・サボテン]光を遮断し影を生成", GetColor(220, 220, 220));
            DrawString(legX + 15, legY + 64, "       [右クリック]ライトON/OFF切替   [初期状態]暗闇スタート", GetColor(220, 220, 220));
        }

        ScreenFlip();
    }

    DxLib_End();
    return 0;
}

#endif

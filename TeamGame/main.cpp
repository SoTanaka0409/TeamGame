#include "DxLib.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // ウィンドウモードで起動設定
    ChangeWindowMode(TRUE);
    
    // ウィンドウタイトルの設定
    SetMainWindowText("TeamGame");

    // DXライブラリの初期化
    if (DxLib_Init() == -1)
    {
        return -1; // エラーが起きたら直ちに終了
    }
    
    // 描画先を裏画面に設定
    SetDrawScreen(DX_SCREEN_BACK);

    // メインループ
    while (ProcessMessage() == 0 && ClearDrawScreen() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0)
    {
        // ==========================================
        // ゲームの更新・描画処理
        // ==========================================
        
        DrawString(100, 100, "DXライブラリ 初期化成功！", GetColor(255, 255, 255));
        
        // 裏画面の内容を表画面に反映
        ScreenFlip();
    }

    // DXライブラリの使用終了処理
    DxLib_End();
    
    return 0;
}

#include "DxLib.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "WeaponManager.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
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

    // 武器データのロード
    WeaponManager::GetInstance().LoadFromCSV("weapons.csv");

    // 描画先を裏画面に設定
    SetDrawScreen(DX_SCREEN_BACK);

    // 最初のシーンをタイトルに設定
    SceneManager::GetInstance().ChangeScene(std::make_shared<TitleScene>());

    // メインループ
    // ESCによる強制終了を削除し、ポーズ画面から終了させる
    while (ProcessMessage() == 0 && ClearDrawScreen() == 0)
    {
        // 入力の更新
        InputManager::GetInstance().Update();

        // シーンの更新と描画
        SceneManager::GetInstance().Update();
        SceneManager::GetInstance().Draw();

        // 裏画面の内容を表画面に反映
        ScreenFlip();
    }

    WeaponManager::GetInstance().Cleanup();

    // DXライブラリの使用終了処理
    DxLib_End();

    return 0;
}

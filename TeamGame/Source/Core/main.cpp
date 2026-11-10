#include "DxLib.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "WeaponManager.h"
#include "CharacterManager.h"
#include "SoundManager.h"
#include "GameBalanceManager.h"
#include "ItemManager.h"
#include "DebugManager.h"
#include "../Skills/SkillData.h"
#include <string>

/**
 * @brief .exeを直接開いて起動した場合でもResourceフォルダが見つかるようカレントディレクトリを自動設定する
 */
static void EnsureResourceWorkingDirectory()
{
    DWORD curAttr = GetFileAttributesW(L"Resource\\CSV\\characters.csv");
    if (curAttr == INVALID_FILE_ATTRIBUTES) curAttr = GetFileAttributesW(L"Resource\\characters.csv");
    if (curAttr != INVALID_FILE_ATTRIBUTES && !(curAttr & FILE_ATTRIBUTE_DIRECTORY))
    {
        return;
    }

    wchar_t exePath[MAX_PATH] = { 0 };
    if (GetModuleFileNameW(NULL, exePath, MAX_PATH) == 0) return;

    std::wstring exeDir(exePath);
    size_t lastSlash = exeDir.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos)
    {
        exeDir = exeDir.substr(0, lastSlash);
    }

    const wchar_t* candidates[] = {
        L"",
        L"\\TeamGame",
        L"\\..\\TeamGame",
        L"\\..\\..\\TeamGame",
        L"\\..",
        L"\\..\\.."
    };

    for (const wchar_t* rel : candidates)
    {
        std::wstring dir = exeDir + rel;
        std::wstring testFile = dir + L"\\Resource\\CSV\\characters.csv";
        DWORD attr = GetFileAttributesW(testFile.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES) {
            testFile = dir + L"\\Resource\\characters.csv";
            attr = GetFileAttributesW(testFile.c_str());
        }
        if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY))
        {
            SetCurrentDirectoryW(dir.c_str());
            return;
        }
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    // .exe直接起動時でもResourceフォルダを正しく参照できるようカレントディレクトリを調整
    EnsureResourceWorkingDirectory();

    // ウィンドウモードで起動設定
    ChangeWindowMode(TRUE);

    // 内部描画解像度は 1920x1080 (Full HD) を維持（全UIや3Dカメラの基準座標）
    SetGraphMode(1920, 1080, 32);

    // ウィンドウの表示サイズを 1280x720 に縮小スケーリングして表示
    SetWindowSizeChangeEnableFlag(TRUE, TRUE);
    SetWindowSize(1280, 720);

    // ウィンドウタイトルの設定
    SetMainWindowText("TeamGame");

    // DXライブラリの文字列処理をUTF-8に設定6
    SetUseCharCodeFormat(DX_CHARCODEFORMAT_UTF8);

    // DXライブラリの初期化
    if (DxLib_Init() == -1)
    {
        return -1; // エラーが起きたら直ちに終了
    }

    // 武器データのロード
    WeaponManager::GetInstance().LoadFromCSV("weapons.csv");

    // キャラクターデータのロード
    CharacterManager::GetInstance().LoadFromCSV("characters.csv");

    // ゲームバランスデータのロード
    GameBalanceManager::GetInstance().LoadFromCSV("game_balance.csv");

    // スキルデータのロード
    SkillDataManager::GetInstance().LoadFromCSV("skills.csv");

    // アイテムデータのロード
    ItemManager::GetInstance().LoadFromCSV("items.csv");

    // 描画先を裏画面に設定
    SetDrawScreen(DX_SCREEN_BACK);

    // 最初のシーンをタイトルに設定
    SceneManager::GetInstance().ChangeScene(std::make_shared<TitleScene>());

    // FPS測定および60FPS固定制御用の変数
    LONGLONG prevTime = GetNowHiPerformanceCount();
    LONGLONG fpsCalcTime = prevTime;
    int frameCount = 0;
    float currentFps = 60.0f;
    float frameTimeMs = 16.6f;
    bool showFps = false; // デバッグモード時または[F3]キーでのみ表示

    // メインループ
    // ESCによる強制終了を削除し、ポーズ画面から終了させる
    while (ProcessMessage() == 0 && ClearDrawScreen() == 0)
    {
        LONGLONG frameStart = GetNowHiPerformanceCount();

        // 入力の更新
        InputManager::GetInstance().Update();

        // [F3]キーでFPSメーター表示のON/OFF切り替え
        if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_F3))
        {
            showFps = !showFps;
        }

        // シーンの更新と描画
        SceneManager::GetInstance().Update();
        SceneManager::GetInstance().Draw();

        // リアルタイムFPSバッジの描画（デバッグモード時またはF3有効時のみ表示）
        if (showFps || DebugManager::GetInstance().IsDebugMode())
        {
            unsigned int fpsColor = (currentFps >= 55.0f) ? GetColor(80, 255, 120) :
                                    (currentFps >= 35.0f) ? GetColor(255, 200, 50) :
                                                            GetColor(255, 70, 70);

            // 右上のデバッグ情報パネル（x=1420〜1900）の直下に配置して他のUIと重ならないようにする
            int fpsX = 1420;
            int fpsY = 195;
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 190);
            DrawBox(fpsX, fpsY, fpsX + 220, fpsY + 34, GetColor(10, 15, 25), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            DrawBox(fpsX, fpsY, fpsX + 220, fpsY + 34, fpsColor, FALSE);
            DrawFormatString(fpsX + 12, fpsY + 8, fpsColor, "FPS: %5.1f (%4.1fms)", currentFps, frameTimeMs);
        }

        // 裏画面の内容を表画面に反映
        ScreenFlip();

        // 60FPS固定制御（16.66ms待機：高速モニターでの過剰描画やスタッターを防止）
        const LONGLONG targetFrameMicro = 1000000 / 60;
        while (GetNowHiPerformanceCount() - frameStart < targetFrameMicro)
        {
            Sleep(0);
        }

        // FPS・フレームタイムの計測更新（0.5秒おきに安定更新）
        LONGLONG now = GetNowHiPerformanceCount();
        frameCount++;
        if (now - fpsCalcTime >= 500000)
        {
            currentFps = (frameCount * 1000000.0f) / static_cast<float>(now - fpsCalcTime);
            frameTimeMs = static_cast<float>(now - fpsCalcTime) / (frameCount * 1000.0f);
            frameCount = 0;
            fpsCalcTime = now;
        }
    }

    WeaponManager::GetInstance().Cleanup();
    SoundManager::GetInstance().Clear();

    // DXライブラリの使用終了処理
    DxLib_End();

    return 0;
}

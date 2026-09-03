#include "DxLib.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    ChangeWindowMode(TRUE);
    SetMainWindowText("TeamGame");

    if (DxLib_Init() == -1) return -1;
    
    SetDrawScreen(DX_SCREEN_BACK);

    while (ProcessMessage() == 0 && ClearDrawScreen() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0)
    {
        DrawString(100, 100, "DXƒ‰ƒCƒuƒ‰ƒŠ ‰Šú‰»¬Œ÷I", GetColor(255, 255, 255));
        ScreenFlip();
    }

    DxLib_End();
    return 0;
}

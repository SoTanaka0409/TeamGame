import os
import xml.etree.ElementTree as ET

content = '''#include "DxLib.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    ChangeWindowMode(TRUE);
    SetMainWindowText("TeamGame");

    if (DxLib_Init() == -1) return -1;
    
    SetDrawScreen(DX_SCREEN_BACK);

    while (ProcessMessage() == 0 && ClearDrawScreen() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0)
    {
        DrawString(100, 100, "DXライブラリ 初期化成功！", GetColor(255, 255, 255));
        ScreenFlip();
    }

    DxLib_End();
    return 0;
}
'''
with open(r'C:\Users\student\Desktop\TeamGame\TeamGame\TeamGame\main.cpp', 'w', encoding='cp932') as f:
    f.write(content)

proj = r'C:\Users\student\Desktop\TeamGame\TeamGame\TeamGame\TeamGame.vcxproj'
ET.register_namespace('', 'http://schemas.microsoft.com/developer/msbuild/2003')
tree = ET.parse(proj)
root = tree.getroot()
ns = {'ms': 'http://schemas.microsoft.com/developer/msbuild/2003'}
for cl in root.findall('.//ms:ClCompile', ns):
    opts = cl.find('ms:AdditionalOptions', ns)
    if opts is not None and opts.text and '/utf-8' in opts.text:
        opts.text = opts.text.replace('/utf-8 ', '').replace('/utf-8', '')

tree.write(proj, encoding='utf-8', xml_declaration=True)

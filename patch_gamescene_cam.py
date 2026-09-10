import re

with open('TeamGame/Source/Scenes/GameScene.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

content = '#include "Camera.h"\n' + content

draw_logic_old = '''    // 1. プレイヤーの位置(playerWorldX, playerWorldY)を 1920x1080 画面の中央 (960, 540) に配置するようズームカメラ描画
    stage.DrawZoomCamera(playerWorldX, playerWorldY, zoomCellSize, worldCellSize, isDebug, stageName.c_str(), -1);'''
draw_logic_new = '''    // カメラの設定
    Camera::TargetWorldX = playerWorldX;
    Camera::TargetWorldY = playerWorldY;
    Camera::ScreenCenterX = 1920.0f / 2.0f;
    Camera::ScreenCenterY = 1080.0f / 2.0f;
    Camera::ZoomScale = zoomCellSize / worldCellSize;

    stage.DrawZoomCamera(playerWorldX, playerWorldY, zoomCellSize, worldCellSize, isDebug, stageName.c_str(), -1);'''

content = content.replace(draw_logic_old, draw_logic_new)

with open('TeamGame/Source/Scenes/GameScene.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

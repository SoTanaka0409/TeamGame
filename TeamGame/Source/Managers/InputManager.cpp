#include "InputManager.h"
#include "DxLib.h"
#include <cstring>

InputManager::InputManager() : currentMouse(0), previousMouse(0)
{
    std::memset(currentKeys, 0, sizeof(currentKeys));
    std::memset(previousKeys, 0, sizeof(previousKeys));
}

InputManager::~InputManager()
{
}

void InputManager::Update()
{
    // 前回フレームの状態を保存
    std::memcpy(previousKeys, currentKeys, sizeof(currentKeys));
    previousMouse = currentMouse;

    // 現在のフレームの状態を取得
    GetHitKeyStateAll(currentKeys);
    currentMouse = GetMouseInput();
}

bool InputManager::IsKeyHeld(int keyCode) const
{
    return currentKeys[keyCode] != 0;
}

bool InputManager::IsKeyPressed(int keyCode) const
{
    return currentKeys[keyCode] != 0 && previousKeys[keyCode] == 0;
}

bool InputManager::IsKeyReleased(int keyCode) const
{
    return currentKeys[keyCode] == 0 && previousKeys[keyCode] != 0;
}

bool InputManager::IsMouseHeld(int button) const
{
    return (currentMouse & button) != 0;
}

bool InputManager::IsMousePressed(int button) const
{
    return (currentMouse & button) != 0 && (previousMouse & button) == 0;
}

bool InputManager::IsMouseReleased(int button) const
{
    return (currentMouse & button) == 0 && (previousMouse & button) != 0;
}

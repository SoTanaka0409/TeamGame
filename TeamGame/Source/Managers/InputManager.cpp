#include "InputManager.h"
#include "DxLib.h"
#include <cstring>

InputManager::InputManager()
    : currentMouse(0), previousMouse(0),
      currentPad(0), previousPad(0),
      currentStickDir(0), previousStickDir(0)
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
    previousPad = currentPad;
    previousStickDir = currentStickDir;

    // 現在のフレームの状態を取得
    GetHitKeyStateAll(currentKeys);
    currentMouse = GetMouseInput();
    currentPad = GetJoypadInputState(DX_INPUT_PAD1);

    // 左スティックを4方向デジタル入力に変換（デッドゾーン: 500 / 1000）
    int ax = 0, ay = 0;
    GetJoypadAnalogInput(&ax, &ay, DX_INPUT_PAD1);
    currentStickDir = 0;
    if (ay < -500) currentStickDir |= PAD_INPUT_UP;
    if (ay > 500)  currentStickDir |= PAD_INPUT_DOWN;
    if (ax < -500) currentStickDir |= PAD_INPUT_LEFT;
    if (ax > 500)  currentStickDir |= PAD_INPUT_RIGHT;
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

bool InputManager::IsPadUpPressed() const
{
    bool cur = ((currentPad | currentStickDir) & PAD_INPUT_UP) != 0;
    bool prev = ((previousPad | previousStickDir) & PAD_INPUT_UP) != 0;
    return cur && !prev;
}

bool InputManager::IsPadDownPressed() const
{
    bool cur = ((currentPad | currentStickDir) & PAD_INPUT_DOWN) != 0;
    bool prev = ((previousPad | previousStickDir) & PAD_INPUT_DOWN) != 0;
    return cur && !prev;
}

bool InputManager::IsPadLeftPressed() const
{
    bool cur = ((currentPad | currentStickDir) & PAD_INPUT_LEFT) != 0;
    bool prev = ((previousPad | previousStickDir) & PAD_INPUT_LEFT) != 0;
    return cur && !prev;
}

bool InputManager::IsPadRightPressed() const
{
    bool cur = ((currentPad | currentStickDir) & PAD_INPUT_RIGHT) != 0;
    bool prev = ((previousPad | previousStickDir) & PAD_INPUT_RIGHT) != 0;
    return cur && !prev;
}

bool InputManager::IsPadConfirmPressed() const
{
    return ((currentPad & PAD_INPUT_1) != 0) && ((previousPad & PAD_INPUT_1) == 0);
}

bool InputManager::IsPadConfirmHeld() const
{
    return (currentPad & PAD_INPUT_1) != 0;
}

bool InputManager::IsPadCancelPressed() const
{
    return ((currentPad & PAD_INPUT_2) != 0) && ((previousPad & PAD_INPUT_2) == 0);
}

bool InputManager::IsPadStartPressed() const
{
    return ((currentPad & PAD_INPUT_8) != 0) && ((previousPad & PAD_INPUT_8) == 0);
}

#pragma once

class InputManager
{
  private:
    char currentKeys[256];
    char previousKeys[256];

    int currentMouse;
    int previousMouse;

    InputManager();
    ~InputManager();

  public:
    static InputManager &GetInstance()
    {
        static InputManager instance;
        return instance;
    }

    InputManager(const InputManager &) = delete;
    InputManager &operator=(const InputManager &) = delete;

    void Update();

    // キーボード用 (keyCode には KEY_INPUT_... を指定)
    bool IsKeyHeld(int keyCode) const;     // 連続で返す（押しっぱなし）
    bool IsKeyPressed(int keyCode) const;  // 1回だけ返す（押した瞬間）
    bool IsKeyReleased(int keyCode) const; // 離した瞬間

    // マウス用 (button には MOUSE_INPUT_LEFT 等を指定)
    bool IsMouseHeld(int button) const;     // 連続で返す（押しっぱなし）
    bool IsMousePressed(int button) const;  // 1回だけ返す（押した瞬間）
    bool IsMouseReleased(int button) const; // 離した瞬間
};

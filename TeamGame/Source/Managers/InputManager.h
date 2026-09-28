#pragma once

/**
 * @brief 入力（キーボード・マウス）を管理するシングルトンクラス
 * @details ユーザーからのキーボードやマウスの入力を監視し、押下・保持・解放の状態を取得できる機能を提供する。
 */
class InputManager
{
  private:
    char currentKeys[256];
    char previousKeys[256];

    int currentMouse;
    int previousMouse;

    /**
     * @brief コンストラクタ
     * @details 初期化を行い、外部からのインスタンス化を防ぐ。
     */
    InputManager();

    /**
     * @brief デストラクタ
     */
    ~InputManager();

  public:
    /**
     * @brief インスタンスの取得
     * @return InputManager& 入力マネージャのシングルトンインスタンス
     */
    static InputManager &GetInstance()
    {
        static InputManager instance;
        return instance;
    }

    InputManager(const InputManager &) = delete;
    InputManager &operator=(const InputManager &) = delete;

    /**
     * @brief 入力状態の更新
     * @details 毎フレーム呼び出され、キーボードとマウスの最新状態を取得・更新する。
     */
    void Update();

    /**
     * @brief キーが押され続けているか（押しっぱなし）
     * @param keyCode 対象のキーコード（KEY_INPUT_...）
     * @return bool 押され続けていればtrue
     * @details 指定したキーが現在押されている状態かどうかを判定する。
     */
    bool IsKeyHeld(int keyCode) const;

    /**
     * @brief キーが押された瞬間か
     * @param keyCode 対象のキーコード（KEY_INPUT_...）
     * @return bool 押された瞬間であればtrue
     * @details 指定したキーが現在フレームで新しく押されたかを判定する。
     */
    bool IsKeyPressed(int keyCode) const;

    /**
     * @brief キーが離された瞬間か
     * @param keyCode 対象のキーコード（KEY_INPUT_...）
     * @return bool 離された瞬間であればtrue
     * @details 指定したキーが現在フレームで離されたかを判定する。
     */
    bool IsKeyReleased(int keyCode) const;

    /**
     * @brief マウスボタンが押され続けているか（押しっぱなし）
     * @param button 対象のマウスボタン（MOUSE_INPUT_LEFT 等）
     * @return bool 押され続けていればtrue
     * @details 指定したマウスボタンが現在押されている状態かどうかを判定する。
     */
    bool IsMouseHeld(int button) const;

    /**
     * @brief マウスボタンが押された瞬間か
     * @param button 対象のマウスボタン（MOUSE_INPUT_LEFT 等）
     * @return bool 押された瞬間であればtrue
     * @details 指定したマウスボタンが現在フレームで新しく押されたかを判定する。
     */
    bool IsMousePressed(int button) const;

    /**
     * @brief マウスボタンが離された瞬間か
     * @param button 対象のマウスボタン（MOUSE_INPUT_LEFT 等）
     * @return bool 離された瞬間であればtrue
     * @details 指定したマウスボタンが現在フレームで離されたかを判定する。
     */
    bool IsMouseReleased(int button) const;
};

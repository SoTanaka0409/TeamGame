#pragma once
#include <string>

class DebugManager
{
private:
    bool m_isDebugMode = false;
    DebugManager() = default;

public:
    static DebugManager& GetInstance()
    {
        static DebugManager instance;
        return instance;
    }

    ~DebugManager() = default;

    // コピー・代入の禁止
    DebugManager(const DebugManager&) = delete;
    DebugManager& operator=(const DebugManager&) = delete;

    bool IsDebugMode() const { return m_isDebugMode; }
    void SetDebugMode(bool enable) { m_isDebugMode = enable; }
    void ToggleDebugMode() { m_isDebugMode = !m_isDebugMode; }

    // キー入力更新（Tab, F1キーなどのデバッグ操作監視）
    void Update();

    // デバッグ情報および操作ガイドのUI描画
    void DrawDebugOverlay(const std::string& stageName, float playerX, float playerY, int enemyCount);
};

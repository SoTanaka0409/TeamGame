#pragma once

#include <string>
#include <vector>
#include <DxLib.h>

/**
 * @brief 簡易クイックチャット・ラジオチャットを管理するシングルトンクラス
 */
class ChatManager {
public:
    struct ChatMessage {
        std::string senderName;
        std::string message;
        int teamId = 0;              // 0: 味方(青), 1: 敵(赤)
        bool isPlayer = false;       // プレイヤー本人の発言か
        int remainingFrames = 240;   // 4秒間表示
        float worldX = 0.0f;         // 発言キャラのワールド座標X（頭上吹き出し用）
        float worldY = 0.0f;         // 発言キャラのワールド座標Y
        bool showBubble = true;      // 頭上吹き出しを表示するか
    };

    static ChatManager& GetInstance();

    /**
     * @brief 毎フレームの更新処理（タイマー減衰、メニュー入力監視）
     * @param playerWorldX プレイヤーの現在ワールドX
     * @param playerWorldY プレイヤーの現在ワールドY
     */
    void Update(float playerWorldX, float playerWorldY);

    /**
     * @brief 画面左下のチャットログおよび頭上の吹き出しを描画
     */
    void Draw();

    /**
     * @brief チャットメッセージを追加
     * @param senderName 発言者名
     * @param message 本文
     * @param teamId チームID (0:味方, 1:敵)
     * @param isPlayer プレイヤー本人か
     * @param worldX 発言キャラのワールド座標X
     * @param worldY 発言キャラのワールド座標Y
     */
    void SendChat(const std::string& senderName, const std::string& message, int teamId, bool isPlayer, float worldX, float worldY);

    /**
     * @brief クイックチャットメニューが開いているか
     */
    bool IsMenuOpen() const { return m_isMenuOpen; }

    /**
     * @brief メッセージの全消去
     */
    void Clear();

private:
    ChatManager();
    ~ChatManager();

    bool m_isMenuOpen = false;
    float m_playerX = 0.0f;
    float m_playerY = 0.0f;

    // クイックチャット定型文定義
    struct QuickCommand {
        std::string keyLabel;
        std::string text;
    };
    std::vector<QuickCommand> m_commands;

    // ログリスト
    std::vector<ChatMessage> m_messages;
};

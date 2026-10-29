#include "ChatManager.h"
#include "FontManager.h"
#include "InputManager.h"
#include "SoundManager.h"
#include "SceneManager.h"
#include "ObjectManager.h"
#include "../Scenes/Scene.h"
#include "../Characters/Character.h"
#include "../Core/Camera.h"
#include <cmath>

ChatManager& ChatManager::GetInstance() {
    static ChatManager instance;
    return instance;
}

ChatManager::ChatManager() {
    // プレイヤーが選べる定型文リスト (1~4)
    m_commands.push_back({ "[1]", "敵を発見した！" });
    m_commands.push_back({ "[2]", "援護を頼む！" });
    m_commands.push_back({ "[3]", "突撃するぞ！" });
    m_commands.push_back({ "[4]", "ナイス！" });
}

ChatManager::~ChatManager() {
    Clear();
}

void ChatManager::Clear() {
    m_messages.clear();
    m_isMenuOpen = false;
}

void ChatManager::SendChat(const std::string& senderName, const std::string& message, int teamId, bool isPlayer, float worldX, float worldY) {
    ChatMessage msg;
    msg.senderName = senderName;
    msg.message = message;
    msg.teamId = teamId;
    msg.isPlayer = isPlayer;
    msg.remainingFrames = 240; // 4秒間表示
    msg.worldX = worldX;
    msg.worldY = worldY;
    msg.showBubble = true;

    m_messages.push_back(msg);
    if (m_messages.size() > 6) {
        m_messages.erase(m_messages.begin());
    }

    // チャット受信時の短い高音ピピッ音(UI音)
    SoundManager::GetInstance().Play3D("gunshot", Vector2(worldX, worldY), 1000.0f, 0.2f, teamId);
}

void ChatManager::Update(float playerWorldX, float playerWorldY) {
    m_playerX = playerWorldX;
    m_playerY = playerWorldY;

    // Cキーでクイックチャットメニューの開閉トグル
    if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_C)) {
        m_isMenuOpen = !m_isMenuOpen;
    }

    // メニューが開いている間の定型文選択入力 (1~4)
    if (m_isMenuOpen) {
        int selectedIndex = -1;
        if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_1) || InputManager::GetInstance().IsKeyPressed(KEY_INPUT_NUMPAD1)) {
            selectedIndex = 0;
        } else if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_2) || InputManager::GetInstance().IsKeyPressed(KEY_INPUT_NUMPAD2)) {
            selectedIndex = 1;
        } else if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_3) || InputManager::GetInstance().IsKeyPressed(KEY_INPUT_NUMPAD3)) {
            selectedIndex = 2;
        } else if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_4) || InputManager::GetInstance().IsKeyPressed(KEY_INPUT_NUMPAD4)) {
            selectedIndex = 3;
        }

        if (selectedIndex >= 0 && selectedIndex < static_cast<int>(m_commands.size())) {
            // プレイヤー本人のチャット発言
            SendChat("YOU", m_commands[selectedIndex].text, 0, true, m_playerX, m_playerY);
            m_isMenuOpen = false; // 発信後に自動でメニューを閉じる

            // 味方Botのリアクション返答（演出）
            auto scene = SceneManager::GetInstance().GetCurrentScene();
            if (scene && scene->GetObjectManager()) {
                const auto& objs = scene->GetObjectManager()->GetObjects();
                for (auto obj : objs) {
                    Character* ch = dynamic_cast<Character*>(obj);
                    // 味方(teamId == 0)かつ自分以外で生存中のBot
                    if (ch && ch->teamId == 0 && ch->characterName != "YOU" && ch->characterName != "Player" && !ch->isDeadProcessed && ch->status.GetHp() > 0) {
                        std::string replyText = "";
                        if (selectedIndex == 0) replyText = "了解、迎撃する！";
                        else if (selectedIndex == 1) replyText = "今向かう、持ちこたえろ！";
                        else if (selectedIndex == 2) replyText = "突撃！援護する！";
                        else if (selectedIndex == 3) replyText = "感謝する！";

                        SendChat(ch->characterName, replyText, 0, false, ch->GetPosition().x, ch->GetPosition().y);
                        break; // 1体の味方が応答
                    }
                }
            }
        }
    }

    // ログ表示タイマーの減衰
    for (auto it = m_messages.begin(); it != m_messages.end();) {
        it->remainingFrames--;
        if (it->remainingFrames <= 0) {
            it = m_messages.erase(it);
        } else {
            ++it;
        }
    }
}

void ChatManager::Draw() {
    int sw = 1920, sh = 1080;
    GetDrawScreenSize(&sw, &sh);

    // =========================================================================
    // 1. キャラクター頭上の吹き出し（直近の発言をワールド座標に合わせて表示）
    // =========================================================================
    for (const auto& msg : m_messages) {
        if (!msg.showBubble || msg.remainingFrames < 60) continue; // 残り1秒未満は消去

        float screenX = Camera::WorldToScreenX(msg.worldX);
        float screenY = Camera::WorldToScreenY(msg.worldY) - 55.0f; // 頭上に配置

        // 画面外ならスキップ
        if (screenX < -100 || screenX > sw + 100 || screenY < -50 || screenY > sh + 50) continue;

        int msgW = FontManager::GetInstance().GetStringWidth(msg.message.c_str(), 18, 6);
        int bubbleW = msgW + 28;
        int bubbleH = 32;
        int bx = static_cast<int>(screenX - bubbleW * 0.5f);
        int by = static_cast<int>(screenY - bubbleH);

        // 吹き出し背景（黒半透明）
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
        DrawRoundRect(bx, by, bx + bubbleW, by + bubbleH, 6, 6, GetColor(15, 20, 25), TRUE);
        // 吹き出し下部の三角のしっぽ
        DrawTriangle(static_cast<int>(screenX - 6), by + bubbleH,
                     static_cast<int>(screenX + 6), by + bubbleH,
                     static_cast<int>(screenX), by + bubbleH + 7,
                     GetColor(15, 20, 25), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 吹き出し外枠（チームカラー）
        unsigned int borderColor = (msg.teamId == 0) ? GetColor(70, 170, 255) : GetColor(255, 90, 90);
        DrawRoundRect(bx, by, bx + bubbleW, by + bubbleH, 6, 6, borderColor, FALSE);

        // テキスト描画
        FontManager::GetInstance().DrawString(bx + 14, by + 7, msg.message.c_str(), GetColor(255, 255, 255), 18, 6);
    }

    // =========================================================================
    // 2. 画面左下のチャットログ（最新のメッセージ一覧）
    // =========================================================================
    if (!m_messages.empty()) {
        int logX = 40;
        int logBottomY = sh - 140;
        int lineH = 34;
        int startY = logBottomY - static_cast<int>(m_messages.size()) * lineH;

        for (size_t i = 0; i < m_messages.size(); ++i) {
            const auto& msg = m_messages[i];
            int y = startY + static_cast<int>(i) * lineH;

            // フェードアウト効果
            int alpha = 210;
            if (msg.remainingFrames < 30) {
                alpha = static_cast<int>((msg.remainingFrames / 30.0f) * 210.0f);
            }

            std::string fullText = "[" + msg.senderName + "]: " + msg.message;
            int textW = FontManager::GetInstance().GetStringWidth(fullText.c_str(), 18, 6);
            int boxW = textW + 32;

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
            // 背景プレート
            DrawRoundRect(logX, y, logX + boxW, y + 28, 4, 4, GetColor(10, 15, 20), TRUE);

            // 送信者カラー (味方青 / 敵赤 / プレイヤー黄)
            unsigned int nameColor = msg.isPlayer ? GetColor(255, 230, 40) : ((msg.teamId == 0) ? GetColor(100, 190, 255) : GetColor(255, 120, 120));
            std::string prefix = "[" + msg.senderName + "]: ";
            FontManager::GetInstance().DrawString(logX + 12, y + 5, prefix.c_str(), nameColor, 18, 6);

            int prefixW = FontManager::GetInstance().GetStringWidth(prefix.c_str(), 18, 6);
            FontManager::GetInstance().DrawString(logX + 12 + prefixW, y + 5, msg.message.c_str(), GetColor(250, 250, 250), 18, 6);

            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
    }

    // =========================================================================
    // 3. クイックチャット選択メニュー (Cキー押下時)
    // =========================================================================
    if (m_isMenuOpen) {
        int menuW = 320;
        int menuH = 46 + static_cast<int>(m_commands.size()) * 38;
        int menuX = 40;
        int menuY = sh - 150 - menuH;

        // メニュー外枠・背景
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 230);
        DrawRoundRect(menuX, menuY, menuX + menuW, menuY + menuH, 8, 8, GetColor(15, 20, 28), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawRoundRect(menuX, menuY, menuX + menuW, menuY + menuH, 8, 8, GetColor(0, 180, 255), FALSE);

        // ヘッダータイトル
        DrawRoundRect(menuX, menuY, menuX + menuW, menuY + 36, 8, 8, GetColor(0, 140, 230), TRUE);
        FontManager::GetInstance().DrawString(menuX + 16, menuY + 8, "RADIO CHAT  [ C で閉じる ]", GetColor(255, 255, 255), 18, 8);

        // 選択肢行
        for (size_t i = 0; i < m_commands.size(); ++i) {
            int itemY = menuY + 44 + static_cast<int>(i) * 36;
            // キー番号バッジ
            FontManager::GetInstance().DrawString(menuX + 18, itemY + 6, m_commands[i].keyLabel.c_str(), GetColor(255, 230, 40), 18, 8);
            // 文言
            FontManager::GetInstance().DrawString(menuX + 60, itemY + 6, m_commands[i].text.c_str(), GetColor(240, 245, 250), 18, 6);
        }
    } else {
        // メニューが閉じている時のヒント案内 (小さく薄く表示)
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 140);
        FontManager::GetInstance().DrawString(40, sh - 100, "[ C ] ラジオチャット", GetColor(200, 215, 230), 16, 6);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}

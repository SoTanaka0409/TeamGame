#include "SoundVisualizer.h"
#include "FontManager.h"
#include "../Characters/Player.h"
#include <DxLib.h>
#include <cmath>
#include <algorithm>

void SoundVisualizer::AddSound(SoundVisualType type, const Vector2& sourcePos, float maxDistance)
{
    // 重複・近接音の統合（同じ種類で、方向や位置が近いものは更新してタイマーリセット）
    for (auto& ind : m_indicators)
    {
        if (ind.type == type)
        {
            float dx = ind.worldPos.x - sourcePos.x;
            float dy = ind.worldPos.y - sourcePos.y;
            if (dx * dx + dy * dy < 220.0f * 220.0f)
            {
                ind.worldPos = sourcePos;
                ind.lifeTimer = ind.maxLife;
                return;
            }
        }
    }

    // 新規追加（最大保持数 10個まで）
    if (m_indicators.size() >= 10)
    {
        m_indicators.erase(m_indicators.begin());
    }

    SoundIndicator ind;
    ind.worldPos = sourcePos;
    ind.type = type;
    ind.maxLife = (type == SoundVisualType::Gunshot) ? 75 : 105;
    ind.lifeTimer = ind.maxLife;
    ind.maxDistance = maxDistance;
    m_indicators.push_back(ind);
}

void SoundVisualizer::Update()
{
    for (auto it = m_indicators.begin(); it != m_indicators.end(); )
    {
        it->lifeTimer--;
        if (it->lifeTimer <= 0)
        {
            it = m_indicators.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void SoundVisualizer::Clear()
{
    m_indicators.clear();
}

void SoundVisualizer::Draw(const Vector2& playerWorldPos, float playerScreenX, float playerScreenY)
{
    Draw(nullptr, playerScreenX, playerScreenY);
}

void SoundVisualizer::Draw(const Player* player, float playerScreenX, float playerScreenY)
{
    if (m_indicators.empty()) return;

    Vector2 playerWorldPos = player ? player->GetPosition() : Vector2(0.0f, 0.0f);

    for (const auto& ind : m_indicators)
    {
        // 敵がライトの範囲内にいる（直接見えている）場合はサウンドマークを出さない
        if (player && player->IsWorldPosInLight(ind.worldPos))
        {
            continue;
        }

        float dx = ind.worldPos.x - playerWorldPos.x;
        float dy = ind.worldPos.y - playerWorldPos.y;
        float distPx = std::sqrt(dx * dx + dy * dy);

        // 自機と重なっている場合や可聴距離を超えている場合はスキップ
        if (distPx < 20.0f || distPx > ind.maxDistance) continue;

        float angle = std::atan2(dy, dx);
        float distMeter = distPx / 40.0f; // 40px = 1m換算

        // 寿命に応じたフェードアウト（残り25フレームで滑らかにフェード）
        float progress = 1.0f - (static_cast<float>(ind.lifeTimer) / ind.maxLife);
        int alpha = (ind.lifeTimer > 25) ? 235 : static_cast<int>(235.0f * (ind.lifeTimer / 25.0f));
        if (alpha <= 0) continue;

        // インジケーターの配置半径（自キャラ周囲、時間経過で微かに外側へ広がるパルス）
        float baseRadius = 142.0f;
        float pulseOffset = 12.0f * progress;
        float centerRadius = baseRadius + pulseOffset;

        // テーマカラーとアイコン設定
        unsigned int primaryColor = 0;
        unsigned int secondaryColor = 0;

        if (ind.type == SoundVisualType::Gunshot)
        {
            // 銃声: 警告感のある赤橙色〜アンバー
            primaryColor = GetColor(255, 60, 30);
            secondaryColor = GetColor(255, 185, 40);
        }
        else
        {
            // 定期音（ソナー音波探知）: 情報感のあるネオンシアン〜エレクトリックブルー
            primaryColor = GetColor(0, 220, 255);
            secondaryColor = GetColor(120, 245, 255);
        }

        // ==========================================
        // 1. 扇型音波アーク（荒野行動・Fortnite風の二重ウェーブ）
        // ==========================================
        float arcHalfAngle = 0.35f; // 約20度
        int segments = 12;

        // 複数層のアーク（外側と内側）
        for (int layer = 0; layer < 2; layer++)
        {
            float r = centerRadius + layer * 14.0f;
            int arcAlpha = static_cast<int>(alpha * (layer == 0 ? 0.95f : 0.65f));

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, arcAlpha);

            // 輪郭黒線
            for (int s = 0; s < segments; s++)
            {
                float a1 = angle - arcHalfAngle + (2.0f * arcHalfAngle * s) / segments;
                float a2 = angle - arcHalfAngle + (2.0f * arcHalfAngle * (s + 1)) / segments;

                int x1 = static_cast<int>(playerScreenX + std::cos(a1) * r);
                int y1 = static_cast<int>(playerScreenY + std::sin(a1) * r);
                int x2 = static_cast<int>(playerScreenX + std::cos(a2) * r);
                int y2 = static_cast<int>(playerScreenY + std::sin(a2) * r);

                DrawLine(x1, y1, x2, y2, GetColor(0, 0, 0), 5);
            }

            // カラー線
            for (int s = 0; s < segments; s++)
            {
                float a1 = angle - arcHalfAngle + (2.0f * arcHalfAngle * s) / segments;
                float a2 = angle - arcHalfAngle + (2.0f * arcHalfAngle * (s + 1)) / segments;

                int x1 = static_cast<int>(playerScreenX + std::cos(a1) * r);
                int y1 = static_cast<int>(playerScreenY + std::sin(a1) * r);
                int x2 = static_cast<int>(playerScreenX + std::cos(a2) * r);
                int y2 = static_cast<int>(playerScreenY + std::sin(a2) * r);

                DrawLine(x1, y1, x2, y2, (layer == 0 ? primaryColor : secondaryColor), 3);
            }
        }

        // ==========================================
        // 2. 音源方向を指す鋭い楔形ポインター（三角矢印）
        // ==========================================
        float arrowCenterR = centerRadius - 4.0f;
        float arrowLen = 14.0f;
        float arrowTipX = playerScreenX + std::cos(angle) * (arrowCenterR + arrowLen);
        float arrowTipY = playerScreenY + std::sin(angle) * (arrowCenterR + arrowLen);
        float arrowLeftX = playerScreenX + std::cos(angle + 0.18f) * (arrowCenterR - 4.0f);
        float arrowLeftY = playerScreenY + std::sin(angle + 0.18f) * (arrowCenterR - 4.0f);
        float arrowRightX = playerScreenX + std::cos(angle - 0.18f) * (arrowCenterR - 4.0f);
        float arrowRightY = playerScreenY + std::sin(angle - 0.18f) * (arrowCenterR - 4.0f);

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        // 黒縁取り
        DrawTriangle(static_cast<int>(arrowTipX), static_cast<int>(arrowTipY),
                     static_cast<int>(arrowLeftX), static_cast<int>(arrowLeftY),
                     static_cast<int>(arrowRightX), static_cast<int>(arrowRightY),
                     GetColor(0, 0, 0), FALSE);
        // 塗りつぶし
        DrawTriangle(static_cast<int>(arrowTipX), static_cast<int>(arrowTipY),
                     static_cast<int>(arrowLeftX), static_cast<int>(arrowLeftY),
                     static_cast<int>(arrowRightX), static_cast<int>(arrowRightY),
                     primaryColor, TRUE);

        // ==========================================
        // 3. アイコン＆距離表示バッジ
        // ==========================================
        float iconR = centerRadius + 30.0f;
        float badgeCenterX = playerScreenX + std::cos(angle) * iconR;
        float badgeCenterY = playerScreenY + std::sin(angle) * iconR;

        // 距離テキスト整形（例: "38m"）
        char distBuf[16];
        snprintf(distBuf, sizeof(distBuf), "%dm", static_cast<int>(distMeter));

        int badgeW = 48;
        int badgeH = 24;
        int bx1 = static_cast<int>(badgeCenterX - badgeW / 2);
        int by1 = static_cast<int>(badgeCenterY - badgeH / 2);
        int bx2 = bx1 + badgeW;
        int by2 = by1 + badgeH;

        // バッジ背景（黒＋半透明）
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(alpha * 0.85f));
        DrawBox(bx1, by1, bx2, by2, GetColor(15, 20, 30), TRUE);

        // バッジ外枠（アクセントカラー）
        DrawBox(bx1, by1, bx2, by2, primaryColor, FALSE);

        // 距離文字の描画
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        FontManager::GetInstance().DrawStringCenter(static_cast<int>(badgeCenterX), by1 + 4, distBuf, secondaryColor, 15);

        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}

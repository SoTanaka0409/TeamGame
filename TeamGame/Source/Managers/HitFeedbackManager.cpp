#include "HitFeedbackManager.h"
#include "FontManager.h"
#include "SceneManager.h"
#include "../Core/Camera.h"
#include "../Characters/Player.h"
#include "../Scenes/GameScene.h"
#include <DxLib.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>

namespace
{
    // ---- 調整用の数値（見た目だけ。ゲームルールには影響しない） ----
    const int   kPopupLife        = 50;    ///< ダメージ数字の表示フレーム
    const int   kKillPopupLife    = 70;    ///< とどめのダメージ数字の表示フレーム
    const int   kMergeFrames      = 8;     ///< この時間内の同じ相手へのヒットは1つの数字にまとめる（ショットガン対策）
    const float kPopupRise        = 46.0f; ///< 数字が浮き上がる高さ（画面ピクセル）
    const int   kMarkerLife       = 16;    ///< ヒットマーカーの表示フレーム
    const int   kKillMarkerLife   = 30;    ///< とどめのヒットマーカーの表示フレーム
    const int   kKillHitStop      = 4;     ///< とどめを刺したとき、完全に止めるフレーム
    const int   kKillSlow         = 18;    ///< とどめを刺したあとのスロー演出のフレーム（2フレームに1回だけ進める）
    const float kHitShakePower    = 2.5f;  ///< 当てたときの画面揺れ
    const int   kHitShakeFrames   = 5;
    const float kKillShakePower   = 9.0f;  ///< とどめを刺したときの画面揺れ
    const int   kKillShakeFrames  = 16;
    const size_t kMaxPopups       = 16;
    const size_t kMaxMarkers      = 8;

    /**
     * @brief 黒いふちどり付きで文字を中央ぞろえ描画する
     */
    void DrawOutlinedCenter(int cx, int y, const char* text, unsigned int color, int size)
    {
        FontManager& fm = FontManager::GetInstance();
        const unsigned int black = GetColor(10, 10, 14);
        const int o = 2;
        fm.DrawStringCenter(cx - o, y, text, black, size);
        fm.DrawStringCenter(cx + o, y, text, black, size);
        fm.DrawStringCenter(cx, y - o, text, black, size);
        fm.DrawStringCenter(cx, y + o, text, black, size);
        fm.DrawStringCenter(cx, y, text, color, size);
    }
}

HitFeedbackManager& HitFeedbackManager::GetInstance()
{
    static HitFeedbackManager instance;
    return instance;
}

void HitFeedbackManager::Clear()
{
    m_popups.clear();
    m_markers.clear();
    m_hitStopTimer = 0;
    m_slowTimer = 0;
    m_frameCount = 0;
    m_shakeTimer = 0;
    m_shakeMaxTimer = 0;
    m_shakePower = 0.0f;
    m_shakeOffset = Vector2(0.0f, 0.0f);
}

bool HitFeedbackManager::IsLocalPlayerAttack(const std::string& attackerName, int attackerTeamId)
{
    auto scene = SceneManager::GetInstance().GetCurrentScene();
    GameScene* gameScene = dynamic_cast<GameScene*>(scene.get());
    if (!gameScene) return false;

    Player* player = gameScene->GetPlayer();
    if (!player) return false;

    // Botの名前には「[味方]」「[敵]」が付くため、名前とチームが一致すればプレイヤー本人の攻撃
    return player->teamId == attackerTeamId && player->GetCharacterName() == attackerName;
}

void HitFeedbackManager::OnPlayerHit(const void* targetId, const Vector2& hitPos, int damage, bool isKill)
{
    // ---- ダメージ数字（同じ相手への連続ヒットは1つにまとめる） ----
    bool merged = false;
    for (auto& p : m_popups)
    {
        if (p.targetId == targetId && (p.maxLife - p.life) <= kMergeFrames && !p.isKill)
        {
            p.amount += damage;
            p.isKill = isKill;
            p.worldPos = hitPos;
            p.maxLife = isKill ? kKillPopupLife : kPopupLife;
            p.life = p.maxLife;
            merged = true;
            break;
        }
    }
    if (!merged)
    {
        if (m_popups.size() >= kMaxPopups) m_popups.erase(m_popups.begin());
        DamagePopup p;
        p.targetId = targetId;
        p.worldPos = hitPos;
        p.amount = damage;
        p.isKill = isKill;
        p.maxLife = isKill ? kKillPopupLife : kPopupLife;
        p.life = p.maxLife;
        p.driftX = static_cast<float>((std::rand() % 41) - 20);
        m_popups.push_back(p);
    }

    // ---- ヒットマーカー ----
    if (m_markers.size() >= kMaxMarkers) m_markers.erase(m_markers.begin());
    HitMarker m;
    m.worldPos = hitPos;
    m.isKill = isKill;
    m.maxLife = isKill ? kKillMarkerLife : kMarkerLife;
    m.life = m.maxLife;
    m_markers.push_back(m);

    // ---- 画面揺れ・ヒットストップ ----
    if (isKill)
    {
        StartShake(kKillShakePower, kKillShakeFrames);
        m_hitStopTimer = kKillHitStop;
        m_slowTimer = kKillSlow;
    }
    else
    {
        StartShake(kHitShakePower, kHitShakeFrames);
    }
}

void HitFeedbackManager::StartShake(float power, int frames)
{
    // 今の揺れのほうが強ければ、弱い揺れで上書きしない
    float currentPower = (m_shakeMaxTimer > 0) ? m_shakePower * (static_cast<float>(m_shakeTimer) / m_shakeMaxTimer) : 0.0f;
    if (power < currentPower) return;

    m_shakePower = power;
    m_shakeTimer = frames;
    m_shakeMaxTimer = frames;
}

void HitFeedbackManager::Update()
{
    m_frameCount++;

    if (m_hitStopTimer > 0)
    {
        m_hitStopTimer--;
    }
    else if (m_slowTimer > 0)
    {
        m_slowTimer--;
    }

    // 画面揺れ（時間とともに弱くなる）
    if (m_shakeTimer > 0)
    {
        float ratio = static_cast<float>(m_shakeTimer) / static_cast<float>(m_shakeMaxTimer);
        float power = m_shakePower * ratio;
        float rx = (static_cast<float>(std::rand()) / RAND_MAX) * 2.0f - 1.0f;
        float ry = (static_cast<float>(std::rand()) / RAND_MAX) * 2.0f - 1.0f;
        m_shakeOffset = Vector2(rx * power, ry * power);
        m_shakeTimer--;
    }
    else
    {
        m_shakeOffset = Vector2(0.0f, 0.0f);
    }

    for (auto& p : m_popups) p.life--;
    for (auto& m : m_markers) m.life--;
    m_popups.erase(std::remove_if(m_popups.begin(), m_popups.end(),
        [](const DamagePopup& p) { return p.life <= 0; }), m_popups.end());
    m_markers.erase(std::remove_if(m_markers.begin(), m_markers.end(),
        [](const HitMarker& m) { return m.life <= 0; }), m_markers.end());
}

bool HitFeedbackManager::ShouldSkipWorldUpdate() const
{
    if (m_hitStopTimer > 0) return true;
    // スロー演出中は2フレームに1回だけゲーム世界を進める
    if (m_slowTimer > 0) return (m_frameCount % 2) != 0;
    return false;
}

void HitFeedbackManager::Draw() const
{
    // ---- ヒットマーカー（当たった場所に出る×印） ----
    for (const auto& m : m_markers)
    {
        float t = 1.0f - static_cast<float>(m.life) / static_cast<float>(m.maxLife); // 0→1
        float sx = Camera::WorldToScreenX(m.worldPos.x);
        float sy = Camera::WorldToScreenY(m.worldPos.y);

        // 最初に大きく出て、すぐに締まる
        float baseGap = m.isKill ? 16.0f : 10.0f;
        float baseLen = m.isKill ? 18.0f : 11.0f;
        float pop = (t < 0.25f) ? (1.0f + (0.25f - t) * 2.0f) : 1.0f;
        float gap = baseGap * pop;
        float len = baseLen * pop;
        float thick = m.isKill ? 4.0f : 3.0f;
        int alpha = static_cast<int>(255.0f * (1.0f - t * t));

        unsigned int col = m.isKill ? GetColor(255, 60, 50) : GetColor(255, 255, 255);
        unsigned int shadow = GetColor(0, 0, 0);

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        const float d = 0.70710678f; // 45度の向き
        for (int i = 0; i < 4; i++)
        {
            float dx = (i == 0 || i == 3) ? d : -d;
            float dy = (i < 2) ? d : -d;
            float x1 = sx + dx * gap;
            float y1 = sy + dy * gap;
            float x2 = sx + dx * (gap + len);
            float y2 = sy + dy * (gap + len);
            DrawLineAA(x1 + 1.5f, y1 + 1.5f, x2 + 1.5f, y2 + 1.5f, shadow, thick + 1.0f);
            DrawLineAA(x1, y1, x2, y2, col, thick);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // ---- ダメージ数字（上に浮かびながら消える） ----
    for (const auto& p : m_popups)
    {
        float t = 1.0f - static_cast<float>(p.life) / static_cast<float>(p.maxLife); // 0→1
        float ease = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);                 // 最初速く、あとゆっくり
        float sx = Camera::WorldToScreenX(p.worldPos.x) + p.driftX * ease;
        float sy = Camera::WorldToScreenY(p.worldPos.y) - 28.0f - kPopupRise * ease;

        // 出た瞬間に少し大きく見せる
        int baseSize = p.isKill ? 40 : 28;
        int size = (t < 0.12f) ? static_cast<int>(baseSize * 1.25f) : baseSize;

        int alpha = (t < 0.6f) ? 255 : static_cast<int>(255.0f * (1.0f - (t - 0.6f) / 0.4f));
        if (alpha < 0) alpha = 0;

        unsigned int col;
        if (p.isKill)
        {
            col = GetColor(255, 70, 50);
        }
        else
        {
            // 大きいダメージほど黄色っぽく
            col = (p.amount >= 20) ? GetColor(255, 215, 70) : GetColor(255, 255, 255);
        }

        std::string text = std::to_string(p.amount);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        DrawOutlinedCenter(static_cast<int>(sx), static_cast<int>(sy), text.c_str(), col, size);
        if (p.isKill)
        {
            DrawOutlinedCenter(static_cast<int>(sx), static_cast<int>(sy) - 30, "KILL", GetColor(255, 235, 120), 22);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}

#include "CutinManager.h"
#include "FontManager.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>

CutinManager& CutinManager::GetInstance() {
    static CutinManager instance;
    return instance;
}

CutinManager::CutinManager() {
}

CutinManager::~CutinManager() {
    if (m_previewHandle != -1) {
        DeleteGraph(m_previewHandle);
        m_previewHandle = -1;
    }
}

void CutinManager::StartCutin(int characterId, const std::string& characterName, const std::string& skillName, unsigned int themeColor, const std::string& previewImgPath) {
    m_isActive = true;
    m_timer = 0;
    m_charaId = characterId;
    m_charaName = characterName;
    m_skillName = skillName;
    m_themeColor = themeColor;

    // カスタム画像ハンドルのロード（キャッシュ制御）
    std::string candidatePath = previewImgPath;
    if (candidatePath.empty()) {
        candidatePath = "Resource/Cutin/cutin_" + std::to_string(characterId) + ".png";
    }

    if (m_loadedPath != candidatePath) {
        if (m_previewHandle != -1) {
            DeleteGraph(m_previewHandle);
            m_previewHandle = -1;
        }
        // まず指定パスを試行
        m_previewHandle = LoadGraph(candidatePath.c_str());
        // 失敗した場合は Resource/Cutin/cutin_X.png を試行
        if (m_previewHandle == -1) {
            std::string fallbackPath = "Resource/Cutin/cutin_" + std::to_string(characterId) + ".png";
            m_previewHandle = LoadGraph(fallbackPath.c_str());
        }
        m_loadedPath = (m_previewHandle != -1) ? candidatePath : "";
    }

    // 放射集中線の生成 (P5風の強烈なスピード線)
    m_speedLines.clear();
    for (int i = 0; i < 48; ++i) {
        SpeedLine sl;
        sl.angle = (i / 48.0f) * 6.283185f + ((rand() % 100) / 1000.0f);
        sl.length = 600.0f + (rand() % 500);
        sl.width = 4.0f + (rand() % 16);
        sl.offset = 200.0f + (rand() % 150);
        m_speedLines.push_back(sl);
    }

    // コミック風トゲトゲ星・インク飛沫パーティクル
    m_shards.clear();
    for (int i = 0; i < 45; ++i) {
        Shard s;
        s.x = 960.0f + ((rand() % 700) - 350.0f);
        s.y = 540.0f + ((rand() % 340) - 170.0f);
        float spd = 16.0f + (rand() % 26);
        float ang = (rand() % 628) / 100.0f;
        s.vx = std::cos(ang) * spd;
        s.vy = std::sin(ang) * spd;
        s.size = 14.0f + (rand() % 28);
        s.angle = (rand() % 628) / 100.0f;
        s.vRot = ((rand() % 20) - 10) * 0.05f;
        s.type = rand() % 3;
        if (i % 3 == 0) s.color = themeColor;
        else if (i % 3 == 1) s.color = GetColor(255, 255, 255);
        else s.color = GetColor(20, 20, 25);
        m_shards.push_back(s);
    }
}

void CutinManager::Update() {
    if (!m_isActive) return;

    m_timer++;

    for (auto& s : m_shards) {
        s.x += s.vx;
        s.y += s.vy;
        s.vx *= 0.91f;
        s.vy *= 0.91f;
        s.angle += s.vRot;
    }

    if (m_timer >= m_maxFrames) {
        m_isActive = false;
        m_timer = 0;
    }
}

bool CutinManager::ShouldPauseGame() const {
    // テンポ維持のためゲーム内時間停止(ヒットストップ)は行わない（リアルタイム進行）
    return false;
}

// P5本家風の放射スピード線
void CutinManager::DrawPersonaSpeedLines(int sw, int sh, unsigned int baseColor) {
    float cx = sw * 0.5f;
    float cy = sh * 0.5f;

    for (const auto& sl : m_speedLines) {
        float cosA = std::cos(sl.angle);
        float sinA = std::sin(sl.angle);
        float startR = sl.offset;
        float endR = startR + sl.length;

        float x1 = cx + cosA * startR;
        float y1 = cy + sinA * startR;
        float x2 = cx + cosA * endR;
        float y2 = cy + sinA * endR;

        // 太い三角形でスピードラインを描画
        float perpX = -sinA * (sl.width * 0.5f);
        float perpY = cosA * (sl.width * 0.5f);

        DrawTriangle(
            static_cast<int>(x1), static_cast<int>(y1),
            static_cast<int>(x2 + perpX), static_cast<int>(y2 + perpY),
            static_cast<int>(x2 - perpX), static_cast<int>(y2 - perpY),
            baseColor, TRUE
        );
    }
}

// P5本家同様のシャープなキャラクター目元劇画イラスト描画
void CutinManager::DrawP5EyeIllustration(int charaId, float centerX, float centerY, float scale, unsigned int themeColor) {
    // 基準寸法
    float w = 560.0f * scale;
    float h = 180.0f * scale;
    float lx = centerX - w * 0.5f;
    float rx = centerX + w * 0.5f;

    // 白目・肌ベースのコントラスト（シャープな逆台形スリット）
    DrawTriangle(static_cast<int>(lx), static_cast<int>(centerY - 60 * scale),
                 static_cast<int>(rx), static_cast<int>(centerY - 40 * scale),
                 static_cast<int>(rx - 40 * scale), static_cast<int>(centerY + 60 * scale),
                 GetColor(245, 240, 240), TRUE);
    DrawTriangle(static_cast<int>(lx), static_cast<int>(centerY - 60 * scale),
                 static_cast<int>(rx - 40 * scale), static_cast<int>(centerY + 60 * scale),
                 static_cast<int>(lx + 30 * scale), static_cast<int>(centerY + 55 * scale),
                 GetColor(245, 240, 240), TRUE);

    // キャラごとの個別目元デザイン（P5劇画調インクペン画）
    // 1:指揮者(冷徹な切れ長アイ・紫), 2:反逆者(仮面・鋭い青瞳), 3:バカ(大きく見開いた狂気アイ・緑),
    // 4:主婦(太い眉と燃える決意の眼光・赤), 5:悪魔(爬虫類のような鋭利なスリット瞳・金)
    float eye1X = centerX - 110.0f * scale;
    float eye2X = centerX + 110.0f * scale;

    for (int side = 0; side < 2; ++side) {
        float ex = (side == 0) ? eye1X : eye2X;
        float dir = (side == 0) ? 1.0f : -1.0f;

        switch (charaId) {
            case 1: { // 指揮者: 冷徹な細身のキリッとした眼光
                // 上まぶたの黒太線
                DrawLine(static_cast<int>(ex - 80 * scale * dir), static_cast<int>(centerY - 10 * scale),
                         static_cast<int>(ex + 80 * scale * dir), static_cast<int>(centerY - 32 * scale),
                         GetColor(10, 10, 15), static_cast<int>(7 * scale));
                // 虹彩 (紫)
                DrawCircle(static_cast<int>(ex), static_cast<int>(centerY - 12 * scale), static_cast<int>(24 * scale), themeColor, TRUE);
                // 瞳孔 (黒・鋭い縦楕円)
                DrawBox(static_cast<int>(ex - 4 * scale), static_cast<int>(centerY - 24 * scale),
                        static_cast<int>(ex + 4 * scale), static_cast<int>(centerY),
                        GetColor(0, 0, 0), TRUE);
                // 白ハイライト
                DrawCircle(static_cast<int>(ex - 6 * scale * dir), static_cast<int>(centerY - 20 * scale), static_cast<int>(6 * scale), GetColor(255, 255, 255), TRUE);
                // まつ毛・アイラインの跳ね上げ
                DrawLine(static_cast<int>(ex + 70 * scale * dir), static_cast<int>(centerY - 30 * scale),
                         static_cast<int>(ex + 95 * scale * dir), static_cast<int>(centerY - 45 * scale),
                         GetColor(10, 10, 15), static_cast<int>(5 * scale));
                break;
            }
            case 2: { // 反逆者: ジョーカー風の黒仮面から覗く反逆の鋭い眼光
                // ペルソナ5風の黒アイマスク
                DrawTriangle(static_cast<int>(ex - 100 * scale * dir), static_cast<int>(centerY - 45 * scale),
                             static_cast<int>(ex + 100 * scale * dir), static_cast<int>(centerY - 55 * scale),
                             static_cast<int>(ex), static_cast<int>(centerY + 25 * scale),
                             GetColor(15, 15, 20), TRUE);
                // 白目スリット
                DrawTriangle(static_cast<int>(ex - 50 * scale * dir), static_cast<int>(centerY - 15 * scale),
                             static_cast<int>(ex + 60 * scale * dir), static_cast<int>(centerY - 25 * scale),
                             static_cast<int>(ex + 10 * scale * dir), static_cast<int>(centerY + 8 * scale),
                             GetColor(255, 255, 255), TRUE);
                // 鮮烈な青瞳
                DrawCircle(static_cast<int>(ex + 15 * scale * dir), static_cast<int>(centerY - 10 * scale), static_cast<int>(20 * scale), themeColor, TRUE);
                DrawCircle(static_cast<int>(ex + 15 * scale * dir), static_cast<int>(centerY - 10 * scale), static_cast<int>(8 * scale), GetColor(0, 0, 0), TRUE);
                DrawCircle(static_cast<int>(ex + 10 * scale * dir), static_cast<int>(centerY - 16 * scale), static_cast<int>(5 * scale), GetColor(255, 255, 255), TRUE);
                break;
            }
            case 3: { // バカ: 狂気のギラついた見開き目＆ギザギザハイライト
                // 大きく見開いた上下まぶた
                DrawLine(static_cast<int>(ex - 75 * scale * dir), static_cast<int>(centerY - 35 * scale),
                         static_cast<int>(ex + 75 * scale * dir), static_cast<int>(centerY - 25 * scale),
                         GetColor(10, 10, 15), static_cast<int>(8 * scale));
                DrawLine(static_cast<int>(ex - 60 * scale * dir), static_cast<int>(centerY + 25 * scale),
                         static_cast<int>(ex + 60 * scale * dir), static_cast<int>(centerY + 30 * scale),
                         GetColor(10, 10, 15), static_cast<int>(6 * scale));
                // 狂乱の緑瞳
                DrawCircle(static_cast<int>(ex), static_cast<int>(centerY), static_cast<int>(28 * scale), themeColor, TRUE);
                DrawCircle(static_cast<int>(ex), static_cast<int>(centerY), static_cast<int>(12 * scale), GetColor(0, 0, 0), TRUE);
                // 狂気放射ハイライト
                DrawLine(static_cast<int>(ex - 12 * scale), static_cast<int>(centerY - 12 * scale),
                         static_cast<int>(ex + 12 * scale), static_cast<int>(centerY + 12 * scale),
                         GetColor(255, 255, 255), static_cast<int>(4 * scale));
                break;
            }
            case 4: { // 主婦: 燃える炎を宿した怒涛の眼光
                // 鋭く角度がついた太眉＆力強い上まぶた
                DrawLine(static_cast<int>(ex - 85 * scale * dir), static_cast<int>(centerY - 15 * scale),
                         static_cast<int>(ex + 85 * scale * dir), static_cast<int>(centerY - 45 * scale),
                         GetColor(10, 10, 15), static_cast<int>(10 * scale));
                // 烈火の赤瞳
                DrawCircle(static_cast<int>(ex), static_cast<int>(centerY - 10 * scale), static_cast<int>(25 * scale), themeColor, TRUE);
                // 瞳の中に炎の十字アクセント
                DrawLine(static_cast<int>(ex - 15 * scale), static_cast<int>(centerY - 10 * scale),
                         static_cast<int>(ex + 15 * scale), static_cast<int>(centerY - 10 * scale),
                         GetColor(255, 240, 50), static_cast<int>(4 * scale));
                DrawLine(static_cast<int>(ex), static_cast<int>(centerY - 25 * scale),
                         static_cast<int>(ex), static_cast<int>(centerY + 5 * scale),
                         GetColor(255, 240, 50), static_cast<int>(4 * scale));
                break;
            }
            case 5: { // 悪魔: スナイパーの冷徹な獲物を狙うスリット瞳＆照準スコープ光
                // 鋭角なシャドウ
                DrawTriangle(static_cast<int>(ex - 90 * scale * dir), static_cast<int>(centerY - 30 * scale),
                             static_cast<int>(ex + 90 * scale * dir), static_cast<int>(centerY - 38 * scale),
                             static_cast<int>(ex), static_cast<int>(centerY + 20 * scale),
                             GetColor(20, 10, 10), TRUE);
                // 妖しく光る琥珀・ゴールド瞳
                DrawCircle(static_cast<int>(ex), static_cast<int>(centerY - 8 * scale), static_cast<int>(22 * scale), themeColor, TRUE);
                // 照準風クロスヘア十字ライン
                DrawLine(static_cast<int>(ex - 22 * scale), static_cast<int>(centerY - 8 * scale),
                         static_cast<int>(ex + 22 * scale), static_cast<int>(centerY - 8 * scale),
                         GetColor(255, 30, 30), static_cast<int>(2 * scale));
                DrawLine(static_cast<int>(ex), static_cast<int>(centerY - 30 * scale),
                         static_cast<int>(ex), static_cast<int>(centerY + 14 * scale),
                         GetColor(255, 30, 30), static_cast<int>(2 * scale));
                // 猫・爬虫類のような縦細スリット瞳孔
                DrawBox(static_cast<int>(ex - 3 * scale), static_cast<int>(centerY - 22 * scale),
                        static_cast<int>(ex + 3 * scale), static_cast<int>(centerY + 6 * scale),
                        GetColor(0, 0, 0), TRUE);
                break;
            }
        }
    }

    // 鼻筋・眉間の劇画タッチ影（コミック風ハッチング線）
    for (int k = -2; k <= 2; ++k) {
        float hx = centerX + k * 8 * scale;
        DrawLine(static_cast<int>(hx), static_cast<int>(centerY - 45 * scale),
                 static_cast<int>(hx - 12 * scale), static_cast<int>(centerY - 10 * scale),
                 GetColor(50, 40, 40), static_cast<int>(2 * scale));
    }
}

void CutinManager::Draw() {
    if (!m_isActive) return;

    int sw = 1920, sh = 1080;
    GetDrawScreenSize(&sw, &sh);

    // =========================================================================
    // Stage 1: ゲーム画面を塞がない薄いレターボックス調シャドウ（画面上下の視界を完全確保）
    // =========================================================================
    // 全画面ベタ塗りは廃止し、帯の背後付近のみほんのり暗くして文字のコントラストを強調
    float centerY = 540.0f;
    float bandHeight = 220.0f; // 400pxから220pxにスリム化

    // =========================================================================
    // Stage 2: 超高速スライスアニメーション (約0.4秒 = 全体26フレーム)
    // 0~4F: 画面左外から超高速で突き刺さる
    // 5~17F: わずかにドリフトしながら魅せる (13フレーム)
    // 18~26F: 画面右へ一気に駆け抜けてフェードアウト
    // =========================================================================
    float slideX = 0.0f;
    float scaleAnim = 1.0f;

    if (m_timer <= 4) {
        float t = m_timer / 4.0f;
        slideX = (1.0f - t) * (1.0f - t) * -1800.0f;
        scaleAnim = 1.15f - (t * 0.15f);
    } else if (m_timer <= 17) {
        slideX = (m_timer - 4) * 2.5f; // 微小なドリフト
        scaleAnim = 1.0f;
    } else {
        float t = (m_timer - 17) / 9.0f;
        slideX = t * t * 2200.0f;
        scaleAnim = 1.0f + (t * 0.1f);
    }

    float currentHeight = bandHeight * scaleAnim;
    float skewX = 200.0f; // 傾き

    // 帯の角頂点
    float p1x = -300.0f + skewX + slideX, p1y = centerY - currentHeight * 0.5f;
    float p2x = sw + 300.0f + skewX + slideX, p2y = centerY - currentHeight * 0.5f - 20.0f;
    float p3x = sw + 300.0f - skewX + slideX, p3y = centerY + currentHeight * 0.5f + 20.0f;
    float p4x = -300.0f - skewX + slideX, p4y = centerY + currentHeight * 0.5f;

    // 1. スラッシュ帯のドロップシャドウ（漆黒）
    DrawTriangle(static_cast<int>(p1x - 16), static_cast<int>(p1y - 16),
                 static_cast<int>(p2x + 16), static_cast<int>(p2y - 16),
                 static_cast<int>(p3x + 16), static_cast<int>(p3y + 16),
                 GetColor(0, 0, 0), TRUE);
    DrawTriangle(static_cast<int>(p1x - 16), static_cast<int>(p1y - 16),
                 static_cast<int>(p3x + 16), static_cast<int>(p3y + 16),
                 static_cast<int>(p4x - 16), static_cast<int>(p4y + 16),
                 GetColor(0, 0, 0), TRUE);

    // 2. キャラクター固有テーマカラーのシャープな帯
    DrawTriangle(static_cast<int>(p1x), static_cast<int>(p1y),
                 static_cast<int>(p2x), static_cast<int>(p2y),
                 static_cast<int>(p3x), static_cast<int>(p3y),
                 m_themeColor, TRUE);
    DrawTriangle(static_cast<int>(p1x), static_cast<int>(p1y),
                 static_cast<int>(p3x), static_cast<int>(p3y),
                 static_cast<int>(p4x), static_cast<int>(p4y),
                 m_themeColor, TRUE);

    // 3. 上下のエッジライン
    DrawLine(static_cast<int>(p1x - 50), static_cast<int>(p1y - 6), static_cast<int>(p2x + 50), static_cast<int>(p2y - 6), GetColor(255, 235, 30), 4);
    DrawLine(static_cast<int>(p4x - 50), static_cast<int>(p4y + 6), static_cast<int>(p3x + 50), static_cast<int>(p3y + 6), GetColor(255, 255, 255), 4);

    // =========================================================================
    // Stage 3: キャラクターカットイン フレーム (横長スリット型で目元・顔を強調)
    // =========================================================================
    float eyeCenterX = sw * 0.32f + slideX * 0.85f;
    float eyeCenterY = centerY;

    float slitW = 340.0f * scaleAnim;
    float slitH = 190.0f * scaleAnim;
    float slitSkew = 50.0f;

    float s1x = eyeCenterX - slitW * 0.5f + slitSkew, s1y = eyeCenterY - slitH * 0.5f;
    float s2x = eyeCenterX + slitW * 0.5f + slitSkew, s2y = eyeCenterY - slitH * 0.5f;
    float s3x = eyeCenterX + slitW * 0.5f - slitSkew, s3y = eyeCenterY + slitH * 0.5f;
    float s4x = eyeCenterX - slitW * 0.5f - slitSkew, s4y = eyeCenterY + slitH * 0.5f;

    // スリットの漆黒外枠フレーム
    DrawTriangle(static_cast<int>(s1x - 10), static_cast<int>(s1y - 10),
                 static_cast<int>(s2x + 10), static_cast<int>(s2y - 10),
                 static_cast<int>(s3x + 10), static_cast<int>(s3y + 10),
                 GetColor(0, 0, 0), TRUE);
    DrawTriangle(static_cast<int>(s1x - 10), static_cast<int>(s1y - 10),
                 static_cast<int>(s3x + 10), static_cast<int>(s3y + 10),
                 static_cast<int>(s4x - 10), static_cast<int>(s4y + 10),
                 GetColor(0, 0, 0), TRUE);

    // 内側背景（深黒）
    DrawTriangle(static_cast<int>(s1x), static_cast<int>(s1y),
                 static_cast<int>(s2x), static_cast<int>(s2y),
                 static_cast<int>(s3x), static_cast<int>(s3y),
                 GetColor(15, 15, 20), TRUE);
    DrawTriangle(static_cast<int>(s1x), static_cast<int>(s1y),
                 static_cast<int>(s3x), static_cast<int>(s3y),
                 static_cast<int>(s4x), static_cast<int>(s4y),
                 GetColor(15, 15, 20), TRUE);

    // キャラクター画像の描画
    if (m_previewHandle != -1) {
        DrawModiGraph(
            static_cast<int>(s1x), static_cast<int>(s1y),
            static_cast<int>(s2x), static_cast<int>(s2y),
            static_cast<int>(s3x), static_cast<int>(s3y),
            static_cast<int>(s4x), static_cast<int>(s4y),
            m_previewHandle, TRUE
        );
    } else {
        DrawP5EyeIllustration(m_charaId, eyeCenterX, eyeCenterY, scaleAnim * 0.85f, m_themeColor);
    }

    // スリット枠の縁取りライン
    DrawLine(static_cast<int>(s1x), static_cast<int>(s1y), static_cast<int>(s2x), static_cast<int>(s2y), GetColor(255, 235, 40), 4);
    DrawLine(static_cast<int>(s2x), static_cast<int>(s2y), static_cast<int>(s3x), static_cast<int>(s3y), GetColor(255, 255, 255), 4);
    DrawLine(static_cast<int>(s3x), static_cast<int>(s3y), static_cast<int>(s4x), static_cast<int>(s4y), GetColor(255, 235, 40), 4);
    DrawLine(static_cast<int>(s4x), static_cast<int>(s4y), static_cast<int>(s1x), static_cast<int>(s1y), GetColor(255, 255, 255), 4);

    // =========================================================================
    // Stage 4: スタイリッシュタイポグラフィ（帯の高さに合わせたコンパクト配置）
    // =========================================================================
    int textBaseX = static_cast<int>(sw * 0.50f + slideX * 0.95f);
    int textBaseY = static_cast<int>(centerY);

    // 1. "CRITICAL SKILL" バッジ
    DrawBox(textBaseX - 8, textBaseY - 76, textBaseX + 260, textBaseY - 48, GetColor(0, 0, 0), TRUE);
    DrawBox(textBaseX - 6, textBaseY - 74, textBaseX + 258, textBaseY - 50, GetColor(255, 235, 40), TRUE);
    FontManager::GetInstance().DrawString(textBaseX + 10, textBaseY - 72, "CRITICAL SKILL >>", GetColor(0, 0, 0), 20, 9);

    // 2. キャラクター名
    std::string charaDisplay = m_charaName;
    FontManager::GetInstance().DrawString(textBaseX + 4, textBaseY - 42, charaDisplay.c_str(), GetColor(0, 0, 0), 32, 9);
    FontManager::GetInstance().DrawString(textBaseX, textBaseY - 44, charaDisplay.c_str(), GetColor(255, 255, 255), 32, 9);

    // 3. スキル名（ボールド英字）
    std::string skillDisplay = m_skillName;
    std::transform(skillDisplay.begin(), skillDisplay.end(), skillDisplay.begin(), ::toupper);

    FontManager::GetInstance().DrawString(textBaseX + 5, textBaseY - 6, skillDisplay.c_str(), GetColor(0, 0, 0), 48, 9);
    FontManager::GetInstance().DrawString(textBaseX, textBaseY - 8, skillDisplay.c_str(), GetColor(255, 255, 255), 48, 9);

    // =========================================================================
    // Stage 5: 飛び散るコミック破片パーティクル
    // =========================================================================
    for (const auto& s : m_shards) {
        float cosA = std::cos(s.angle);
        float sinA = std::sin(s.angle);
        float r = s.size * 0.8f;

        if (s.type == 0) {
            int t1x = static_cast<int>(s.x + cosA * r);
            int t1y = static_cast<int>(s.y + sinA * r);
            int t2x = static_cast<int>(s.x - sinA * (r * 0.35f));
            int t2y = static_cast<int>(s.y + cosA * (r * 0.35f));
            int t3x = static_cast<int>(s.x - cosA * r);
            int t3y = static_cast<int>(s.y - sinA * r);
            int t4x = static_cast<int>(s.x + sinA * (r * 0.35f));
            int t4y = static_cast<int>(s.y - cosA * (r * 0.35f));

            DrawTriangle(t1x, t1y, t2x, t2y, t3x, t3y, s.color, TRUE);
            DrawTriangle(t1x, t1y, t3x, t3y, t4x, t4y, s.color, TRUE);
        } else if (s.type == 1) {
            int pA_x = static_cast<int>(s.x + cosA * r * 1.2f);
            int pA_y = static_cast<int>(s.y + sinA * r * 1.2f);
            int pB_x = static_cast<int>(s.x - sinA * r * 0.6f);
            int pB_y = static_cast<int>(s.y + cosA * r * 0.6f);
            int pC_x = static_cast<int>(s.x - cosA * r * 0.6f);
            int pC_y = static_cast<int>(s.y - sinA * r * 0.6f);

            DrawTriangle(pA_x, pA_y, pB_x, pB_y, pC_x, pC_y, s.color, TRUE);
        } else {
            DrawCircle(static_cast<int>(s.x), static_cast<int>(s.y), static_cast<int>(r * 0.45f), s.color, TRUE);
        }
    }
}

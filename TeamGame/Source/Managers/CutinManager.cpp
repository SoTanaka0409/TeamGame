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
    // カットイン開始〜決まりの瞬間(0〜34フレーム)は世界を停止(ヒットストップ)
    return m_isActive && (m_timer <= 34);
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
    // Stage 1: P5シグネチャーの「純赤 + 深黒」スピード線背景
    // =========================================================================
    int bgAlpha = 230;
    if (m_timer < 4) bgAlpha = m_timer * 55;
    else if (m_timer > m_maxFrames - 8) bgAlpha = (m_maxFrames - m_timer) * 28;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, bgAlpha);
    // 深紅の背景 (P5テーマの真紅 0xD01020)
    DrawBox(0, 0, sw, sh, GetColor(190, 15, 25), TRUE);
    // コミック調の漆黒集中線
    DrawPersonaSpeedLines(sw, sh, GetColor(10, 10, 15));
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // =========================================================================
    // Stage 2: イナズマのような急角度オランダアングル（Dutch Angle）斜めスラッシュ帯
    // =========================================================================
    // アニメーションカーブ:
    // 0~6F: 画面外から超高速で突き刺さる (スナップ・オーバーシュート)
    // 7~36F: わずかにドリフトしながら静止 (ヒットストップ)
    // 37~54F: 画面右下へ超高速で一気にスライス突破
    float slideX = 0.0f;
    float scaleAnim = 1.0f;

    if (m_timer <= 6) {
        float t = m_timer / 6.0f;
        // イージング(バウンドオーバーシュート)
        slideX = (1.0f - t) * (1.0f - t) * -1600.0f;
        scaleAnim = 1.25f - (t * 0.25f);
    } else if (m_timer <= 36) {
        slideX = (m_timer - 6) * 1.5f; // 微小なドリフト
        scaleAnim = 1.0f;
    } else {
        float t = (m_timer - 36) / 18.0f;
        slideX = t * t * 2000.0f;
        scaleAnim = 1.0f + (t * 0.15f);
    }

    float centerY = 540.0f;
    float bandHeight = 400.0f * scaleAnim;
    float skewX = 260.0f; // P5本家のようなダイナミックな傾き

    // 帯の角頂点
    float p1x = -300.0f + skewX + slideX, p1y = centerY - bandHeight * 0.5f;
    float p2x = sw + 300.0f + skewX + slideX, p2y = centerY - bandHeight * 0.5f - 30.0f;
    float p3x = sw + 300.0f - skewX + slideX, p3y = centerY + bandHeight * 0.5f + 30.0f;
    float p4x = -300.0f - skewX + slideX, p4y = centerY + bandHeight * 0.5f;

    // 1. スラッシュ帯のドロップシャドウ（極太漆黒）
    DrawTriangle(static_cast<int>(p1x - 24), static_cast<int>(p1y - 24),
                 static_cast<int>(p2x + 24), static_cast<int>(p2y - 24),
                 static_cast<int>(p3x + 24), static_cast<int>(p3y + 24),
                 GetColor(0, 0, 0), TRUE);
    DrawTriangle(static_cast<int>(p1x - 24), static_cast<int>(p1y - 24),
                 static_cast<int>(p3x + 24), static_cast<int>(p3y + 24),
                 static_cast<int>(p4x - 24), static_cast<int>(p4y + 24),
                 GetColor(0, 0, 0), TRUE);

    // 2. キャラクター固有テーマカラーの太帯
    DrawTriangle(static_cast<int>(p1x), static_cast<int>(p1y),
                 static_cast<int>(p2x), static_cast<int>(p2y),
                 static_cast<int>(p3x), static_cast<int>(p3y),
                 m_themeColor, TRUE);
    DrawTriangle(static_cast<int>(p1x), static_cast<int>(p1y),
                 static_cast<int>(p3x), static_cast<int>(p3y),
                 static_cast<int>(p4x), static_cast<int>(p4y),
                 m_themeColor, TRUE);

    // 3. P5本家特有の「上下ギザギザカッティング＆イエロー/ホワイトアクセントエッジ」
    DrawLine(static_cast<int>(p1x - 80), static_cast<int>(p1y - 12), static_cast<int>(p2x + 80), static_cast<int>(p2y - 12), GetColor(255, 235, 30), 6);
    DrawLine(static_cast<int>(p4x - 80), static_cast<int>(p4y + 12), static_cast<int>(p3x + 80), static_cast<int>(p3y + 12), GetColor(255, 255, 255), 6);

    // =========================================================================
    // Stage 3: 本家 P5 の核心「キャラクターカットイン フレーム」
    // (正方形イラスト~300x300に最適化されたスタイリッシュなポートレートフレーム)
    // =========================================================================
    float eyeCenterX = sw * 0.35f + slideX * 0.85f;
    float eyeCenterY = centerY;

    // イラストフレームの寸法 (ほぼ正方形~黄金比に近い斜めフレーム)
    float slitW = 390.0f * scaleAnim;
    float slitH = 340.0f * scaleAnim;
    float slitSkew = 70.0f;

    float s1x = eyeCenterX - slitW * 0.5f + slitSkew, s1y = eyeCenterY - slitH * 0.5f;
    float s2x = eyeCenterX + slitW * 0.5f + slitSkew, s2y = eyeCenterY - slitH * 0.5f;
    float s3x = eyeCenterX + slitW * 0.5f - slitSkew, s3y = eyeCenterY + slitH * 0.5f;
    float s4x = eyeCenterX - slitW * 0.5f - slitSkew, s4y = eyeCenterY + slitH * 0.5f;

    // スリットの漆黒外枠フレーム
    DrawTriangle(static_cast<int>(s1x - 14), static_cast<int>(s1y - 14),
                 static_cast<int>(s2x + 14), static_cast<int>(s2y - 14),
                 static_cast<int>(s3x + 14), static_cast<int>(s3y + 14),
                 GetColor(0, 0, 0), TRUE);
    DrawTriangle(static_cast<int>(s1x - 14), static_cast<int>(s1y - 14),
                 static_cast<int>(s3x + 14), static_cast<int>(s3y + 14),
                 static_cast<int>(s4x - 14), static_cast<int>(s4y + 14),
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

    // 画像がロードされている場合はカスタムグラフィックをアスペクト比を維持してフィッティング描画
    if (m_previewHandle != -1) {
        // P5風のシャープな斜めフレームに合わせて描画
        DrawModiGraph(
            static_cast<int>(s1x), static_cast<int>(s1y),
            static_cast<int>(s2x), static_cast<int>(s2y),
            static_cast<int>(s3x), static_cast<int>(s3y),
            static_cast<int>(s4x), static_cast<int>(s4y),
            m_previewHandle, TRUE
        );
    } else {
        // 画像がない場合は本家P5スタイルの精巧な目元劇画イラストをプロシージャル描画
        DrawP5EyeIllustration(m_charaId, eyeCenterX, eyeCenterY, scaleAnim, m_themeColor);
    }

    // スリット枠の白＆黄色二重縁取りライン
    DrawLine(static_cast<int>(s1x), static_cast<int>(s1y), static_cast<int>(s2x), static_cast<int>(s2y), GetColor(255, 235, 40), 5);
    DrawLine(static_cast<int>(s2x), static_cast<int>(s2y), static_cast<int>(s3x), static_cast<int>(s3y), GetColor(255, 255, 255), 5);
    DrawLine(static_cast<int>(s3x), static_cast<int>(s3y), static_cast<int>(s4x), static_cast<int>(s4y), GetColor(255, 235, 40), 5);
    DrawLine(static_cast<int>(s4x), static_cast<int>(s4y), static_cast<int>(s1x), static_cast<int>(s1y), GetColor(255, 255, 255), 5);

    // =========================================================================
    // Stage 4: P5 アメコミ調タイポグラフィ（斜体・切り文字ボックス・二重シャドウ）
    // =========================================================================
    int textBaseX = static_cast<int>(sw * 0.54f + slideX * 0.95f);
    int textBaseY = static_cast<int>(centerY);

    // 1. "CRITICAL SKILL >>" バッジ
    DrawBox(textBaseX - 10, textBaseY - 130, textBaseX + 350, textBaseY - 90, GetColor(0, 0, 0), TRUE);
    DrawBox(textBaseX - 8, textBaseY - 128, textBaseX + 348, textBaseY - 92, GetColor(255, 235, 40), TRUE);
    FontManager::GetInstance().DrawString(textBaseX + 12, textBaseY - 124, "CRITICAL SKILL >>", GetColor(0, 0, 0), 24, 9);

    // 2. キャラクター名（大迫力の切り文字・黒白コントラスト）
    std::string charaDisplay = m_charaName;
    FontManager::GetInstance().DrawString(textBaseX + 6, textBaseY - 80, charaDisplay.c_str(), GetColor(0, 0, 0), 44, 9);
    FontManager::GetInstance().DrawString(textBaseX + 3, textBaseY - 82, charaDisplay.c_str(), GetColor(190, 15, 25), 44, 9);
    FontManager::GetInstance().DrawString(textBaseX, textBaseY - 84, charaDisplay.c_str(), GetColor(255, 255, 255), 44, 9);

    // 3. スキル名（特大ボールド英字・P5風の立体シャドウ）
    std::string skillDisplay = m_skillName;
    std::transform(skillDisplay.begin(), skillDisplay.end(), skillDisplay.begin(), ::toupper);

    // 多重ドロップシャドウで雑誌・アメコミの飛び出す文字を完全再現
    FontManager::GetInstance().DrawString(textBaseX + 8, textBaseY - 18, skillDisplay.c_str(), GetColor(0, 0, 0), 62, 9);
    FontManager::GetInstance().DrawString(textBaseX + 4, textBaseY - 22, skillDisplay.c_str(), GetColor(20, 20, 25), 62, 9);
    FontManager::GetInstance().DrawString(textBaseX, textBaseY - 26, skillDisplay.c_str(), GetColor(255, 255, 255), 62, 9);

    // サブキャプション
    FontManager::GetInstance().DrawString(textBaseX + 2, textBaseY + 54, "[ SHOW ME YOUR TRUE FORM ]", GetColor(0, 0, 0), 20, 6);
    FontManager::GetInstance().DrawString(textBaseX, textBaseY + 52, "[ SHOW ME YOUR TRUE FORM ]", GetColor(255, 235, 40), 20, 6);

    // =========================================================================
    // Stage 5: 飛び散るコミック破片・星・インク飛沫パーティクル
    // =========================================================================
    for (const auto& s : m_shards) {
        float cosA = std::cos(s.angle);
        float sinA = std::sin(s.angle);
        float r = s.size;

        if (s.type == 0) {
            // P5風 4芒星（ダイヤスター）
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
            DrawTriangle(t1x, t1y, t2x, t2y, t3x, t3y, GetColor(0, 0, 0), FALSE);
            DrawTriangle(t1x, t1y, t3x, t3y, t4x, t4y, GetColor(0, 0, 0), FALSE);
        } else if (s.type == 1) {
            // 鋭角な三角ガラス・紙片
            int pA_x = static_cast<int>(s.x + cosA * r * 1.2f);
            int pA_y = static_cast<int>(s.y + sinA * r * 1.2f);
            int pB_x = static_cast<int>(s.x - sinA * r * 0.6f);
            int pB_y = static_cast<int>(s.y + cosA * r * 0.6f);
            int pC_x = static_cast<int>(s.x - cosA * r * 0.6f);
            int pC_y = static_cast<int>(s.y - sinA * r * 0.6f);

            DrawTriangle(pA_x, pA_y, pB_x, pB_y, pC_x, pC_y, s.color, TRUE);
            DrawTriangle(pA_x, pA_y, pB_x, pB_y, pC_x, pC_y, GetColor(0, 0, 0), FALSE);
        } else {
            // インク滴・スプラッター丸
            DrawCircle(static_cast<int>(s.x), static_cast<int>(s.y), static_cast<int>(r * 0.45f), s.color, TRUE);
            DrawCircle(static_cast<int>(s.x), static_cast<int>(s.y), static_cast<int>(r * 0.45f), GetColor(0, 0, 0), FALSE);
        }
    }

    // =========================================================================
    // Stage 6: 激しいインパクトフレーム（白フラッシュ ＆ 黒反転）
    // =========================================================================
    if (m_timer <= 3) {
        // 発動0~3フレームの激しい純白閃光
        int flashAlpha = (4 - m_timer) * 70;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, flashAlpha);
        DrawBox(0, 0, sw, sh, GetColor(255, 255, 255), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    } else if (m_timer == 4) {
        // 1フレームだけの黒反転（P5の打撃衝撃フレーム技法）
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
        DrawBox(0, 0, sw, sh, GetColor(0, 0, 0), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}

#pragma once

#include <string>
#include <vector>
#include <DxLib.h>

/**
 * @brief ペルソナ5本家同様のスタイリッシュなスキル目元カットイン演出を管理するクラス (シングルトン)
 */
class CutinManager {
public:
    static CutinManager& GetInstance();

    /**
     * @brief スキル発動時のカットイン演出を開始する
     * @param characterId キャラクターID (1:指揮者, 2:反逆者, 3:バカ, 4:主婦, 5:悪魔)
     * @param characterName キャラクター名
     * @param skillName スキル名
     * @param themeColor キャラクタースラッシュ帯のテーマカラー
     * @param previewImgPath プレビュー画像パス
     */
    void StartCutin(int characterId, const std::string& characterName, const std::string& skillName, unsigned int themeColor, const std::string& previewImgPath = "");

    /**
     * @brief 毎フレームの更新処理
     */
    void Update();

    /**
     * @brief 画面最前面への描画処理
     */
    void Draw();

    /**
     * @brief カットイン演出中かどうか
     */
    bool IsActive() const { return m_isActive; }

    /**
     * @brief ゲーム内オブジェクトの更新を一時停止（ヒットストップ）するべきか
     */
    bool ShouldPauseGame() const;

private:
    CutinManager();
    ~CutinManager();

    // 内部描画ヘルパー
    void DrawPersonaSpeedLines(int sw, int sh, unsigned int baseColor);
    void DrawP5EyeIllustration(int charaId, float centerX, float centerY, float scale, unsigned int themeColor);
    void DrawComicHalftoneBand(float p1x, float p1y, float p2x, float p2y, float p3x, float p3y, float p4x, float p4y);

    bool m_isActive = false;
    int m_timer = 0;
    int m_maxFrames = 26; // 約0.43秒間のスピーディな演出

    int m_charaId = 1;
    std::string m_charaName;
    std::string m_skillName;
    unsigned int m_themeColor = 0xFF0000;
    int m_previewHandle = -1;
    std::string m_loadedPath;

    // スプラッシュ・トゲトゲ・星型コミック破片パーティクル
    struct Shard {
        float x, y;
        float vx, vy;
        float size;
        float angle;
        float vRot;
        int type; // 0: ダイヤ星, 1: 三角破片, 2: インク滴
        unsigned int color;
    };
    std::vector<Shard> m_shards;

    // 放射集中線データ
    struct SpeedLine {
        float angle;
        float length;
        float width;
        float offset;
    };
    std::vector<SpeedLine> m_speedLines;
};

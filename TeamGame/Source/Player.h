#pragma once

#include "Object2D.h"

class Stage;

class Player
{
public:
    Player();
    ~Player() = default;

    void Initialize(float startX, float startY);

    // プレイヤー移動・壁衝突判定・右クリックライトON/OFF・草むら隠れ更新
    void Update(const Stage& stage, int screenWidth, int screenHeight, float startDrawX, float startDrawY, float cellSize);

    // 懐中電灯・スポットライト（水と草は光透過・右クリック消灯・壁のみ光遮蔽）
    void RenderLightMask(const Stage& stage, int rectX, int rectY, int rectW, int rectH, float startDrawX, float startDrawY, float cellSize) const;

    // プレイヤーP本体の描画 (草むら潜伏時は隠れる演出)
    void Draw(float startDrawX, float startDrawY, float cellSize, bool isDebugMode = false) const;

    // ゲッター & セッター
    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    void SetPosition(float x, float y) { m_x = x; m_y = y; }

    float GetLightAngle() const { return m_lightAngle; }
    float GetRadius() const { return m_radius; }

    bool IsLightOn() const { return m_isLightOn; }
    void ToggleLight() { m_isLightOn = !m_isLightOn; }

    bool IsInBush() const { return m_isInBush; }

private:
    float m_x = 0.0f;           // グリッド単位のX座標
    float m_y = 0.0f;           // グリッド単位のY座標
    float m_speed = 0.20f;      // 移動速度 (セル/フレーム)
    float m_radius = 0.55f;     // プレイヤー描画サイズ
    float m_lightAngle = 0.0f;  // マウス照準の角度 (ラジアン)

    bool m_isLightOn = true;    // 右クリックで切り替わる懐中電灯スイッチ (ON/OFF)
    bool m_isInBush = false;    // 草むらの中に隠れているか

    bool m_prevMouseRight = false; // 右クリックエッジ検出用

    // 懐中電灯・視界ライティングのパラメーター
    float m_maxSpotDistCells = 14.0f; // 60°スポットライトの射程 (セル数)
    float m_closeRadiusCells = 2.0f;  // 足元のやや見える円形明かりの半径 (セル数)
    float m_fanAngleHalf = 0.5236f;   // 視野角 60度 (±30度)
};

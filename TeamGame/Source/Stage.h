#pragma once

#include <vector>
#include "Object2D.h"

// 地形タイルの種類
enum class CellType
{
    EMPTY_FLOOR = 0, // オレンジ・サンド床
    WALL_BLOCK,      // 茶色の木箱・丸太ブロック壁（通行不可）
    OUTER_WALL,      // 外周封鎖壁（絶対通行不可の外枠）
    BUSH,            // 草むら・茂み（Grass1.pngテクスチャ適用・当たり判定あり）
    WATER,           // 青い水場・池（通行不可）
    CACTUS           // サボテン・たる障害物（通行不可）
};

// スポーン・目的要素の種類
enum class SpawnType
{
    NONE = 0,
    CENTER_STAR,     // マップ中央の青いスター/エメラルド
    ITEM_POWERUP     // アイテム
};

struct SpawnPoint
{
    Point2D gridPos;
    SpawnType type;
};

class Stage
{
public:
    Stage();
    ~Stage() = default;

    void Initialize(int width, int height);

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

    CellType GetCell(int x, int y) const;
    void SetCell(int x, int y, CellType type);

    void AddSpawnPoint(Point2D pos, SpawnType type);
    const std::vector<SpawnPoint>& GetSpawnPoints() const { return m_spawnPoints; }

    Point2D GetPlayerStartPos() const { return m_playerStartPos; }
    void SetPlayerStartPos(Point2D pos) { m_playerStartPos = pos; }

    bool IsSolidWall(int gridX, int gridY) const;
    bool IsOutOfBounds(int gridX, int gridY) const;

    void SetGrassGraph(int handle) { m_hGrassGraph = handle; }

    // 1920x1080 フィッティング描画（Grass1.png 画像テクスチャ描画対応）
    void DrawFitToArea(int rectX, int rectY, int rectW, int rectH, bool isDebugMode, float playerX, float playerY, float lightAngle = 0.0f, const char* patternName = "", int hGrass = -1) const;

private:
    int m_width = 0;   // 横セル数 (48)
    int m_height = 0;  // 縦セル数 (27)
    std::vector<CellType> m_grid;
    std::vector<SpawnPoint> m_spawnPoints;
    Point2D m_playerStartPos{ 3, 13 };
    int m_hGrassGraph = -1;
};

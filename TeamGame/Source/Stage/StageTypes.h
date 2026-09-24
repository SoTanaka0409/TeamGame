#pragma once

// 2D位置ベクトル・点構造体
struct Vector2D
{
    float x = 0.0f;
    float y = 0.0f;

    Vector2D() = default;
    Vector2D(float _x, float _y) : x(_x), y(_y) {}
};

// 整数のグリッド座標構造体
struct Point2D
{
    int x = 0;
    int y = 0;

    Point2D() = default;
    Point2D(int _x, int _y) : x(_x), y(_y) {}

    bool operator==(const Point2D& other) const {
        return x == other.x && y == other.y;
    }
};

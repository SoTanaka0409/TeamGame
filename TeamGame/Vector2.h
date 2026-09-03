#pragma once

struct Vector2
{
    float x;
    float y;

    Vector2() : x(0.0f), y(0.0f)
    {
    }
    Vector2(float x, float y) : x(x), y(y)
    {
    }

    Vector2 operator+(const Vector2 &other) const
    {
        return Vector2(x + other.x, y + other.y);
    }
    Vector2 operator-(const Vector2 &other) const
    {
        return Vector2(x - other.x, y - other.y);
    }
    Vector2 &operator+=(const Vector2 &other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }
    Vector2 &operator-=(const Vector2 &other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }
    Vector2 operator*(float scalar) const
    {
        return Vector2(x * scalar, y * scalar);
    }
    Vector2 operator/(float scalar) const
    {
        return Vector2(x / scalar, y / scalar);
    }
};

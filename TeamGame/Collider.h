#pragma once

class Object2D;

enum class ColliderType
{
    Rectangle,
    Circle
};

class Collider
{
  private:
    ColliderType type;
    Object2D *owner;
    float radius;        // For circle
    float width, height; // For rectangle
    float offsetX, offsetY;

  public:
    Collider(Object2D *owner, float radius);     // Circle constructor
    Collider(Object2D *owner, float w, float h); // Rect constructor
    ~Collider();

    ColliderType GetType() const
    {
        return type;
    }
    Object2D *GetOwner() const
    {
        return owner;
    }

    float GetWorldX() const;
    float GetWorldY() const;
    float GetRadius() const
    {
        return radius;
    }
    float GetWidth() const
    {
        return width;
    }
    float GetHeight() const
    {
        return height;
    }

    bool IsCollision(Collider *other) const;
};

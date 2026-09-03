#pragma once

class Object2D
{
  protected:
    float x, y;
    float width, height;
    bool isActive;

  public:
    Object2D();
    virtual ~Object2D();

    virtual void Update() = 0;
    virtual void Draw() = 0;

    float GetX() const
    {
        return x;
    }
    float GetY() const
    {
        return y;
    }
    float GetWidth() const
    {
        return width;
    }
    float GetHeight() const
    {
        return height;
    }

    bool IsActive() const
    {
        return isActive;
    }
    void SetActive(bool active)
    {
        isActive = active;
    }
};

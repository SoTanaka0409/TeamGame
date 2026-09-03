#pragma once
#include "Object2D.h"

class Player : public Object2D
{
  private:
    float speed;
    float radius;

  public:
    Player(float startX, float startY);
    virtual ~Player();

    void Update() override;
    void Draw() override;
};

#pragma once

class BaseScene
{
  public:
    BaseScene()
    {
    }
    virtual ~BaseScene()
    {
    }
    virtual void Init() = 0;
    virtual void Update() = 0;
    virtual void Draw() = 0;
    virtual void Finalize() = 0;
};

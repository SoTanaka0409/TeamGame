#pragma once
#include <vector>

// 簡易エフェクト管理クラス (基本形式)
class Effect
{
  public:
    float x, y;
    int lifeTimer;
    int maxLife;

    Effect(float x, float y, int life)
        : x(x), y(y), lifeTimer(life), maxLife(life)
    {
    }
    virtual ~Effect()
    {
    }
    virtual void Update()
    {
        if (lifeTimer > 0)
            lifeTimer--;
    }
    virtual void Draw() = 0;
    bool IsDead() const
    {
        return lifeTimer <= 0;
    }
};

class EffectManager
{
  private:
    std::vector<Effect *> effects;

  public:
    EffectManager();
    ~EffectManager();

    void AddEffect(Effect *effect);
    void Update();
    void Draw();
    void Clear();
};

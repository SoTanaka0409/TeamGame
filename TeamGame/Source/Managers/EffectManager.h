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

class BloodParticle : public Effect
{
  public:
    float vx, vy;
    float size;
    unsigned int color;

    BloodParticle(float startX, float startY, float velX, float velY, float pSize, int life, unsigned int col)
        : Effect(startX, startY, life), vx(velX), vy(velY), size(pSize), color(col)
    {
    }

    void Update() override;
    void Draw() override;
};

class KnifeSlashEffect : public Effect
{
  public:
    float angle;
    KnifeSlashEffect(float startX, float startY, float dirAngle)
        : Effect(startX, startY, 12), angle(dirAngle)
    {
    }

    void Draw() override;
};

class MuzzleFlashEffect : public Effect
{
  public:
    float angle;
    float flashRadius;

    MuzzleFlashEffect(float startX, float startY, float dirAngle, float radius = 18.0f)
        : Effect(startX, startY, 6), angle(dirAngle), flashRadius(radius)
    {
    }

    void Draw() override;
};

class SparkParticle : public Effect
{
  public:
    float vx, vy;
    float size;
    unsigned int color;

    SparkParticle(float startX, float startY, float velX, float velY, float pSize, int life, unsigned int col)
        : Effect(startX, startY, life), vx(velX), vy(velY), size(pSize), color(col)
    {
    }

    void Update() override;
    void Draw() override;
};

class EffectManager
{
  private:
    std::vector<Effect *> effects;
    int gunFlashTimer = 0;
    float lastFlashWorldX = 0.0f;
    float lastFlashWorldY = 0.0f;

  public:
    EffectManager();
    ~EffectManager();

    void AddEffect(Effect *effect);
    void AddBloodEffect(float worldX, float worldY, int count = 10);
    void AddKnifeSlashEffect(float worldX, float worldY, float dirAngle);
    void AddMuzzleFlashEffect(float worldX, float worldY, float dirAngle, float radius = 18.0f);
    void TriggerGunFlash(float worldX, float worldY, int duration = 5)
    {
        gunFlashTimer = duration;
        lastFlashWorldX = worldX;
        lastFlashWorldY = worldY;
    }
    int GetGunFlashTimer() const { return gunFlashTimer; }
    float GetLastFlashWorldX() const { return lastFlashWorldX; }
    float GetLastFlashWorldY() const { return lastFlashWorldY; }
    void Update();
    void Draw();
    void Clear();
};


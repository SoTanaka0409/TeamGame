#pragma once
#include "ColliderManager.h"
#include "ObjectManager.h"
#include "EffectManager.h"

class Scene
{
  protected:
    ObjectManager *objectManager;
    ColliderManager *colliderManager;
    EffectManager *effectManager;

  public:
    Scene();
    virtual ~Scene();

    virtual void Init()
    {
    }
    virtual void Update();
    virtual void Draw();
    virtual void Finalize()
    {
    }

    ObjectManager *GetObjectManager() const
    {
        return objectManager;
    }
    ColliderManager *GetColliderManager() const
    {
        return colliderManager;
    }
    EffectManager *GetEffectManager() const
    {
        return effectManager;
    }

    virtual const class Stage* GetStage() const
    {
        return nullptr;
    }
};

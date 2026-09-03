#pragma once
#include "ColliderManager.h"
#include "ObjectManager.h"

class Scene
{
  protected:
    ObjectManager *objectManager;
    ColliderManager *colliderManager;

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
};

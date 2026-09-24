#include "Scene.h"

Scene::Scene()
{
    objectManager = new ObjectManager();
    colliderManager = new ColliderManager();
    effectManager = new EffectManager();
}

Scene::~Scene()
{
    if (colliderManager)
    {
        colliderManager->Clear();
    }
    delete objectManager;
    delete colliderManager;
    delete effectManager;
}

void Scene::Update()
{
    objectManager->Update();
    colliderManager->CheckAllCollisions();
    objectManager->RemoveDestroyedObjects();
    effectManager->Update();
}

void Scene::Draw()
{
    objectManager->Draw();
    effectManager->Draw();
}

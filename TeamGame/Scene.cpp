#include "Scene.h"

Scene::Scene()
{
    objectManager = new ObjectManager();
    colliderManager = new ColliderManager();
}

Scene::~Scene()
{
    delete objectManager;
    delete colliderManager;
}

void Scene::Update()
{
    objectManager->Update();
    colliderManager->CheckAllCollisions();
    objectManager->RemoveDestroyedObjects();
}

void Scene::Draw()
{
    objectManager->Draw();
}

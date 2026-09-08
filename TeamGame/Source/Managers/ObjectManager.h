#pragma once
#include <vector>

class Object2D;

class ObjectManager
{
  private:
    std::vector<Object2D *> objects;

  public:
    ObjectManager();
    ~ObjectManager();

    void AddObject(Object2D *obj);
    void Update();
    void Draw();
    void RemoveDestroyedObjects();
    void Clear();
};

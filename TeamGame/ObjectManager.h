#pragma once
#include <vector>

class Object2D;

class ObjectManager
{
  private:
    std::vector<Object2D *> objects;

    ObjectManager();
    ~ObjectManager();

  public:
    static ObjectManager &GetInstance()
    {
        static ObjectManager instance;
        return instance;
    }

    ObjectManager(const ObjectManager &) = delete;
    ObjectManager &operator=(const ObjectManager &) = delete;

    void AddObject(Object2D *obj);
    void Update();
    void Draw();
    void RemoveDestroyedObjects();
    void Clear();
};

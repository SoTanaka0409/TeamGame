#include "Object2D.h"
#include "ObjectManager.h"

Object2D::Object2D()
    : x(0.0f), y(0.0f), width(0.0f), height(0.0f), isActive(true)
{
    ObjectManager::GetInstance().AddObject(this);
}

Object2D::~Object2D()
{
}

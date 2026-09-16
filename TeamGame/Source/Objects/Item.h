#pragma once
#include "Object2D.h"
#include "../Colliders/CircleCollider.h"

enum class ItemType {
    Health,
    Ammo
};

class Item : public Object2D
{
private:
    CircleCollider* collider;
    ItemType type;
    int amount;
    float floatOffset;
    float time;

public:
    Item(float startX, float startY, ItemType itemType, int amountValue);
    ~Item();

    void Update() override;
    void Draw() override;
    void OnCollisionEnter(Collider* otherCollider) override;

    ItemType GetItemType() const { return type; }
    int GetAmount() const { return amount; }
};

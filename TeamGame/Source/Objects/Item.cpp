#include "Item.h"
#include "../Core/Camera.h"
#include "../Characters/Player.h"
#include "../Managers/SceneManager.h"
#include "../Scenes/Scene.h"
#include "../Managers/SoundManager.h"
#include "DxLib.h"
#include <cmath>

Item::Item(float startX, float startY, ItemType itemType, int amountValue)
    : Object2D(ObjectTag::Item), type(itemType), amount(amountValue), floatOffset(0.0f), time(0.0f)
{
    position = Vector2(startX, startY);
    width = 30.0f;
    height = 30.0f;
    collider = new CircleCollider(this, 15.0f, "Item");

    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (scene && scene->GetColliderManager())
    {
        scene->GetColliderManager()->AddCollider(collider);
    }
}

Item::~Item()
{
    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (scene && scene->GetColliderManager())
    {
        scene->GetColliderManager()->RemoveCollider(collider);
    }
    delete collider;
}

void Item::Update()
{
    time += 0.05f;
    floatOffset = std::sin(time) * 5.0f; // ぷかぷか浮くアニメーション
}

void Item::Draw()
{
    float screenX = Camera::WorldToScreenX(position.x);
    float screenY = Camera::WorldToScreenY(position.y + floatOffset);

    // アイテムの種類に応じて色を変える（回復＝緑、弾薬＝黄色）
    unsigned int color = (type == ItemType::Health) ? GetColor(50, 255, 50) : GetColor(255, 200, 50);
    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY), 15, color, TRUE);
    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY), 15, GetColor(255, 255, 255), FALSE);
}

void Item::OnCollisionEnter(Collider* otherCollider)
{
    // プレイヤーが触れたら効果を発動して消滅する
    if (otherCollider->GetTag() == "Player")
    {
        Player* player = dynamic_cast<Player*>(otherCollider->GetOwner());
        if (player)
        {
            if (type == ItemType::Health) {
                player->status.Heal(amount);
            }
            else if (type == ItemType::Ammo) {
                player->AddAmmo(amount);
            }
            // 取得SEを鳴らす
            SoundManager::GetInstance().Play3D("item_get", position, 500.0f);
        }
        SetActive(false); // 取得したら消滅
    }
}

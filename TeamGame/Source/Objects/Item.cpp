#include "Item.h"
#include "../Core/Camera.h"
#include "../Characters/Player.h"
#include "../Characters/Character.h"
#include "../Characters/Enemy.h"
#include "../Managers/SceneManager.h"
#include "../Scenes/Scene.h"
#include "../Scenes/GameScene.h"
#include "../Managers/SoundManager.h"
#include "../Managers/ItemManager.h"
#include "DxLib.h"
#include <cmath>

Item::Item(float startX, float startY, ItemType itemType, int amountValue)
    : Object2D(ObjectTag::Item), type(itemType), amount(amountValue), floatOffset(0.0f), time(0.0f)
{
    position = Vector2(startX, startY);
    std::string keyName = (type == ItemType::Health) ? "Health" : ((type == ItemType::Ammo) ? "Ammo" : "HorrorTrap");
    const ItemData* data = ItemManager::GetInstance().GetItemData(keyName);
    
    float colRadius = 15.0f;
    if (data)
    {
        if (amountValue <= 0)
        {
            amount = data->amount;
        }
        colRadius = data->radius;
    }

    width = colRadius * 2.0f;
    height = colRadius * 2.0f;
    collider = new CircleCollider(this, colRadius, "Item");

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
    if (type == ItemType::HorrorTrap) {
        // おどろおどろしい黒赤いモヤのような見た目
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180 + (int)(std::sin(time * 2) * 50));
        DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY), 15 + (int)(std::sin(time*3)*3), GetColor(20, 0, 0), TRUE);
        DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY), 10, GetColor(80, 0, 0), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    } else {
        unsigned int color = (type == ItemType::Health) ? GetColor(50, 255, 50) : GetColor(255, 200, 50);
        DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY), 15, color, TRUE);
        DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY), 15, GetColor(255, 255, 255), FALSE);
    }
}

void Item::OnCollisionEnter(Collider* otherCollider)
{
    if (otherCollider->GetTag() == "Player" || otherCollider->GetTag() == "Enemy" || otherCollider->GetTag() == "PlayerBody" || otherCollider->GetTag() == "EnemyBody")
    {
        Character* character = dynamic_cast<Character*>(otherCollider->GetOwner());
        if (character)
        {
            if (type == ItemType::Health && character->status.GetCurrentHp() >= character->status.GetMaxHp()) {
                return;
            }

            std::string keyName = (type == ItemType::Health) ? "Health" : ((type == ItemType::Ammo) ? "Ammo" : "HorrorTrap");
            const ItemData* data = ItemManager::GetInstance().GetItemData(keyName);
            std::string soundEffect = (type == ItemType::HorrorTrap) ? "trap_scare" : "item_get";
            if (data && !data->soundEffect.empty()) {
                soundEffect = data->soundEffect;
            }

            if (type == ItemType::HorrorTrap) {
                // 爆音を鳴らして敵AIをすべて引き寄せる
                SoundManager::GetInstance().Play3D(soundEffect.c_str(), position, 9999.0f, 1.0f, -1);
                
                // もし踏んだのがプレイヤーなら、ホラーエフェクトを発動
                if (character->GetObjectTag() == ObjectTag::Player) {
                    auto gameScene = std::dynamic_pointer_cast<GameScene>(SceneManager::GetInstance().GetCurrentScene());
                    if (gameScene) {
                        gameScene->TriggerHorrorEffect(180); // 3秒間
                    }
                }
            } else if (type == ItemType::Health) {
                character->status.Heal(amount);
                SoundManager::GetInstance().Play3D(soundEffect.c_str(), position, 1500.0f, 1.0f, character->teamId);
            }
            else if (type == ItemType::Ammo) {
                Player* player = dynamic_cast<Player*>(character);
                if (player) player->AddAmmo(amount);
                SoundManager::GetInstance().Play3D(soundEffect.c_str(), position, 1500.0f, 1.0f, character->teamId);
            }
            SetActive(false);
        }
    }
}
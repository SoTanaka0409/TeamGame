#include "CharacterFactory.h"
#include "../Characters/Player.h"
#include "../Characters/Enemy.h"
#include "../Weapons/Handgun.h"
#include "../Weapons/Shotgun.h"
#include "../Weapons/SniperRifle.h"
#include "../Weapons/SubmachineGun.h"
#include "../Stage/Stage.h"
#include "../Managers/ObjectManager.h"
#include "PlayerSelection.h"
#include <algorithm>
#include <random>

Player* CharacterFactory::CreatePlayer(float startX, float startY, Stage* stage, float cellSize, int teamId) {
    Player* player = new Player(startX, startY);
    player->teamId = teamId;
    player->SetStage(stage, cellSize);

    const auto& selection = PlayerSelectionManager::GetInstance().GetSelection();
    const auto& charaData = selection.selectedCharacter;

    player->status.Init(charaData.maxHp, charaData.moveSpeed, charaData.attackPower);
    player->SetThemeColor(charaData.themeColor);
    player->SetColliderRadius(charaData.colliderRadius);
    player->SetModelScale(charaData.modelScale);
    player->SetCharacterId(charaData.id);
    player->SetCharacterName(charaData.name.empty() ? "プレイヤー" : charaData.name);
    player->SetCutinImagePath(charaData.cutinImagePath);
    player->SetModelPath(charaData.modelPath.empty() ? "Resource/GreenCharacter/character.mv1" : charaData.modelPath);
    player->SetEquippedSkillById(charaData.skillId);

    // 選択された武器 (スロット1: 固定ハンドガン, スロット2: 自由選択武器) を装備
    std::vector<Weapon*> playerWeapons;
    playerWeapons.push_back(CreateWeaponByName(selection.slot1Weapon.name));
    playerWeapons.push_back(CreateWeaponByName(selection.slot2Weapon.name));
    player->SetWeapons(playerWeapons);

    // 注意: ここでは objectManager->AddObject(player) を行いません (呼び出し側での重複登録・メモリ破壊を防ぐため)
    return player;
}

Enemy* CharacterFactory::CreateBot(float startX, float startY, int teamId, int characterId, Stage* stage, float cellSize) {
    Enemy* bot = new Enemy(startX, startY, teamId);
    bot->SetStage(stage, cellSize);
    bot->SetCharacterId(characterId);

    auto availableCharas = PlayerSelectionManager::GetAvailableCharacters();
    bool found = false;
    for (const auto& chara : availableCharas) {
        if (chara.id == characterId) {
            // Botのステータス(CSV設定値)を適用
            float botSpeed = chara.moveSpeed;
            if (botSpeed <= 0.1f) botSpeed = 0.5f; // 異常値防止ガード

            bot->status.Init(chara.maxHp, botSpeed, chara.attackPower / 2); // Bot攻撃力半分
            bot->SetColliderRadius(chara.colliderRadius);
            bot->SetModelScale(chara.modelScale);
            std::string teamPrefix = (teamId == 0) ? "[味方] " : "[敵] ";
            bot->SetCharacterName(teamPrefix + chara.name);
            if (!chara.modelPath.empty()) {
                bot->SetModelPath(chara.modelPath);
            }
            found = true;
            break;
        }
    }
    if (!found) {
        bot->status.Init(33, 1.8f, 5);
        bot->SetColliderRadius(8.9f);
        bot->SetModelScale(0.3f);
        bot->SetCharacterName((teamId == 0) ? "[味方] ボット" : "[敵] ボット");
    }
    if (!bot->HasModel()) {
        bot->SetModelPath("Resource/GreenCharacter/character.mv1");
    }

    return bot;
}

std::vector<int> CharacterFactory::GenerateUniqueCharacterIds(int count, int excludeId) {
    std::vector<int> candidateIds;
    auto availableCharas = PlayerSelectionManager::GetAvailableCharacters();

    for (const auto& chara : availableCharas) {
        if (chara.id != excludeId) {
            candidateIds.push_back(chara.id);
        }
    }

    for (size_t i = candidateIds.size(); i > 1; --i) {
        size_t j = static_cast<size_t>(std::rand()) % i;
        std::swap(candidateIds[i - 1], candidateIds[j]);
    }

    if (candidateIds.size() > static_cast<size_t>(count)) {
        candidateIds.resize(count);
    }

    return candidateIds;
}

Weapon* CharacterFactory::CreateWeaponByName(const std::string& name) {
    const auto& selection = PlayerSelectionManager::GetInstance().GetSelection();
    int charaId = selection.selectedCharacter.id;
    return CreateWeaponForCharacter(name, charaId);
}

Weapon* CharacterFactory::CreateWeaponForCharacter(const std::string& name, int characterId) {
    if (name.find("Shotgun") != std::string::npos) return new Shotgun();
    if (name.find("Sniper") != std::string::npos) return new SniperRifle();
    if (name.find("Submachine") != std::string::npos || name.find("SMG") != std::string::npos || name.find("Assault") != std::string::npos) {
        // キャラクター別の派生SMG（パラメータ＆弾・銃身の描画アセット）を生成
        return SubmachineGunFactory::CreateForCharacter(characterId);
    }
    return new Handgun();
}

void CharacterFactory::SpawnMatchBots(const Stage& stage, float cellSize, Player* player, ObjectManager* objectManager, std::vector<Enemy*>& enemiesOut) {
    int midX = stage.GetWidth() / 2;
    int team0Y = stage.GetHeight() - 4;
    int team1Y = 3;

    // プレイヤーの位置を初期位置に設定
    if (player) {
        player->SetPosition(Vector2((midX + 0.5f) * cellSize, (team0Y + 0.5f) * cellSize));
    }

    std::string playerSlot2 = PlayerSelectionManager::GetInstance().GetSelection().slot2Weapon.name;

    // Team 0（味方チーム）: プレイヤーの選択サブ武器以外の2種類を味方ボット2体に割り当て
    std::vector<std::string> team0Available = {"Handgun", "Shotgun", "SniperRifle"};
    std::vector<std::string> allyBotWeapons;
    for (const auto& wName : team0Available)
    {
        if (wName != playerSlot2 && playerSlot2.find(wName) == std::string::npos)
        {
            allyBotWeapons.push_back(wName);
        }
    }
    if (allyBotWeapons.size() < 2)
    {
        allyBotWeapons = {"Shotgun", "SniperRifle"};
    }

    // Ally bots (Team 0)
    int allyOffsets[] = {-3, 3};
    int allyCharaIds[] = {1, 3}; // 指揮者(Tactical), バカ(Assault)
    for (int i = 0; i < 2; ++i)
    {
        int charaId = allyCharaIds[i];
        Enemy* allyBot = CreateBot((midX + allyOffsets[i] + 0.5f) * cellSize, (team0Y + 0.5f) * cellSize, 0, charaId, const_cast<Stage*>(&stage), cellSize);
        std::string wName = (i < static_cast<int>(allyBotWeapons.size())) ? allyBotWeapons[i] : "Handgun";
        allyBot->SetEquippedWeapon(CreateWeaponForCharacter(wName, charaId));
        enemiesOut.push_back(allyBot);
        if (objectManager) objectManager->AddObject(allyBot);
    }

    // Team 1（敵チーム）: Handgun, Shotgun, SniperRifle (およびキャラ別SMG)
    std::vector<std::string> enemyBotWeapons = {"Handgun", "Shotgun", "SniperRifle"};
    for (size_t i = enemyBotWeapons.size(); i > 1; --i) {
        size_t j = static_cast<size_t>(std::rand()) % i;
        std::swap(enemyBotWeapons[i - 1], enemyBotWeapons[j]);
    }

    // Enemy bots (Team 1): 反逆者(2), 主婦(4), 悪魔(5) などの多彩な個性
    int enemyOffsets[] = {-3, 0, 3};
    int enemyCharaIds[] = {2, 4, 5};
    for (int i = 0; i < 3; ++i)
    {
        int charaId = enemyCharaIds[i];
        Enemy* enemyBot = CreateBot((midX + enemyOffsets[i] + 0.5f) * cellSize, (team1Y + 0.5f) * cellSize, 1, charaId, const_cast<Stage*>(&stage), cellSize);
        std::string wName = (i < static_cast<int>(enemyBotWeapons.size())) ? enemyBotWeapons[i] : "Handgun";
        enemyBot->SetEquippedWeapon(CreateWeaponForCharacter(wName, charaId));
        enemiesOut.push_back(enemyBot);
        if (objectManager) objectManager->AddObject(enemyBot);
    }
}

#include "CharacterFactory.h"
#include "../Characters/Player.h"
#include "../Characters/Enemy.h"
#include "../Weapons/Handgun.h"
#include "../Weapons/Shotgun.h"
#include "../Weapons/SniperRifle.h"
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

    player->status.Init(charaData.maxHp, charaData.moveSpeed, 10);
    player->SetThemeColor(charaData.themeColor);
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

    auto availableCharas = PlayerSelectionManager::GetAvailableCharacters();
    for (const auto& chara : availableCharas) {
        if (chara.id == characterId) {
            bot->status.Init(chara.maxHp, chara.moveSpeed, 10);
            bot->SetCharacterName(chara.name);
            if (!chara.modelPath.empty()) {
                bot->SetModelPath(chara.modelPath);
            }
            break;
        }
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
    if (name.find("Shotgun") != std::string::npos) return new Shotgun();
    if (name.find("Sniper") != std::string::npos) return new SniperRifle();
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
    for (int i = 0; i < 2; ++i)
    {
        Enemy* allyBot = new Enemy((midX + allyOffsets[i] + 0.5f) * cellSize, (team0Y + 0.5f) * cellSize, 0);
        allyBot->SetStage(const_cast<Stage*>(&stage), cellSize);
        allyBot->SetModelPath("Resource/GreenCharacter/character.mv1");
        std::string wName = (i < static_cast<int>(allyBotWeapons.size())) ? allyBotWeapons[i] : "Handgun";
        allyBot->SetEquippedWeapon(CreateWeaponByName(wName));
        enemiesOut.push_back(allyBot);
        if (objectManager) objectManager->AddObject(allyBot);
    }

    // Team 1（敵チーム）: Handgun, Shotgun, SniperRifle の3種類を重複なく固定配分
    std::vector<std::string> enemyBotWeapons = {"Handgun", "Shotgun", "SniperRifle"};
    for (size_t i = enemyBotWeapons.size(); i > 1; --i) {
        size_t j = static_cast<size_t>(std::rand()) % i;
        std::swap(enemyBotWeapons[i - 1], enemyBotWeapons[j]);
    }

    // Enemy bots (Team 1)
    int enemyOffsets[] = {-3, 0, 3};
    for (int i = 0; i < 3; ++i)
    {
        Enemy* enemyBot = new Enemy((midX + enemyOffsets[i] + 0.5f) * cellSize, (team1Y + 0.5f) * cellSize, 1);
        enemyBot->SetStage(const_cast<Stage*>(&stage), cellSize);
        enemyBot->SetModelPath("Resource/GreenCharacter/character.mv1");
        std::string wName = (i < static_cast<int>(enemyBotWeapons.size())) ? enemyBotWeapons[i] : "Handgun";
        enemyBot->SetEquippedWeapon(CreateWeaponByName(wName));
        enemiesOut.push_back(enemyBot);
        if (objectManager) objectManager->AddObject(enemyBot);
    }
}


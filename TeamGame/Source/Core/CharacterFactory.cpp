#include "CharacterFactory.h"
#include "../Characters/Player.h"
#include "../Characters/Enemy.h"
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

    return player;
}

Enemy* CharacterFactory::CreateBot(float startX, float startY, int teamId, int characterId, Stage* stage, float cellSize) {
    Enemy* bot = new Enemy(startX, startY, teamId);
    bot->SetStage(stage, cellSize);

    auto availableCharas = PlayerSelectionManager::GetAvailableCharacters();
    for (const auto& chara : availableCharas) {
        if (chara.id == characterId) {
            bot->status.Init(chara.maxHp, chara.moveSpeed, 10);
            break;
        }
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

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(candidateIds.begin(), candidateIds.end(), g);

    if (candidateIds.size() > static_cast<size_t>(count)) {
        candidateIds.resize(count);
    }

    return candidateIds;
}

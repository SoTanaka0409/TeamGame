#pragma once

#include <vector>

class Player;
class Enemy;
class Stage;

/**
 * @brief プレイヤーおよびボット(Enemy)のスポーン生成を専門に行うファクトリークラス
 */
class CharacterFactory {
public:
    /**
     * @brief プレイヤー(人間操作キャラ)を生成して初期化
     */
    static Player* CreatePlayer(float startX, float startY, Stage* stage, float cellSize, int teamId = 0);

    /**
     * @brief キャラクターID(1~5)を指定してAIボット(Enemy)を生成
     */
    static Enemy* CreateBot(float startX, float startY, int teamId, int characterId, Stage* stage, float cellSize);

    /**
     * @brief キャラ番号(1~5)が被らないようにランダムなキャラIDリストを抽出生成
     */
    static std::vector<int> GenerateUniqueCharacterIds(int count, int excludeId = -1);
};

#pragma once

#include <vector>
#include <string>

class Player;
class Enemy;
class Stage;
class ObjectManager;
class Weapon;

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

    /**
     * @brief 武器名からWeaponインスタンスを生成するヘルパー
     */
    static Weapon* CreateWeaponByName(const std::string& name);

    /**
     * @brief 試合開始時のボット（味方ボット・敵ボット）を生成し、チームごとの武器重複防止配分と配置を行う
     */
    static void SpawnMatchBots(const Stage& stage, float cellSize, Player* player, ObjectManager* objectManager, std::vector<Enemy*>& enemiesOut);
};


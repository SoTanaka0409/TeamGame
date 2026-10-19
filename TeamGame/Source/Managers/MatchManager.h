#pragma once

class ObjectManager;
class Stage;

/**
 * @brief 試合結果のステート
 */
enum class MatchResult
{
    IN_PROGRESS,
    TEAM0_WIN,
    TEAM1_WIN
};

/**
 * @brief 試合ルール、キル数、リスポーン処理、勝敗判定を管理するマネージャークラス
 */
class MatchManager
{
private:
    int team0Kills = 0;
    int team1Kills = 0;
    int targetKillCount = 10;
    bool isMatchEnded = false;

    MatchManager() = default;

public:
    static MatchManager& GetInstance()
    {
        static MatchManager instance;
        return instance;
    }

    MatchManager(const MatchManager&) = delete;
    MatchManager& operator=(const MatchManager&) = delete;

    /**
     * @brief 試合状態のリセット
     */
    void Reset();

    /**
     * @brief 死亡キャラクターのリスポーンタイマー進行および復活座標設定
     */
    void UpdateRespawns(ObjectManager* objectManager, const Stage& stage, float cellSize);

    /**
     * @brief 勝敗の判定
     */
    MatchResult CheckMatchResult();

    int GetTeam0Kills() const { return team0Kills; }
    int GetTeam1Kills() const { return team1Kills; }
    int GetTargetKillCount() const { return targetKillCount; }
    bool IsMatchEnded() const { return isMatchEnded; }
};

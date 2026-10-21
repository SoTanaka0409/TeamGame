#pragma once
#include <string>
#include <vector>

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
public:
    struct KillNotification {
        std::string attackerName;
        std::string victimName;
        int attackerTeamId = -1;
        int victimTeamId = -1;
        bool isPlayerKill = false;
        bool isPlayerDeath = false;
        int remainingFrames = 240; // 4秒間表示
    };

private:
    int team0Kills = 0;
    int team1Kills = 0;
    int targetKillCount = 10;
    bool isMatchEnded = false;
    std::vector<KillNotification> killLogs;

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

    /**
     * @brief キルログ通知の追加
     */
    void AddKillLog(const std::string& attacker, int attackerTeam, const std::string& victim, int victimTeam, bool isPlayerKill, bool isPlayerDeath);

    /**
     * @brief キル通知のタイマー更新
     */
    void UpdateKillLogs();

    /**
     * @brief 画面上へのキル通知・デス通知の描画
     */
    void DrawKillNotifications();

    int GetTeam0Kills() const { return team0Kills; }
    int GetTeam1Kills() const { return team1Kills; }
    int GetTargetKillCount() const { return targetKillCount; }
    bool IsMatchEnded() const { return isMatchEnded; }
};

#include "MatchManager.h"
#include "ObjectManager.h"
#include "GameBalanceManager.h"
#include "../Characters/Character.h"
#include "../Characters/Enemy.h"
#include "../Stage/Stage.h"
#include "Vector2.h"
#include <cstdlib>

void MatchManager::Reset()
{
    team0Kills = 0;
    team1Kills = 0;
    targetKillCount = GameBalanceManager::GetInstance().GetInt("TargetKillCount", 10);
    isMatchEnded = false;
}

void MatchManager::UpdateRespawns(ObjectManager* objectManager, const Stage& stage, float cellSize)
{
    if (!objectManager) return;

    for (auto obj : objectManager->GetObjects())
    {
        Character* ch = dynamic_cast<Character*>(obj);
        if (ch && !ch->IsActive())
        {
            if (!ch->isDeadProcessed)
            {
                if (ch->teamId == 0) team1Kills++;
                else if (ch->teamId == 1) team0Kills++;
                
                ch->isDeadProcessed = true;
                ch->respawnTimer = GameBalanceManager::GetInstance().GetInt("RespawnTimeFrames", 180);
            }
            
            if (ch->respawnTimer > 0)
            {
                ch->respawnTimer--;
            }
            
            if (ch->respawnTimer <= 0)
            {
                ch->isDeadProcessed = false;
                
                int midX = stage.GetWidth() / 2;
                int spawnY = (ch->teamId == 0) ? (stage.GetHeight() - 4) : 3;
                int spawnX = midX + (std::rand() % 7 - 3);
                
                while (stage.IsSolidWall(spawnX, spawnY)) {
                    spawnX = midX + (std::rand() % 7 - 3);
                }

                ch->SetPosition(Vector2((spawnX + 0.5f) * cellSize, (spawnY + 0.5f) * cellSize));
                ch->status.Heal(ch->status.GetMaxHp());
                ch->SetActive(true);
                ch->invincibleTimer = 180;
                Enemy* e = dynamic_cast<Enemy*>(ch);
                if (e) { e->ResetMoveToCenter(); }
            }
        }
    }
}

MatchResult MatchManager::CheckMatchResult()
{
    if (isMatchEnded) return MatchResult::IN_PROGRESS;

    if (team0Kills >= targetKillCount)
    {
        isMatchEnded = true;
        return MatchResult::TEAM0_WIN;
    }
    if (team1Kills >= targetKillCount)
    {
        isMatchEnded = true;
        return MatchResult::TEAM1_WIN;
    }

    return MatchResult::IN_PROGRESS;
}

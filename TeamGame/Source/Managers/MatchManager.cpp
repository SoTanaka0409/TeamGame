#include "MatchManager.h"
#include "ObjectManager.h"
#include "GameBalanceManager.h"
#include "../Characters/Character.h"
#include "../Characters/Enemy.h"
#include "../Characters/Player.h"
#include "../Stage/Stage.h"
#include "Vector2.h"
#include <cstdlib>

#include "FontManager.h"
#include "../UI/HudStyle.h"
#include <DxLib.h>

void MatchManager::Reset()
{
    team0Kills = 0;
    team1Kills = 0;
    targetKillCount = GameBalanceManager::GetInstance().GetInt("TargetKillCount", 10);
    isMatchEnded = false;
    killLogs.clear();
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
                if (ch->isSuicideDeath)
                {
                    // 指揮者の自爆特攻スキルによる死亡: 敵チームのキルスコアには加算しない
                    std::string victimName = ch->GetCharacterName();
                    AddKillLog("自爆 (指揮者スキル)", ch->teamId, victimName, ch->teamId, false, false);
                }
                else
                {
                    if (ch->teamId == 0) team1Kills++;
                    else if (ch->teamId == 1) team0Kills++;

                    // キルログの生成
                    std::string victimName = ch->GetCharacterName();
                    std::string killerName = ch->GetLastAttackerName();
                    int killerTeam = ch->GetLastAttackerTeamId();
                    if (killerName.empty()) {
                        killerName = (ch->teamId == 0) ? "敵ボット" : "プレイヤー";
                        killerTeam = (ch->teamId == 0) ? 1 : 0;
                    }

                    bool isPlayerKill = (killerName == "プレイヤー" || (killerTeam == 0 && killerName != "味方ボット"));
                    bool isPlayerDeath = (victimName == "プレイヤー");

                    AddKillLog(killerName, killerTeam, victimName, ch->teamId, isPlayerKill, isPlayerDeath);
                }
                
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
                ch->isSuicideDeath = false;
                
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
                Player* p = dynamic_cast<Player*>(ch);
                if (p) {
                    p->ResetDying();
                    if (p->GetModel()) p->GetModel()->PlayIdle();
                }
                Enemy* e = dynamic_cast<Enemy*>(ch);
                if (e) {
                    e->StopHomingSuicide();
                    e->ResetMoveToCenter();
                    e->ResetDying();
                    if (e->GetModel()) e->GetModel()->PlayIdle();
                }
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

void MatchManager::AddKillLog(const std::string& attacker, int attackerTeam, const std::string& victim, int victimTeam, bool isPlayerKill, bool isPlayerDeath)
{
    KillNotification notif;
    notif.attackerName = attacker;
    notif.attackerTeamId = attackerTeam;
    notif.victimName = victim;
    notif.victimTeamId = victimTeam;
    notif.isPlayerKill = isPlayerKill;
    notif.isPlayerDeath = isPlayerDeath;
    notif.remainingFrames = 240; // 4秒間

    killLogs.push_back(notif);
    // 最大表示数は5件に制限
    if (killLogs.size() > 5) {
        killLogs.erase(killLogs.begin());
    }
}

void MatchManager::UpdateKillLogs()
{
    for (auto it = killLogs.begin(); it != killLogs.end(); )
    {
        it->remainingFrames--;
        if (it->remainingFrames <= 0) {
            it = killLogs.erase(it);
        } else {
            ++it;
        }
    }
}

void MatchManager::DrawKillNotifications()
{
    if (killLogs.empty()) return;

    int rightMargin = 24;
    int baseTopY = 18;
    int logW = 340;
    int logH = 30;
    int spacing = 6;

    for (size_t i = 0; i < killLogs.size(); ++i)
    {
        const auto& log = killLogs[i];
        int x = 1920 - rightMargin - logW;
        int y = baseTopY + static_cast<int>(i) * (logH + spacing);

        // フェードアウト処理 (残り40フレームで透明化)
        int alpha = HudStyle::DefaultBgAlpha;
        if (log.remainingFrames < 40) {
            alpha = static_cast<int>(HudStyle::DefaultBgAlpha * (log.remainingFrames / 40.0f));
        }

        // 1. パネル背景と枠線（自分のキルは琥珀、自分のデスは赤）
        if (log.isPlayerKill) {
            HudStyle::DrawPanel(x, y, logW, logH, true, HudStyle::ColAmber(), alpha);
        } else if (log.isPlayerDeath) {
            HudStyle::DrawPanel(x, y, logW, logH, true, HudStyle::ColEnemy(), alpha);
        } else {
            HudStyle::DrawPanel(x, y, logW, logH, false, 0, alpha);
        }

        // 2. テキスト描画 (Killer > Victim)
        int textY = y + 7;

        std::string killerText = log.isPlayerKill ? "YOU" : log.attackerName;
        unsigned int killerCol = HudStyle::ColEnemy();
        if (log.isPlayerKill) {
            killerCol = HudStyle::ColAmber();
        } else if (log.attackerTeamId == 0) {
            killerCol = HudStyle::ColAlly();
        }

        std::string victimText = log.isPlayerDeath ? "YOU" : log.victimName;
        unsigned int victimCol = HudStyle::ColEnemy();
        if (log.isPlayerDeath) {
            victimCol = HudStyle::ColEnemy();
        } else if (log.victimTeamId == 0) {
            victimCol = HudStyle::ColAlly();
        }

        // Killer
        FontManager::GetInstance().DrawString(x + 14, textY, killerText.c_str(), killerCol, 14);
        int killerW = FontManager::GetInstance().GetStringWidth(killerText.c_str(), 14);

        // Separator (>)
        FontManager::GetInstance().DrawString(x + 14 + killerW + 6, textY, ">", HudStyle::ColSubText(), 14);
        int sepW = FontManager::GetInstance().GetStringWidth(">", 14);

        // Victim
        FontManager::GetInstance().DrawString(x + 14 + killerW + 6 + sepW + 6, textY, victimText.c_str(), victimCol, 14);

        // 右端バッジ (自分のキル: KILL / 自分のデス: DEAD)
        if (log.isPlayerKill) {
            FontManager::GetInstance().DrawStringRight(x + logW - 14, textY, "KILL", HudStyle::ColAmber(), 13);
        } else if (log.isPlayerDeath) {
            FontManager::GetInstance().DrawStringRight(x + logW - 14, textY, "DEAD", HudStyle::ColEnemy(), 13);
        }
    }
}

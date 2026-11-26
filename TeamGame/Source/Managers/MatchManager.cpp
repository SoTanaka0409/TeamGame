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
#include "../Scenes/Scene.h"
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
                // キルスコア加算（倒されたチームの相手側のキル数が増加：指揮者スキル自爆含む）
                if (ch->teamId == 0) team1Kills++;
                else if (ch->teamId == 1) team0Kills++;

                std::string victimName = ch->GetCharacterName();

                if (ch->isSuicideDeath)
                {
                    // 指揮者の自爆特攻スキルによる死亡: 相手チームのキルとして記録
                    int enemyTeamId = (ch->teamId == 0) ? 1 : 0;
                    std::string killerTitle = (ch->teamId == 0) ? "指揮者スキル (敵得点)" : "指揮者スキル (味方得点)";
                    AddKillLog(killerTitle, enemyTeamId, victimName, ch->teamId, (enemyTeamId == 0), false);
                }
                else
                {
                    // 通常のキルログ生成
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
    int logW = 410;
    int logH = 38;
    int spacing = 6;

    static int s_killStripIdx = -2;
    static int s_killSelIdx = -2;
    if (s_killStripIdx == -2) {
        s_killStripIdx = Scene::Load2DImage("Images/UI/PreGame/panel_strip.png");
        s_killSelIdx   = Scene::Load2DImage("Images/UI/PreGame/btn_selected.png");
    }

    for (size_t i = 0; i < killLogs.size(); ++i)
    {
        const auto& log = killLogs[i];
        int x = 1920 - rightMargin - logW;
        int y = baseTopY + static_cast<int>(i) * (logH + spacing);

        // フェードアウト処理 (残り40フレームで透明化)
        int alpha = 225;
        if (log.remainingFrames < 40) {
            alpha = static_cast<int>(225 * (log.remainingFrames / 40.0f));
        }

        // 1. 手描き質感パネル背景
        if (log.isPlayerKill || log.isPlayerDeath) {
            HudStyle::DrawTexturedPanel(x, y, logW, logH, Scene::Get2DImage(s_killSelIdx), HudStyle::ColBg(),
                                        log.isPlayerKill ? HudStyle::ColAmber() : HudStyle::ColEnemy(), alpha);
        } else {
            HudStyle::DrawTexturedPanel(x, y, logW, logH, Scene::Get2DImage(s_killStripIdx), HudStyle::ColBg(), HudStyle::ColBorderSoft(), alpha);
        }

        // 2. テキスト描画 (日本語表記で統一)
        int fontSz = 16;

        std::string killerText = log.isPlayerKill ? "自分" : log.attackerName;
        unsigned int killerCol = HudStyle::ColEnemy();
        if (log.isPlayerKill) {
            killerCol = HudStyle::ColAmber();
        } else if (log.attackerTeamId == 0) {
            killerCol = HudStyle::ColAlly();
        }

        std::string victimText = log.isPlayerDeath ? "自分" : log.victimName;
        unsigned int victimCol = HudStyle::ColEnemy();
        if (log.isPlayerDeath) {
            victimCol = HudStyle::ColEnemy();
        } else if (log.victimTeamId == 0) {
            victimCol = HudStyle::ColAlly();
        }

        int badgeAreaW = (log.isPlayerKill || log.isPlayerDeath) ? 68 : 16;
        int maxTextW = logW - 20 - badgeAreaW;
        std::string fullLine = killerText + " ▶ " + victimText;
        int fitSz = FontManager::GetInstance().GetFittingFontSize(fullLine.c_str(), maxTextW, fontSz, 13);
        int drawY = y + (logH - fitSz) / 2;

        // Killer
        FontManager::GetInstance().DrawString(x + 14, drawY, killerText.c_str(), killerCol, fitSz, 6);
        int killerW = FontManager::GetInstance().GetStringWidth(killerText.c_str(), fitSz, 6);

        // Separator
        FontManager::GetInstance().DrawString(x + 14 + killerW + 6, drawY, "▶", HudStyle::ColSubText(), fitSz);
        int sepW = FontManager::GetInstance().GetStringWidth("▶", fitSz);

        // Victim
        FontManager::GetInstance().DrawString(x + 14 + killerW + 6 + sepW + 6, drawY, victimText.c_str(), victimCol, fitSz);

        // 右端バッジ (自分のキル: 撃破 / 自分のデス: ダウン)
        if (log.isPlayerKill) {
            FontManager::GetInstance().DrawStringRight(x + logW - 14, y + (logH - 15) / 2, "撃破", HudStyle::ColAmber(), 15, 7);
        } else if (log.isPlayerDeath) {
            FontManager::GetInstance().DrawStringRight(x + logW - 14, y + (logH - 15) / 2, "ダウン", HudStyle::ColEnemy(), 15, 7);
        }
    }
}

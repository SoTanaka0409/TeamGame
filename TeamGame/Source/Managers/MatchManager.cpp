#include "MatchManager.h"
#include "ObjectManager.h"
#include "GameBalanceManager.h"
#include "../Characters/Character.h"
#include "../Characters/Enemy.h"
#include "../Stage/Stage.h"
#include "Vector2.h"
#include <cstdlib>

#include "FontManager.h"
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
                Enemy* e = dynamic_cast<Enemy*>(ch);
                if (e) {
                    e->StopHomingSuicide();
                    e->ResetMoveToCenter();
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

    int rightMargin = 30;
    int baseTopY = 90;
    int bannerH = 46;
    int spacing = 8;

    for (size_t i = 0; i < killLogs.size(); ++i)
    {
        const auto& log = killLogs[i];
        int y = baseTopY + static_cast<int>(i) * (bannerH + spacing);

        // フェードアウト処理 (残り40フレームで透明化)
        int alpha = 230;
        if (log.remainingFrames < 40) {
            alpha = static_cast<int>(230.0f * (log.remainingFrames / 40.0f));
        }

        // バナー幅の計算
        std::string text = log.attackerName + "  >>  " + log.victimName;
        int textW = FontManager::GetInstance().GetStringWidth(text.c_str(), 20, 6);
        int bannerW = textW + 70;
        if (bannerW < 280) bannerW = 280;

        int x = 1920 - rightMargin - bannerW;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

        // バナー背景（漆黒ベース）
        DrawBox(x, y, x + bannerW, y + bannerH, GetColor(15, 15, 20), TRUE);

        // バナーの左アクセントカラーバー＆縁取り
        unsigned int accentCol = GetColor(180, 180, 190);
        if (log.isPlayerKill) {
            // プレイヤーのキル: 鮮烈なゴールド / シアン
            accentCol = GetColor(255, 215, 0);
            DrawBox(x, y, x + 8, y + bannerH, accentCol, TRUE);
            DrawBox(x, y, x + bannerW, y + bannerH, accentCol, FALSE);
        } else if (log.isPlayerDeath) {
            // プレイヤーのデス: 警戒レッド
            accentCol = GetColor(255, 40, 50);
            DrawBox(x, y, x + 8, y + bannerH, accentCol, TRUE);
            DrawBox(x, y, x + bannerW, y + bannerH, accentCol, FALSE);
        } else {
            // 通常のキル（味方キルまたは敵キル）
            accentCol = (log.attackerTeamId == 0) ? GetColor(60, 160, 255) : GetColor(255, 80, 80);
            DrawBox(x, y, x + 6, y + bannerH, accentCol, TRUE);
            DrawBox(x, y, x + bannerW, y + bannerH, GetColor(50, 50, 60), FALSE);
        }

        // テキスト描画 (Killer: チームカラー / Victim: チームカラー)
        unsigned int killerCol = (log.attackerTeamId == 0) ? GetColor(100, 200, 255) : GetColor(255, 110, 110);
        if (log.isPlayerKill) killerCol = GetColor(255, 235, 60);

        unsigned int victimCol = (log.victimTeamId == 0) ? GetColor(120, 190, 255) : GetColor(255, 120, 120);
        if (log.isPlayerDeath) victimCol = GetColor(255, 70, 70);

        int textStartX = x + 18;
        int textY = y + 12;

        FontManager::GetInstance().DrawString(textStartX, textY, log.attackerName.c_str(), killerCol, 20, 6);
        int killerW = FontManager::GetInstance().GetStringWidth(log.attackerName.c_str(), 20, 6);

        // アイコン (クロスボーン/矢印)
        FontManager::GetInstance().DrawString(textStartX + killerW + 8, textY, ">>", GetColor(255, 255, 255), 20, 8);
        int arrowW = FontManager::GetInstance().GetStringWidth(">>", 20, 8);

        FontManager::GetInstance().DrawString(textStartX + killerW + 8 + arrowW + 8, textY, log.victimName.c_str(), victimCol, 20, 6);

        // プレイヤーのキルまたはデスの場合はバッジアイコンを右端に表示
        if (log.isPlayerKill) {
            FontManager::GetInstance().DrawString(x + bannerW - 55, textY + 2, "KILL", GetColor(255, 230, 0), 16, 9);
        } else if (log.isPlayerDeath) {
            FontManager::GetInstance().DrawString(x + bannerW - 55, textY + 2, "DEAD", GetColor(255, 60, 60), 16, 9);
        }

        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}

#define NOMINMAX
#include "PlayerHudRenderer.h"
#include "HudStyle.h"
#include "../Characters/Player.h"
#include "../Characters/Enemy.h"
#include "../Core/Camera.h"
#include "../Managers/FontManager.h"
#include "../Managers/GameBalanceManager.h"
#include "../Managers/SceneManager.h"
#include "../Scenes/Scene.h"
#include "../Skills/Skill.h"
#include "../Skills/SkillData.h"
#include "../Weapons/Weapon.h"
#include <DxLib.h>
#include <cmath>
#include <cstdio>
#include <string>

PlayerHudRenderer::~PlayerHudRenderer()
{
    if (m_hudPlateHandle != -1)
    {
        DeleteGraph(m_hudPlateHandle);
        m_hudPlateHandle = -1;
    }
}

void PlayerHudRenderer::EnsureHudPlateLoaded(int characterId)
{
    if (m_hudPlateLoadAttempted && m_loadedCharacterId == characterId)
    {
        return;
    }

    if (m_hudPlateHandle != -1)
    {
        DeleteGraph(m_hudPlateHandle);
        m_hudPlateHandle = -1;
    }

    m_hudPlateLoadAttempted = true;
    m_loadedCharacterId = characterId;

    static const std::string hudFiles[5] = {
        "Resource/Images/UI/HUD/hud_conductor.png",
        "Resource/Images/UI/HUD/hud_rebel.png",
        "Resource/Images/UI/HUD/hud_baka.png",
        "Resource/Images/UI/HUD/hud_housewife.png",
        "Resource/Images/UI/HUD/hud_devil.png"
    };
    const int idx = characterId - 1;
    if (idx >= 0 && idx < 5)
    {
        m_hudPlateHandle = Scene::LoadGraphWithSearch(hudFiles[idx]);
    }
}

void PlayerHudRenderer::DrawStatusHud(const Player& player, int screenX, int screenY)
{
    Weapon* currentWeapon = player.GetCurrentWeapon();
    if (!currentWeapon) return;

    const int characterId = player.GetCharacterId();
    EnsureHudPlateLoaded(characterId);

    // HUDプレートの配置・スケール設定 (1024x480 -> スケール0.42: 430x201)
    const float scale = 0.42f;
    const int plateW = static_cast<int>(1024 * scale); // 430
    const int plateH = static_cast<int>(480 * scale);  // 201

    int plateX = screenX;
    int plateY = screenY;
    if (plateY + plateH > 1060)
    {
        plateY = 1060 - plateH;
    }
    if (plateX < 20) plateX = 20;

    // 上部: 手描き質感の武器・弾薬バー（選択画面と統一デザイン）
    static int s_weaponStripIdx = -2;
    if (s_weaponStripIdx == -2)
    {
        s_weaponStripIdx = Scene::Load2DImage("Images/UI/PreGame/panel_strip.png");
    }

    const int bannerH = 30;
    const int bannerY = plateY - bannerH - 8;
    HudStyle::DrawTexturedPanel(plateX, bannerY, plateW, bannerH, Scene::Get2DImage(s_weaponStripIdx), HudStyle::ColBg(), HudStyle::ColBorder(), 235);

    // 武器名（日本語表記に統一）
    const std::string rawName = currentWeapon->GetName();
    std::string wName = rawName;
    if (rawName.find("Handgun") != std::string::npos) wName = "ハンドガン";
    else if (rawName.find("Shotgun") != std::string::npos) wName = "ショットガン";
    else if (rawName.find("Sniper") != std::string::npos) wName = "スナイパー";
    else if (rawName.find("SMG") != std::string::npos || rawName.find("Submachine") != std::string::npos) wName = "サブマシンガン";

    const bool aimLocked = player.IsAimLocked();
    if (aimLocked) wName += "（固定）";

    char weaponTitle[96];
    snprintf(weaponTitle, sizeof(weaponTitle), "%d. %s", player.GetCurrentWeaponIndex() + 1, wName.c_str());
    FontManager::GetInstance().DrawString(plateX + 14, bannerY + 7, weaponTitle, aimLocked ? HudStyle::ColAmber() : HudStyle::ColText(), 15, 6);

    // 弾薬数
    const int currentAmmo = currentWeapon->GetCurrentAmmo();
    const int maxAmmo = currentWeapon->GetMaxAmmo();
    if (currentWeapon->IsReloading())
    {
        FontManager::GetInstance().DrawStringRight(plateX + plateW - 14, bannerY + 7, "リロード中...", HudStyle::ColAmber(), 15, 6);
    }
    else
    {
        char ammoText[32];
        snprintf(ammoText, sizeof(ammoText), "弾薬 %d / %d", currentAmmo, maxAmmo);
        const unsigned int ammoColor = HudStyle::GetAmmoColor(currentAmmo, maxAmmo);
        FontManager::GetInstance().DrawStringRight(plateX + plateW - 14, bannerY + 7, ammoText, ammoColor, 15, 6);
    }

    // キャラクター別HUDプレート画像の描画
    if (m_hudPlateHandle != -1)
    {
        DrawExtendGraph(plateX, plateY, plateX + plateW, plateY + plateH, m_hudPlateHandle, TRUE);
    }
    else
    {
        HudStyle::DrawPanel(plateX, plateY, plateW, plateH, true);
    }

    // 各キャラクター固有のバー座標（1024x480画像内の矩形座標）
    struct BarRect
    {
        int x0, y0, x1, y1;
    };
    static const BarRect mpSlots[5] = {
        { 200, 102, 880, 155 }, // 1: 指揮者（ト音記号の右から終止線まで）
        { 136, 104, 990, 168 }, // 2: 反逆者（有刺鉄線フレーム内）
        { 140, 105, 880, 165 }, // 3: バカ（黄色キャンバス）
        { 140, 105, 880, 165 }, // 4: 主婦（桜吹雪の軌道上）
        { 140, 105, 750, 165 }  // 5: 悪魔（月手前の境界まで）
    };
    static const BarRect hpSlots[5] = {
        { 200, 324, 880, 377 }, // 1: 指揮者（ヘ音記号の右から終止線まで）
        { 136, 326, 990, 390 }, // 2: 反逆者（有刺鉄線フレーム内）
        { 140, 320, 880, 380 }, // 3: バカ（黄色キャンバス）
        { 140, 320, 880, 380 }, // 4: 主婦（桜吹雪の軌道上）
        { 140, 320, 750, 380 }  // 5: 悪魔
    };

    int charIdx = characterId - 1;
    if (charIdx < 0 || charIdx >= 5) charIdx = 0;

    const int mpX = plateX + static_cast<int>(mpSlots[charIdx].x0 * scale);
    const int mpY = plateY + static_cast<int>(mpSlots[charIdx].y0 * scale);
    const int mpW = static_cast<int>((mpSlots[charIdx].x1 - mpSlots[charIdx].x0) * scale);
    const int mpH = static_cast<int>((mpSlots[charIdx].y1 - mpSlots[charIdx].y0) * scale);

    const int hpX = plateX + static_cast<int>(hpSlots[charIdx].x0 * scale);
    const int hpY = plateY + static_cast<int>(hpSlots[charIdx].y0 * scale);
    const int hpW = static_cast<int>((hpSlots[charIdx].x1 - hpSlots[charIdx].x0) * scale);
    const int hpH = static_cast<int>((hpSlots[charIdx].y1 - hpSlots[charIdx].y0) * scale);

    // MPバー（スキルクールタイム：MAXで使用可能）
    Skill* skill = player.GetEquippedSkill();
    if (skill)
    {
        const int ct = skill->GetCoolTimeTimer();
        const int maxCt = (skill->GetData() && skill->GetData()->coolTime > 0) ? skill->GetData()->coolTime : 300;
        int activeFrames = 0;
        const int maxDuration = (skill->GetData() && skill->GetData()->duration > 0) ? skill->GetData()->duration : 300;

        if (player.IsTransparent())
        {
            activeFrames = player.GetEffectRemainingFrames(SkillMinorTag::Transparency);
        }
        else if (player.IsHighRisk())
        {
            activeFrames = player.GetEffectRemainingFrames(SkillMinorTag::HighRisk);
        }

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
        HudStyle::DrawRoundBox(mpX, mpY, mpX + mpW, mpY + mpH, HudStyle::RadiusGauge, GetColor(14, 15, 12), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        if (activeFrames > 0)
        {
            float actRatio = static_cast<float>(activeFrames) / static_cast<float>(maxDuration);
            if (actRatio > 1.0f) actRatio = 1.0f; else if (actRatio < 0.0f) actRatio = 0.0f;
            const int fillW = static_cast<int>(mpW * actRatio);

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 210);
            HudStyle::DrawRoundBox(mpX, mpY, mpX + fillW, mpY + mpH, HudStyle::RadiusGauge, HudStyle::ColAmber(), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            HudStyle::DrawRoundBox(mpX, mpY, mpX + mpW, mpY + mpH, HudStyle::RadiusGauge, GetColor(245, 220, 140), FALSE);

            char actText[32];
            snprintf(actText, sizeof(actText), "発動中 %.1f秒", activeFrames / 60.0f);
            FontManager::GetInstance().DrawStringCenter(mpX + mpW / 2, mpY + (mpH - 14) / 2, actText, GetColor(255, 255, 255), 13, 6);
        }
        else if (ct > 0)
        {
            float mpRatio = 1.0f - static_cast<float>(ct) / static_cast<float>(maxCt);
            if (mpRatio < 0.0f) mpRatio = 0.0f; else if (mpRatio > 1.0f) mpRatio = 1.0f;
            const int fillW = static_cast<int>(mpW * mpRatio);

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 185);
            HudStyle::DrawRoundBox(mpX, mpY, mpX + fillW, mpY + mpH, HudStyle::RadiusGauge, GetColor(62, 138, 132), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            HudStyle::DrawRoundBox(mpX, mpY, mpX + mpW, mpY + mpH, HudStyle::RadiusGauge, GetColor(46, 78, 74), FALSE);

            char ctText[32];
            snprintf(ctText, sizeof(ctText), "チャージ中 %.1f秒", ct / 60.0f);
            FontManager::GetInstance().DrawStringCenter(mpX + mpW / 2, mpY + (mpH - 14) / 2, ctText, HudStyle::ColText(), 13);
        }
        else
        {
            static int readyTimer = 0;
            readyTimer++;
            const int pulseAlpha = 185 + static_cast<int>(50.0f * std::sin(readyTimer * 0.08f));

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, pulseAlpha);
            HudStyle::DrawRoundBox(mpX, mpY, mpX + mpW, mpY + mpH, HudStyle::RadiusGauge, GetColor(68, 174, 164), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            HudStyle::DrawRoundBox(mpX - 1, mpY - 1, mpX + mpW + 1, mpY + mpH + 1, HudStyle::RadiusGauge + 1, HudStyle::ColAmber(), FALSE);

            const char* readyKey = (player.GetInputType() == PlayerInputType::KEYBOARD_MOUSE) ? "スキル発動 [F]" : "スキル発動 [RT]";
            FontManager::GetInstance().DrawStringCenter(mpX + mpW / 2, mpY + (mpH - 14) / 2, readyKey, GetColor(255, 255, 255), 14, 7);
        }
    }

    // HPバー（プレイヤー体力）
    const int curHp = player.status.GetCurrentHp();
    const int maxHp = player.status.GetMaxHp();
    float hpRatio = (maxHp > 0) ? static_cast<float>(curHp) / static_cast<float>(maxHp) : 0.0f;
    if (hpRatio < 0.0f) hpRatio = 0.0f; else if (hpRatio > 1.0f) hpRatio = 1.0f;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
    HudStyle::DrawRoundBox(hpX, hpY, hpX + hpW, hpY + hpH, HudStyle::RadiusGauge, GetColor(14, 15, 12), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    const int hpFillW = static_cast<int>(hpW * hpRatio);
    unsigned int hpCol = GetColor(112, 174, 82);
    if (hpRatio <= 0.25f) hpCol = HudStyle::ColEnemy();
    else if (hpRatio <= 0.50f) hpCol = HudStyle::ColAmber();

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 205);
    HudStyle::DrawRoundBox(hpX, hpY, hpX + hpFillW, hpY + hpH, HudStyle::RadiusGauge, hpCol, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    HudStyle::DrawRoundBox(hpX, hpY, hpX + hpW, hpY + hpH, HudStyle::RadiusGauge, HudStyle::ColBorderSoft(), FALSE);

    char hpStr[32];
    snprintf(hpStr, sizeof(hpStr), "%d / %d", curHp, maxHp);
    FontManager::GetInstance().DrawStringCenter(hpX + hpW / 2, hpY + (hpH - 14) / 2, hpStr, GetColor(255, 255, 255), 14, 6);

    // ゲームパッド等のオートエイム状態インジケーター表示
    if (player.GetInputType() != PlayerInputType::KEYBOARD_MOUSE)
    {
        if (player.GetAutoAimTarget())
        {
            FontManager::GetInstance().DrawString(plateX + 10, bannerY - 20, "オートエイム中", HudStyle::ColEnemy(), 13);
        }
    }
}

void PlayerHudRenderer::DrawBloodSplatterOverlay(const Player& player, int screenWidth, int screenHeight) const
{
    if (!player.IsActive()) return;
    const int lowHpThreshold = GameBalanceManager::GetInstance().GetInt("LowHpThreshold", 3);
    if (player.status.GetCurrentHp() > lowHpThreshold) return;

    static float pulseTimer = 0.0f;
    pulseTimer += 0.08f;
    const float pulse = (std::sin(pulseTimer) + 1.0f) * 0.5f;

    const int hpGap = 4 - player.status.GetCurrentHp();
    const int baseAlpha = 70 + hpGap * 35;
    int currentAlpha = static_cast<int>(baseAlpha + pulse * 60);
    if (currentAlpha > 240) currentAlpha = 240;

    const int borderSteps = 6;
    for (int i = 0; i < borderSteps; ++i)
    {
        const int bandThickness = (borderSteps - i) * 10;
        const int stepAlpha = static_cast<int>(currentAlpha * (0.3f + 0.7f * static_cast<float>(borderSteps - i) / borderSteps));

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, stepAlpha);
        DrawBox(0, 0, screenWidth, bandThickness, GetColor(180, 0, 0), TRUE);
        DrawBox(0, screenHeight - bandThickness, screenWidth, screenHeight, GetColor(180, 0, 0), TRUE);
        DrawBox(0, 0, bandThickness, screenHeight, GetColor(180, 0, 0), TRUE);
        DrawBox(screenWidth - bandThickness, 0, screenWidth, screenHeight, GetColor(180, 0, 0), TRUE);
    }

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(currentAlpha * 0.95f));
    DrawBox(0, 0, screenWidth, screenHeight, GetColor(255, 30, 30), FALSE);
    DrawBox(3, 3, screenWidth - 3, screenHeight - 3, GetColor(220, 10, 10), FALSE);

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(160 + pulse * 85));
    DrawString(screenWidth / 2 - 80, 30, "!! LOW HP DANGER !!", GetColor(255, 60, 60));
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void PlayerHudRenderer::DrawHighRiskEnemyIndicators(const Player& player) const
{
    if (!player.IsActive() || !player.IsHighRisk()) return;

    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (!scene || !scene->GetObjectManager()) return;

    const Vector2 playerPos = player.GetPosition();

    for (auto obj : scene->GetObjectManager()->GetObjects())
    {
        Enemy* enemy = dynamic_cast<Enemy*>(obj);
        if (!enemy || !enemy->IsActive() || enemy->teamId == player.teamId) continue;

        const Vector2 ePos = enemy->GetPosition();
        const float eScreenX = Camera::WorldToScreenX(ePos.x);
        const float eScreenY = Camera::WorldToScreenY(ePos.y);

        const float dx = ePos.x - playerPos.x;
        const float dy = ePos.y - playerPos.y;
        const float distPx = std::sqrt(dx * dx + dy * dy);
        const float distMeter = distPx / 40.0f;

        float arrowScreenX = eScreenX;
        float arrowScreenY = eScreenY;
        bool isOffScreen = false;

        const float margin = 80.0f;
        if (arrowScreenX < margin || arrowScreenX > 1920.0f - margin ||
            arrowScreenY < margin || arrowScreenY > 1080.0f - margin)
        {
            isOffScreen = true;
            const float dirX = dx / (distPx > 0.001f ? distPx : 1.0f);
            const float dirY = dy / (distPx > 0.001f ? distPx : 1.0f);
            arrowScreenX = 1920.0f / 2.0f + dirX * 420.0f;
            arrowScreenY = 1080.0f / 2.0f + dirY * 420.0f;
        }

        const float angleToEnemy = std::atan2(dy, dx);
        const float tipX = arrowScreenX;
        const float tipY = isOffScreen ? arrowScreenY : (eScreenY - 35.0f);

        const float arrowLen = 16.0f;
        const float p1X = tipX + std::cos(angleToEnemy) * arrowLen;
        const float p1Y = tipY + std::sin(angleToEnemy) * arrowLen;
        const float p2X = tipX + std::cos(angleToEnemy + 2.4f) * (arrowLen * 0.8f);
        const float p2Y = tipY + std::sin(angleToEnemy + 2.4f) * (arrowLen * 0.8f);
        const float p3X = tipX + std::cos(angleToEnemy - 2.4f) * (arrowLen * 0.8f);
        const float p3Y = tipY + std::sin(angleToEnemy - 2.4f) * (arrowLen * 0.8f);

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
        DrawTriangle(static_cast<int>(p1X), static_cast<int>(p1Y),
                     static_cast<int>(p2X), static_cast<int>(p2Y),
                     static_cast<int>(p3X), static_cast<int>(p3Y),
                     GetColor(0, 0, 0), FALSE);
        DrawTriangle(static_cast<int>(p1X), static_cast<int>(p1Y),
                     static_cast<int>(p2X), static_cast<int>(p2Y),
                     static_cast<int>(p3X), static_cast<int>(p3Y),
                     GetColor(255, 40, 40), TRUE);

        char distText[32];
        snprintf(distText, sizeof(distText), "あと %.1fm", distMeter);
        const int textW = 86;
        const int textH = 18;
        const int textX = static_cast<int>(tipX) - textW / 2;
        const int textY = static_cast<int>(tipY) - 26;

        HudStyle::DrawRoundPanel(textX - 4, textY - 2, textW + 4, textH + 2, HudStyle::RadiusBadge, GetColor(0, 0, 0), GetColor(255, 220, 0), 200);
        DrawString(textX, textY, distText, GetColor(255, 230, 40));

        if (!isOffScreen)
        {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
            DrawCircle(static_cast<int>(eScreenX), static_cast<int>(eScreenY), 26, GetColor(255, 30, 30), FALSE);
            DrawCircle(static_cast<int>(eScreenX), static_cast<int>(eScreenY), 24, GetColor(255, 210, 0), FALSE);
            DrawCircle(static_cast<int>(eScreenX), static_cast<int>(eScreenY), 22, GetColor(255, 50, 50), FALSE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
    }
}

void PlayerHudRenderer::DrawAutoAimReticle(const Player& player) const
{
    Character* target = player.GetAutoAimTarget();
    if (!target || !target->IsActive() || target->status.IsDead()) return;

    const Vector2 targetPos = target->GetPosition();
    const float tScreenX = Camera::WorldToScreenX(targetPos.x);
    const float tScreenY = Camera::WorldToScreenY(targetPos.y);

    if (tScreenX < -100.0f || tScreenX > 2020.0f || tScreenY < -100.0f || tScreenY > 1180.0f)
    {
        return;
    }

    const float targetR = target->GetRadius() * Camera::ZoomScale + 12.0f;
    const float timeSec = GetNowCount() / 1000.0f;
    const float pulse = std::sin(timeSec * 8.0f) * 3.0f;
    const float drawR = targetR + pulse;

    const unsigned int lockColor = GetColor(255, 60, 60);
    DrawCircle(static_cast<int>(tScreenX), static_cast<int>(tScreenY), static_cast<int>(drawR), lockColor, FALSE);

    const int len = 10;
    const int offset = static_cast<int>(drawR);
    DrawLine(static_cast<int>(tScreenX - offset), static_cast<int>(tScreenY - offset), static_cast<int>(tScreenX - offset + len), static_cast<int>(tScreenY - offset), lockColor, 2);
    DrawLine(static_cast<int>(tScreenX - offset), static_cast<int>(tScreenY - offset), static_cast<int>(tScreenX - offset), static_cast<int>(tScreenY - offset + len), lockColor, 2);
    DrawLine(static_cast<int>(tScreenX + offset), static_cast<int>(tScreenY - offset), static_cast<int>(tScreenX + offset - len), static_cast<int>(tScreenY - offset), lockColor, 2);
    DrawLine(static_cast<int>(tScreenX + offset), static_cast<int>(tScreenY - offset), static_cast<int>(tScreenX + offset), static_cast<int>(tScreenY - offset + len), lockColor, 2);
    DrawLine(static_cast<int>(tScreenX - offset), static_cast<int>(tScreenY + offset), static_cast<int>(tScreenX - offset + len), static_cast<int>(tScreenY + offset), lockColor, 2);
    DrawLine(static_cast<int>(tScreenX - offset), static_cast<int>(tScreenY + offset), static_cast<int>(tScreenX - offset), static_cast<int>(tScreenY + offset - len), lockColor, 2);
    DrawLine(static_cast<int>(tScreenX + offset), static_cast<int>(tScreenY + offset), static_cast<int>(tScreenX + offset - len), static_cast<int>(tScreenY + offset), lockColor, 2);
    DrawLine(static_cast<int>(tScreenX + offset), static_cast<int>(tScreenY + offset), static_cast<int>(tScreenX + offset), static_cast<int>(tScreenY + offset - len), lockColor, 2);

    FontManager::GetInstance().DrawString(static_cast<int>(tScreenX - 32), static_cast<int>(tScreenY - offset - 18), "LOCK ON", lockColor, 16);
}

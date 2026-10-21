#include "Camera.h"
#include "EffectManager.h"
#include "DxLib.h"
#include "Scene.h"
#include "SceneManager.h"
#include <cmath>
#include <cstdlib>

void BloodParticle::Update()
{
    Effect::Update();
    x += vx;
    y += vy;
    vx *= 0.90f;
    vy *= 0.90f;
}

void BloodParticle::Draw()
{
    float screenX = Camera::WorldToScreenX(x);
    float screenY = Camera::WorldToScreenY(y);

    if (screenX < -50.0f || screenX > 1970.0f || screenY < -50.0f || screenY > 1130.0f)
        return;

    int alpha = static_cast<int>(255.0f * (static_cast<float>(lifeTimer) / maxLife));
    if (alpha <= 0) return;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    int drawRadius = static_cast<int>(size * Camera::ZoomScale);
    if (drawRadius < 1) drawRadius = 1;
    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY), drawRadius, color, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void KnifeSlashEffect::Draw()
{
    float screenX = Camera::WorldToScreenX(x);
    float screenY = Camera::WorldToScreenY(y);

    float arcLength = 40.0f * Camera::ZoomScale;
    float cosA = std::cos(angle);
    float sinA = std::sin(angle);
    float perpX = -sinA * arcLength;
    float perpY = cosA * arcLength;

    int x1 = static_cast<int>(screenX - perpX);
    int y1 = static_cast<int>(screenY - perpY);
    int x2 = static_cast<int>(screenX + perpX);
    int y2 = static_cast<int>(screenY + perpY);

    int alpha = static_cast<int>(255.0f * (static_cast<float>(lifeTimer) / maxLife));
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    DrawLine(x1, y1, x2, y2, GetColor(255, 255, 255), 5);
    DrawLine(x1, y1, x2, y2, GetColor(255, 40, 40), 2);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void SparkParticle::Update()
{
    Effect::Update();
    x += vx;
    y += vy;
    vx *= 0.85f;
    vy *= 0.85f;
}

void SparkParticle::Draw()
{
    float screenX = Camera::WorldToScreenX(x);
    float screenY = Camera::WorldToScreenY(y);

    if (screenX < -50.0f || screenX > 1970.0f || screenY < -50.0f || screenY > 1130.0f)
        return;

    int alpha = static_cast<int>(255.0f * (static_cast<float>(lifeTimer) / maxLife));
    if (alpha <= 0) return;

    // 火花の尾引き（移動方向への伸び）
    float zoom = Camera::ZoomScale;
    float tailX = screenX - vx * 2.5f * zoom;
    float tailY = screenY - vy * 2.5f * zoom;

    // 加算ブレンドでリアルな発光感を表現
    SetDrawBlendMode(DX_BLENDMODE_ADD, alpha);

    // 火花の線（尾引き）
    DrawLine(static_cast<int>(tailX), static_cast<int>(tailY),
             static_cast<int>(screenX), static_cast<int>(screenY),
             color, (size > 1.5f) ? 2 : 1);

    // 火花先端の輝き
    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
               static_cast<int>(size), GetColor(255, 255, 220), TRUE);

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void MuzzleFlashEffect::Draw()
{
    float screenX = Camera::WorldToScreenX(x);
    float screenY = Camera::WorldToScreenY(y);

    if (screenX < -50.0f || screenX > 1970.0f || screenY < -50.0f || screenY > 1130.0f)
        return;

    float progress = static_cast<float>(lifeTimer) / maxLife; // 1.0 -> 0.0
    int alpha = static_cast<int>(255.0f * progress);
    if (alpha <= 0) return;

    float currentRadius = flashRadius * (0.6f + 0.4f * progress);

    // 撃った人の周りのみの局所的な環境光フラッシュオーラ（加算合成）
    float auraRadius = 220.0f * (0.7f + 0.3f * progress);
    SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(alpha * 0.35f));
    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY), static_cast<int>(auraRadius), GetColor(255, 160, 50), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(alpha * 0.60f));
    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY), static_cast<int>(auraRadius * 0.5f), GetColor(255, 220, 120), TRUE);

    // 加算合成でまばゆいマズルフラッシュ
    SetDrawBlendMode(DX_BLENDMODE_ADD, alpha);
    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY), static_cast<int>(currentRadius * 1.8f), GetColor(255, 120, 20), TRUE);
    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY), static_cast<int>(currentRadius), GetColor(255, 220, 100), TRUE);
    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY), static_cast<int>(currentRadius * 0.5f), GetColor(255, 255, 255), TRUE);

    // マズルフラッシュ扇形コーン（発砲方向）
    float cosA = std::cos(angle);
    float sinA = std::sin(angle);
    float coneLength = currentRadius * 2.5f;
    float coneWidth = currentRadius * 1.2f;

    float tipX = screenX + cosA * coneLength;
    float tipY = screenY + sinA * coneLength;

    float perpX = -sinA * (coneWidth * 0.5f);
    float perpY = cosA * (coneWidth * 0.5f);

    int x1 = static_cast<int>(screenX + perpX);
    int y1 = static_cast<int>(screenY + perpY);
    int x2 = static_cast<int>(screenX - perpX);
    int y2 = static_cast<int>(screenY - perpY);
    int x3 = static_cast<int>(tipX);
    int y3 = static_cast<int>(tipY);

    DrawTriangle(x1, y1, x2, y2, x3, y3, GetColor(255, 240, 150), TRUE);

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void WindGustParticle::Update()
{
    Effect::Update();
    x += vx;
    y += vy;
    // 空気の摩擦減衰
    vx *= 0.94f;
    vy *= 0.94f;
    length *= 0.96f;
}

void WindGustParticle::Draw()
{
    float screenX = Camera::WorldToScreenX(x);
    float screenY = Camera::WorldToScreenY(y);

    if (screenX < -200.0f || screenX > 2120.0f || screenY < -200.0f || screenY > 1280.0f)
        return;

    float progress = 1.0f - (static_cast<float>(lifeTimer) / maxLife);
    int alpha = static_cast<int>(240.0f * (1.0f - progress));
    if (alpha <= 0) return;

    float tailX = screenX - std::cos(angle) * (length * Camera::ZoomScale);
    float tailY = screenY - std::sin(angle) * (length * Camera::ZoomScale);

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    DrawLine(static_cast<int>(tailX), static_cast<int>(tailY),
             static_cast<int>(screenX), static_cast<int>(screenY),
             color, static_cast<int>(thickness * Camera::ZoomScale));
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void WindShockwaveEffect::Update()
{
    Effect::Update();
    startAngle += 0.08f; // 旋回する渦巻き
}

void WindShockwaveEffect::Draw()
{
    float screenX = Camera::WorldToScreenX(x);
    float screenY = Camera::WorldToScreenY(y);

    if (screenX < -300.0f || screenX > 2220.0f || screenY < -300.0f || screenY > 1380.0f)
        return;

    float progress = 1.0f - (static_cast<float>(lifeTimer) / maxLife); // 0.0 -> 1.0
    // イージング（急激に拡大して外側へ抜ける）
    float easeRadius = std::sin(progress * 1.5707963f);
    float currentRadius = maxRadius * easeRadius * Camera::ZoomScale;

    int alpha = static_cast<int>(255.0f * (1.0f - progress * progress));
    if (alpha <= 0) return;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

    // 1. 強烈な極太白熱・風圧メインリング (幅12px相当の多重同心円)
    for (int ring = -5; ring <= 5; ++ring)
    {
        float r = currentRadius + ring * Camera::ZoomScale;
        if (r > 2.0f) {
            int ringAlpha = alpha - std::abs(ring) * 22;
            if (ringAlpha > 0) {
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, ringAlpha);
                unsigned int rCol = (std::abs(ring) <= 1) ? GetColor(255, 255, 255) : GetColor(200, 245, 255);
                DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY), static_cast<int>(r), rCol, FALSE);
            }
        }
    }

    // 2. インナー風圧リング（淡いエメラルド・シアンのグラデーション気流）
    if (currentRadius > 30.0f) {
        int innerAlpha = static_cast<int>(alpha * 0.75f);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, innerAlpha);
        DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY), static_cast<int>(currentRadius * 0.88f), GetColor(180, 250, 240), FALSE);
        DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY), static_cast<int>(currentRadius * 0.72f), GetColor(140, 230, 220), FALSE);
        DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY), static_cast<int>(currentRadius * 0.52f), GetColor(100, 210, 200), FALSE);
    }

    // 3. 突風のうねり・スパイラル気流リボン（竜巻・ハリケーン風の回転スジ）
    int arcSegments = 16;
    for (int i = 0; i < arcSegments; ++i)
    {
        float a1 = startAngle + (i / static_cast<float>(arcSegments)) * 6.2831853f;
        float a2 = a1 + 0.45f;

        float r1 = currentRadius * 0.75f;
        float r2 = currentRadius * 1.15f;

        int ax1 = static_cast<int>(screenX + std::cos(a1) * r1);
        int ay1 = static_cast<int>(screenY + std::sin(a1) * r1);
        int ax2 = static_cast<int>(screenX + std::cos(a2) * r2);
        int ay2 = static_cast<int>(screenY + std::sin(a2) * r2);

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(alpha * 0.9f));
        DrawLine(ax1, ay1, ax2, ay2, GetColor(255, 255, 255), static_cast<int>(5 * Camera::ZoomScale));
        DrawLine(ax1 + 1, ay1 + 1, ax2 + 1, ay2 + 1, GetColor(180, 245, 255), static_cast<int>(3 * Camera::ZoomScale));
    }

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void SonarWaveEffect::Draw()
{
    float screenX = Camera::WorldToScreenX(x);
    float screenY = Camera::WorldToScreenY(y);

    if (screenX < -200.0f || screenX > 2120.0f || screenY < -200.0f || screenY > 1280.0f)
        return;

    float progress = 1.0f - (static_cast<float>(lifeTimer) / maxLife); // 0.0 -> 1.0
    float currentRadius = maxRadius * progress * Camera::ZoomScale;
    int alpha = static_cast<int>(220.0f * (1.0f - progress));
    if (alpha <= 0) return;

    unsigned int drawCol = (color != 0) ? color : GetColor(0, 220, 255);

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY), static_cast<int>(currentRadius), drawCol, FALSE);
    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY), static_cast<int>(currentRadius * 0.95f), drawCol, FALSE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}


void FleshGoreParticle::Update()
{
    Effect::Update();
    x += vx;
    y += vy;
    vx *= 0.92f;
    vy *= 0.92f;
    rotAngle += rotSpeed;
}

void FleshGoreParticle::Draw()
{
    float screenX = Camera::WorldToScreenX(x);
    float screenY = Camera::WorldToScreenY(y);

    if (screenX < -50.0f || screenX > 1970.0f || screenY < -50.0f || screenY > 1130.0f)
        return;

    int alpha = static_cast<int>(255.0f * (static_cast<float>(lifeTimer) / maxLife));
    if (alpha <= 0) return;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    float drawRadius = size * Camera::ZoomScale;
    if (drawRadius < 1.5f) drawRadius = 1.5f;

    // 多角形・肉片のいびつな形状描画
    float cosR = std::cos(rotAngle);
    float sinR = std::sin(rotAngle);
    float p1X = screenX + cosR * drawRadius;
    float p1Y = screenY + sinR * drawRadius;
    float p2X = screenX + std::cos(rotAngle + 2.2f) * (drawRadius * 0.8f);
    float p2Y = screenY + std::sin(rotAngle + 2.2f) * (drawRadius * 0.8f);
    float p3X = screenX + std::cos(rotAngle - 1.9f) * (drawRadius * 1.2f);
    float p3Y = screenY + std::sin(rotAngle - 1.9f) * (drawRadius * 1.2f);

    DrawTriangle(static_cast<int>(p1X), static_cast<int>(p1Y),
                 static_cast<int>(p2X), static_cast<int>(p2Y),
                 static_cast<int>(p3X), static_cast<int>(p3Y),
                 color, TRUE);

    // 肉片の黒ずんだ縁
    DrawTriangle(static_cast<int>(p1X), static_cast<int>(p1Y),
                 static_cast<int>(p2X), static_cast<int>(p2Y),
                 static_cast<int>(p3X), static_cast<int>(p3Y),
                 GetColor(40, 0, 0), FALSE);

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

BloodStainEffect::BloodStainEffect(float startX, float startY, float radius, int life)
    : Effect(startX, startY, life), baseRadius(radius)
{
    color = GetColor(110 + (rand() % 30), 0, 0); // どす黒い血液色

    // メインの血溜まり＋周囲に飛び散った血痕スプラッター
    int dropCount = 10 + (rand() % 8);
    for (int i = 0; i < dropCount; ++i)
    {
        float ang = (static_cast<float>(rand()) / RAND_MAX) * 6.2831853f;
        float dist = (static_cast<float>(rand()) / RAND_MAX) * (baseRadius * 1.3f);
        float r = 3.0f + (static_cast<float>(rand()) / RAND_MAX) * (baseRadius * 0.45f);
        drops.push_back({ std::cos(ang) * dist, std::sin(ang) * dist, r });
    }
}

void BloodStainEffect::Update()
{
    Effect::Update();
}

void BloodStainEffect::Draw()
{
    float screenX = Camera::WorldToScreenX(x);
    float screenY = Camera::WorldToScreenY(y);

    if (screenX < -150.0f || screenX > 2070.0f || screenY < -150.0f || screenY > 1230.0f)
        return;

    // 長時間残ってゆっくり薄くなる
    float ratio = static_cast<float>(lifeTimer) / maxLife;
    int alpha = static_cast<int>(200.0f * (ratio > 0.4f ? 1.0f : (ratio / 0.4f)));
    if (alpha <= 0) return;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

    // 中心メインの血溜まり
    DrawCircle(static_cast<int>(screenX), static_cast<int>(screenY),
               static_cast<int>(baseRadius * 0.75f * Camera::ZoomScale), color, TRUE);

    // 周囲に飛び散った飛び血
    for (const auto& d : drops)
    {
        float dx = screenX + d.ox * Camera::ZoomScale;
        float dy = screenY + d.oy * Camera::ZoomScale;
        int dr = static_cast<int>(d.r * Camera::ZoomScale);
        if (dr < 1) dr = 1;
        DrawCircle(static_cast<int>(dx), static_cast<int>(dy), dr, color, TRUE);
    }

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

EffectManager::EffectManager()
{
}

EffectManager::~EffectManager()
{
    Clear();
}

void EffectManager::AddEffect(Effect *effect)
{
    effects.push_back(effect);
}

void EffectManager::AddBloodEffect(float worldX, float worldY, int count)
{
    for (int i = 0; i < count; ++i)
    {
        float angle = (static_cast<float>(rand()) / RAND_MAX) * 6.2831853f;
        float speed = 1.5f + (static_cast<float>(rand()) / RAND_MAX) * 5.5f;
        float vx = std::cos(angle) * speed;
        float vy = std::sin(angle) * speed;
        float pSize = 2.5f + (static_cast<float>(rand()) / RAND_MAX) * 4.0f;
        int life = 18 + rand() % 22;

        int colorType = rand() % 3;
        unsigned int col = GetColor(180, 0, 0);
        if (colorType == 1) col = GetColor(230, 20, 20);
        else if (colorType == 2) col = GetColor(120, 0, 0);

        AddEffect(new BloodParticle(worldX, worldY, vx, vy, pSize, life, col));
    }
}

void EffectManager::AddKnifeSlashEffect(float worldX, float worldY, float dirAngle)
{
    AddEffect(new KnifeSlashEffect(worldX, worldY, dirAngle));
}

void EffectManager::AddMuzzleFlashEffect(float worldX, float worldY, float dirAngle, float radius)
{
    AddEffect(new MuzzleFlashEffect(worldX, worldY, dirAngle, radius));

    // 飛び散るリアルな火花（スパーク）粒子の多数生成
    int sparkCount = 12;
    for (int i = 0; i < sparkCount; ++i)
    {
        // 扇状の広い散乱（前方に広がるように）
        float spread = ((static_cast<float>(rand()) / RAND_MAX) - 0.5f) * 1.2f; // 約±34度
        float speed = 4.0f + (static_cast<float>(rand()) / RAND_MAX) * 10.0f;
        float sparkAngle = dirAngle + spread;
        float vx = std::cos(sparkAngle) * speed;
        float vy = std::sin(sparkAngle) * speed;
        float pSize = 1.2f + (static_cast<float>(rand()) / RAND_MAX) * 2.2f;
        int life = 5 + rand() % 8;

        // 火花グラデーション（白熱色〜黄〜オレンジ〜赤）
        int colorType = rand() % 4;
        unsigned int col = GetColor(255, 255, 220); // 白熱
        if (colorType == 1) col = GetColor(255, 210, 80);  // 金黄
        else if (colorType == 2) col = GetColor(255, 140, 30); // オレンジ
        else if (colorType == 3) col = GetColor(255, 80, 20);  // 赤熱

        AddEffect(new SparkParticle(worldX, worldY, vx, vy, pSize, life, col));
    }

    // 発砲の瞬間に撃った人の周囲のみを一瞬明るくフラッシュ（5フレーム間）
    TriggerGunFlash(worldX, worldY, 5);
}

void EffectManager::AddSonarWaveEffect(float worldX, float worldY, unsigned int color)
{
    AddEffect(new SonarWaveEffect(worldX, worldY, 350.0f, 35, color));
}

void EffectManager::AddWindShockwaveEffect(float worldX, float worldY, float maxRadius)
{
    // 1. メインの突風衝撃波リング（多段展開：先陣波、本波、追従波の3層）
    AddEffect(new WindShockwaveEffect(worldX, worldY, maxRadius * 1.15f, 32));
    AddEffect(new WindShockwaveEffect(worldX, worldY, maxRadius * 0.85f, 26));
    AddEffect(new WindShockwaveEffect(worldX, worldY, maxRadius * 0.55f, 20));

    // 2. 放射状に猛烈な速度で吹き飛ぶ多数の極太風のスジ（気流パーティクル：72本）
    int gustCount = 72;
    for (int i = 0; i < gustCount; ++i)
    {
        float angle = (i / static_cast<float>(gustCount)) * 6.2831853f + ((rand() % 100) / 250.0f);
        float speed = 12.0f + (static_cast<float>(rand()) / RAND_MAX) * 22.0f;
        float vx = std::cos(angle) * speed;
        float vy = std::sin(angle) * speed;
        float len = 45.0f + (static_cast<float>(rand()) / RAND_MAX) * 55.0f;
        float thick = 3.5f + (static_cast<float>(rand()) / RAND_MAX) * 4.5f;
        int life = 22 + rand() % 16;

        // 風の色彩（純白〜淡いエメラルド〜シアン）
        int colType = rand() % 3;
        unsigned int col = GetColor(255, 255, 255);
        if (colType == 1) col = GetColor(215, 255, 245);
        else if (colType == 2) col = GetColor(185, 245, 255);

        // 中心から少しオフセットして発生
        float startR = 10.0f + (rand() % 40);
        float spawnX = worldX + std::cos(angle) * startR;
        float spawnY = worldY + std::sin(angle) * startR;

        AddEffect(new WindGustParticle(spawnX, spawnY, vx, vy, len, thick, life, col));
    }
}


void EffectManager::AddCruelExplosionEffect(float worldX, float worldY, float blastRadius)
{
    // 1. 地面に永続残留する特大血溜まり＆スプラッター血痕（約5秒間残留）
    AddEffect(new BloodStainEffect(worldX, worldY, blastRadius * 0.7f, 300));
    AddEffect(new BloodStainEffect(worldX + 25.0f, worldY - 15.0f, blastRadius * 0.4f, 260));
    AddEffect(new BloodStainEffect(worldX - 20.0f, worldY + 20.0f, blastRadius * 0.45f, 280));

    // 2. 圧倒的な量の鮮血・血しぶきパーティクル（120個の大量出血）
    AddBloodEffect(worldX, worldY, 120);

    // 3. 四方八方に激しく吹き飛ぶ肉片・骨片ゴア破片（35個）
    int goreCount = 35;
    for (int i = 0; i < goreCount; ++i)
    {
        float angle = (i / static_cast<float>(goreCount)) * 6.2831853f + ((rand() % 100) / 200.0f);
        float speed = 4.0f + (static_cast<float>(rand()) / RAND_MAX) * 14.0f;
        float vx = std::cos(angle) * speed;
        float vy = std::sin(angle) * speed;
        float pSize = 4.0f + (static_cast<float>(rand()) / RAND_MAX) * 6.5f;
        int life = 35 + rand() % 35;

        // 肉塊色〜黒ずんだ血肉〜骨片（赤黒・暗赤・白灰）
        int colType = rand() % 4;
        unsigned int col = GetColor(160, 15, 15);
        if (colType == 1) col = GetColor(100, 5, 5);      // どす黒い生肉
        else if (colType == 2) col = GetColor(200, 30, 30); // 鮮血肉片
        else if (colType == 3) col = GetColor(220, 215, 200); // 骨片

        AddEffect(new FleshGoreParticle(worldX, worldY, vx, vy, pSize, life, col));
    }

    // 4. 残虐な紅蓮と爆砕のマズルフラッシュ・衝撃光
    AddMuzzleFlashEffect(worldX, worldY, 0.0f, blastRadius * 1.2f);
    AddMuzzleFlashEffect(worldX, worldY, 1.57f, blastRadius * 0.8f);

    // 5. 禍々しい深紅・暗黒レッドの生体爆砕ショックウェーブ（多段）
    AddSonarWaveEffect(worldX, worldY, GetColor(255, 20, 20));
    AddSonarWaveEffect(worldX, worldY, GetColor(180, 0, 50));
    AddSonarWaveEffect(worldX, worldY, GetColor(255, 120, 0));

    // 画面全体に一瞬血のような赤熱ガンフラッシュを発生
    TriggerGunFlash(worldX, worldY, 2);
}

void EffectManager::Update()
{
    if (gunFlashTimer > 0)
    {
        gunFlashTimer--;
    }

    auto it = effects.begin();
    while (it != effects.end())
    {
        (*it)->Update();
        if ((*it)->IsDead())
        {
            delete *it;
            it = effects.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void EffectManager::Draw()
{
    DrawGround();
    DrawForeground();
}

void EffectManager::DrawGround()
{
    for (auto eff : effects)
    {
        if (eff->IsGroundLayer())
        {
            eff->Draw();
        }
    }
}

void EffectManager::DrawForeground()
{
    for (auto eff : effects)
    {
        if (!eff->IsGroundLayer())
        {
            eff->Draw();
        }
    }
}

void EffectManager::Clear()
{
    for (auto eff : effects)
    {
        delete eff;
    }
    effects.clear();
}


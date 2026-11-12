#include "ClearScene.h"
#include "SoundManager.h"
#include "DxLib.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "GameScene.h"
#include "TitleScene.h"
#include <cstdio>
#include <cmath>

ClearScene::ClearScene(const ClearStats& inStats)
    : stats(inStats), menuCursor(0), animTimer(0.0f)
{
    // =========================================================================
    // 【2D画像配列への読み込み & 番号管理】
    // 一度読み込むと内部配列にキャッシュされ、配列番号（インデックス）で何度でも呼び出せます。
    // =========================================================================
    clearBgIndex = Load2DImage("clear_bg.png");
    if (clearBgIndex == -1) {
        clearBgIndex = Load2DImage("result_clear_bg.png");
    }
    windowFrameIndex = Load2DImage("ui_window.png"); // 9スライス用枠画像

    // ゲームクリアロゴ画像の読み込み
    clearLogoIndex = Load2DImage("logo_gameclear.png");
    if (clearLogoIndex == -1) {
        clearLogoIndex = Load2DImage("UI/Clear/logo_gameclear.png");
    }
    if (clearLogoIndex == -1) {
        clearLogoIndex = Load2DImage("Images/UI/Clear/logo_gameclear.png");
    }

    // UITextWindow によるリザルト枠の初期化
    statsWindow = new UI::UITextWindow();
    statsWindow->SetFixedSize(700, 380);
    statsWindow->SetMaxTextWidth(640);
    statsWindow->SetPadding(UI::Margin(40, 25, 40, 25));
    statsWindow->SetLineSpacing(14);
    statsWindow->SetBoxColor(GetColor(20, 30, 45), GetColor(0, 200, 255), 190);
    if (windowFrameIndex != -1) {
        statsWindow->SetBgGraphHandle(Get2DImage(windowFrameIndex));
    }
}

ClearScene::~ClearScene()
{
    delete statsWindow;
    statsWindow = nullptr;
}

void ClearScene::Init()
{
    // クリア音・ファンファーレBGMを再生
    SoundManager::GetInstance().PlayBGM("clear_bgm", 0.85f);
    Scene::Init();
    animTimer = 0.0f;
    prevEnter = true; // 前のシーンの押しっぱなし事故防止
}

void ClearScene::Update()
{
    Scene::Update();

    animTimer += 0.016f; // ~60 FPS timer update

    bool currUp = (CheckHitKey(KEY_INPUT_UP) != 0 || CheckHitKey(KEY_INPUT_W) != 0);
    bool currDown = (CheckHitKey(KEY_INPUT_DOWN) != 0 || CheckHitKey(KEY_INPUT_S) != 0);
    bool currEnter = (CheckHitKey(KEY_INPUT_RETURN) != 0 || CheckHitKey(KEY_INPUT_SPACE) != 0 || (GetMouseInput() & MOUSE_INPUT_LEFT) != 0);

    if (!currEnter)
    {
        hasReleasedConfirm = true;
    }

    if (currUp && !prevUp)
    {
        menuCursor = (menuCursor - 1 + 2) % 2;
    }
    if (currDown && !prevDown)
    {
        menuCursor = (menuCursor + 1) % 2;
    }

    // シーン遷移後2秒間＆一度キーを離すまでは決定操作をロック（連打・押しっぱなしによる意図しない即時選択を防止）
    bool canConfirm = (animTimer >= 2.0f) && hasReleasedConfirm;
    if (canConfirm && currEnter && !prevEnter)
    {
        SoundManager::GetInstance().PlayTapSE();
        if (menuCursor == 0)
        {
            // もう一度プレイ
            SceneManager::GetInstance().ChangeScene(std::make_shared<GameScene>());
        }
        else if (menuCursor == 1)
        {
            // タイトルへ戻る
            SceneManager::GetInstance().ChangeScene(std::make_shared<TitleScene>());
        }
    }

    prevUp = currUp;
    prevDown = currDown;
    prevEnter = currEnter;
}

void ClearScene::Draw()
{
    Scene::Draw();

    // 背景描画（配列番号から呼び出し）
    if (clearBgIndex != -1)
    {
        DrawFullScreen2DImage(clearBgIndex, false);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);
        DrawBox(0, 0, 1920, 1080, GetColor(10, 15, 25), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
    else
    {
        // 画像がない場合は暗いグラデーション背景
        DrawBox(0, 0, 1920, 1080, GetColor(10, 15, 25), TRUE);
    }

    // 装飾背景ライン (走査線アニメーション演出 - sou からの復元)
    for (int i = 0; i < 10; ++i)
    {
        int y = (i * 120 + static_cast<int>(animTimer * 50.0f)) % 1080;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 35);
        DrawLine(0, y, 1920, y, GetColor(0, 200, 255), 1);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    const int centerX = 1920 / 2;

    // タイトルアニメーション (優雅な浮遊＆脈動エフェクト)
    float pulse = sinf(animTimer * 4.0f) * 4.0f;
    int logoW = 720;
    int logoH = 162; // 960x216 のアスペクト比維持 (0.75倍)
    int logoX = centerX - logoW / 2;
    int logoY = static_cast<int>(50 + pulse);

    int logoHandle = Get2DImage(clearLogoIndex);
    if (logoHandle != -1)
    {
        // 背後の上品なゴールドグロー光彩演出
        float glowPulse = std::abs(sinf(animTimer * 3.5f));
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(35 + glowPulse * 30));
        DrawBox(logoX - 25, logoY - 12, logoX + logoW + 25, logoY + logoH + 12, GetColor(230, 190, 40), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // GAME CLEAR ロゴ画像の描画
        DrawExtendGraph(logoX, logoY, logoX + logoW, logoY + logoH, logoHandle, TRUE);

        // 1P/2P 対戦勝利時のサブタイトルバナー表示（該当する場合のみ）
        if (stats.rankName == "1P WIN" || stats.rankName == "2P WIN")
        {
            SetFontSize(24);
            const char* subText = (stats.rankName == "1P WIN") ? "★ 1P (青) VICTORY ★" : "★ 2P (赤) VICTORY ★";
            unsigned int subCol = (stats.rankName == "1P WIN") ? GetColor(100, 220, 255) : GetColor(255, 110, 110);
            int subW = GetDrawStringWidth(subText, -1);
            DrawString(centerX - subW / 2 + 2, logoY + logoH + 4, subText, GetColor(0, 0, 0));
            DrawString(centerX - subW / 2, logoY + logoH + 2, subText, subCol);
        }
    }
    else
    {
        // 画像未読み込み時のフォールバックテキスト
        SetFontSize(48);
        std::string headerText = "STAGE CLEAR!";
        unsigned int headerColor = GetColor(255, 215, 0);
        if (stats.rankName == "1P WIN")
        {
            headerText = "1P (青) VICTORY!";
            headerColor = GetColor(80, 200, 255);
        }
        else if (stats.rankName == "2P WIN")
        {
            headerText = "2P (赤) VICTORY!";
            headerColor = GetColor(255, 80, 80);
        }

        int titleWidth = GetDrawStringWidth(headerText.c_str(), -1);
        DrawString(centerX - titleWidth / 2 + 4, logoY + 4, headerText.c_str(), GetColor(0, 0, 0));
        DrawString(centerX - titleWidth / 2, logoY, headerText.c_str(), headerColor);
    }

    // リザルト枠（UITextWindow による描画）
    int panelW = 740;
    int panelH = 370;
    int panelX = centerX - panelW / 2;
    int panelY = 240;

    if (statsWindow)
    {
        statsWindow->SetFixedSize(panelW, panelH);
        statsWindow->SetTitle(""); // タイトルはパネル中央に美しく自前描画
        statsWindow->Draw(panelX, panelY);
    }
    else
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 190);
        DrawBox(panelX, panelY, panelX + panelW, panelY + panelH, GetColor(20, 30, 45), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawBox(panelX, panelY, panelX + panelW, panelY + panelH, GetColor(0, 200, 255), FALSE);
    }

    // パネル上部タイトル「STAGE RESULT」を中央揃えで描画
    SetFontSize(28);
    const char* resultTitle = "STAGE RESULT";
    int resTitleW = GetDrawStringWidth(resultTitle, -1);
    DrawString(centerX - resTitleW / 2, panelY + 24, resultTitle, GetColor(255, 220, 0));
    DrawLine(panelX + 40, panelY + 62, panelX + panelW - 40, panelY + 62, GetColor(0, 180, 240));

    // スコア＆統計情報表示 (区切り線の下から整然と配置)
    SetFontSize(24);
    int itemStartY = panelY + 85;
    int itemStep = 56;

    int minutes = static_cast<int>(stats.clearTimeSec) / 60;
    float seconds = fmodf(stats.clearTimeSec, 60.0f);
    char timeStr[64];
    snprintf(timeStr, sizeof(timeStr), "クリアタイム : %02d:%05.2f", minutes, seconds);

    char enemyStr[64];
    snprintf(enemyStr, sizeof(enemyStr), "撃破数       : %d / %d", stats.defeatedEnemies, stats.totalEnemies);

    char scoreStr[64];
    snprintf(scoreStr, sizeof(scoreStr), "最終スコア   : %d pts", stats.rankScore);

    DrawString(panelX + 80, itemStartY + 0 * itemStep, timeStr, GetColor(255, 255, 255));
    DrawString(panelX + 80, itemStartY + 1 * itemStep, enemyStr, GetColor(255, 255, 255));
    DrawString(panelX + 80, itemStartY + 2 * itemStep, scoreStr, GetColor(255, 255, 255));

    // ランク表示（「評価ランク   : 」と ランク大文字 "S" などの横位置・縦位置を精密に揃える）
    const char* rankLabel = "評価ランク   : ";
    DrawString(panelX + 80, itemStartY + 3 * itemStep, rankLabel, GetColor(255, 255, 255));

    int labelWidth = GetDrawStringWidth(rankLabel, -1);

    unsigned int rankColor = GetColor(255, 215, 0); // Gold for S
    if (stats.rankName == "A") rankColor = GetColor(200, 200, 255);
    else if (stats.rankName == "B") rankColor = GetColor(100, 255, 100);
    else if (stats.rankName == "C") rankColor = GetColor(200, 150, 100);

    // ランク文字を強調大文字フォント(38pt)で位置調整して描画
    SetFontSize(38);
    DrawString(panelX + 80 + labelWidth + 5, itemStartY + 3 * itemStep - 8, stats.rankName.c_str(), rankColor);

    // メニュー選択
    SetFontSize(28);
    const char* options[] = {
        "もう一度プレイ (RETRY)",
        "タイトル画面へ (TITLE)"
    };

    int menuStartY = 670;
    int menuSpacing = 60;

    for (int i = 0; i < 2; ++i)
    {
        int itemY = menuStartY + i * menuSpacing;
        unsigned int color = (i == menuCursor) ? GetColor(255, 255, 0) : GetColor(180, 180, 180);
        int optWidth = GetDrawStringWidth(options[i], -1);
        int optX = centerX - optWidth / 2;

        if (i == menuCursor)
        {
            // カーソル矢印
            float arrowOffset = sinf(animTimer * 8.0f) * 5.0f;
            DrawString(static_cast<int>(optX - 35 + arrowOffset), itemY, ">", color);
        }

        DrawString(optX, itemY, options[i], color);
    }

    if (animTimer >= 2.0f)
    {
        SetFontSize(22);
        const char* guideMsg = "[ W / S / 矢印キー ] 選択   [ Enter / Space / 左クリック ] 決定";
        int msgWidth = GetDrawStringWidth(guideMsg, -1);
        DrawString(centerX - msgWidth / 2, 820, guideMsg, GetColor(140, 180, 220));
    }

    SetFontSize(32); // フォントサイズをデフォルトに戻す
}

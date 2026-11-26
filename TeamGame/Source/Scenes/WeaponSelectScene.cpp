#include "WeaponSelectScene.h"
#include "SoundManager.h"
#include "GameScene.h"
#include "CharacterSelectScene.h"
#include "../Managers/SceneManager.h"
#include "../Managers/FontManager.h"
#include "../Managers/InputManager.h"
#include "../Managers/WeaponManager.h"
#include "../UI/HudStyle.h"
#include <DxLib.h>

WeaponSelectScene::WeaponSelectScene() {
}

void WeaponSelectScene::Init() {
    // タイトル〜武器選択までBGMを継続再生
    SoundManager::GetInstance().PlayBGM("title_bgm", 0.85f);
    m_bgImageIndex = Load2DImage("weapon_select_bg.png");
    if (m_bgImageIndex == -1) {
        m_bgImageIndex = Load2DImage("select_bg.png");
    }
    if (m_bgImageIndex == -1) {
        m_bgImageIndex = Load2DImage("Titleu.jpg");
    }
    m_windowFrameIndex   = Load2DImage("ui_window.png");
    m_cardIdleImgIndex   = Load2DImage("Images/UI/PreGame/panel_card_idle.png");
    m_cardSelImgIndex    = Load2DImage("Images/UI/PreGame/panel_card_selected.png");
    m_modalImgIndex      = Load2DImage("Images/UI/PreGame/panel_modal.png");
    m_stripImgIndex      = Load2DImage("Images/UI/PreGame/panel_strip.png");
    m_btnIdleImgIndex    = Load2DImage("Images/UI/PreGame/btn_idle.png");
    m_btnSelImgIndex     = Load2DImage("Images/UI/PreGame/btn_selected.png");
    m_underlineImgIndex  = Load2DImage("Images/UI/PreGame/header_underline.png");
    m_wpnHandgunImgIndex = Load2DImage("Images/UI/PreGame/wpn_handgun.png");
    m_wpnShotgunImgIndex = Load2DImage("Images/UI/PreGame/wpn_shotgun.png");
    m_wpnSniperImgIndex  = Load2DImage("Images/UI/PreGame/wpn_sniper.png");
    m_wpnSmgImgIndex     = Load2DImage("Images/UI/PreGame/wpn_smg.png");

    m_slot1Weapon = PlayerSelectionManager::GetFixedSlot1Weapon();
    m_slot2Options = PlayerSelectionManager::GetAvailableSlot2Weapons();
    m_selectedSlot2Index = 0;
    m_state = SelectState::SELECTING;
    m_confirmCursor = 0;
    m_hasReleasedConfirm = false;
}

void WeaponSelectScene::Update() {
    int count = static_cast<int>(m_slot2Options.size());
    auto& input = InputManager::GetInstance();

    // マウス座標の取得（ウィンドウ拡縮対応）
    int mouseX = 0, mouseY = 0;
    GetMousePoint(&mouseX, &mouseY);
    int drawW = 1920, drawH = 1080;
    GetDrawScreenSize(&drawW, &drawH);
    if (drawW > 0 && drawH > 0 && (drawW != 1920 || drawH != 1080)) {
        mouseX = static_cast<int>(mouseX * (1920.0f / drawW));
        mouseY = static_cast<int>(mouseY * (1080.0f / drawH));
    }

    bool currMouseLeft = ((GetMouseInput() & MOUSE_INPUT_LEFT) != 0);
    bool mouseClicked = (currMouseLeft && !m_prevMouseLeft);
    bool mouseMoved = (m_prevMouseX != -1 && (mouseX != m_prevMouseX || mouseY != m_prevMouseY));

    // 誤進行防止
    bool anyConfirmHeld = (input.IsKeyHeld(KEY_INPUT_Z) || input.IsKeyHeld(KEY_INPUT_RETURN) || input.IsKeyHeld(KEY_INPUT_SPACE) || currMouseLeft || input.IsPadConfirmHeld());
    if (!anyConfirmHeld) {
        m_hasReleasedConfirm = true;
    }

    if (m_state == SelectState::SELECTING) {
        // --- 画面左上: 戻るボタン ---
        const int backBtnX = 50;
        const int backBtnY = 40;
        const int backBtnW = 160;
        const int backBtnH = 44;
        if (mouseX >= backBtnX && mouseX <= backBtnX + backBtnW &&
            mouseY >= backBtnY && mouseY <= backBtnY + backBtnH) {
            if (mouseClicked && m_hasReleasedConfirm) {
                SoundManager::GetInstance().PlayTapSE();
                SceneManager::GetInstance().ChangeScene(std::make_shared<CharacterSelectScene>());
                m_prevMouseLeft = currMouseLeft;
                m_prevMouseX = mouseX;
                m_prevMouseY = mouseY;
                return;
            }
        }

        // --- 武器カードのタップ（マウスクリック＆ホバー）判定 ---
        int cardW = 460;
        int cardH = 520;
        int gap = 48;
        int totalW = count * cardW + (count - 1) * gap;
        int startX = (1920 - totalW) / 2;
        int startY = 290;

        for (int i = 0; i < count; i++) {
            int cardX = startX + i * (cardW + gap);
            if (mouseX >= cardX && mouseX <= cardX + cardW &&
                mouseY >= startY && mouseY <= startY + cardH) {
                if (mouseMoved && m_selectedSlot2Index != i) {
                    m_selectedSlot2Index = i;
                }
                if (mouseClicked && m_hasReleasedConfirm) {
                    m_selectedSlot2Index = i;
                    SoundManager::GetInstance().PlayTapSE();
                    m_state = SelectState::CONFIRMING;
                    m_confirmCursor = 0;
                    m_hasReleasedConfirm = false;
                }
            }
        }

        // --- キーボード・ゲームパッド操作 ---
        if (input.IsKeyPressed(KEY_INPUT_LEFT) || input.IsKeyPressed(KEY_INPUT_A) || input.IsPadLeftPressed()) {
            m_selectedSlot2Index = (m_selectedSlot2Index - 1 + count) % count;
        }
        if (input.IsKeyPressed(KEY_INPUT_RIGHT) || input.IsKeyPressed(KEY_INPUT_D) || input.IsPadRightPressed()) {
            m_selectedSlot2Index = (m_selectedSlot2Index + 1) % count;
        }

        // 決定キー（Z, Enter, Space, パッドA）で確認画面へ
        if (m_hasReleasedConfirm && (input.IsKeyPressed(KEY_INPUT_Z) || input.IsKeyPressed(KEY_INPUT_RETURN) || input.IsKeyPressed(KEY_INPUT_SPACE) || input.IsPadConfirmPressed())) {
            if (m_selectedSlot2Index >= 0 && m_selectedSlot2Index < count) {
                SoundManager::GetInstance().PlayTapSE();
                m_state = SelectState::CONFIRMING;
                m_confirmCursor = 0;
                m_hasReleasedConfirm = false;
            }
        }

        // ESCキー / パッドBでキャラクター選択画面へ戻る
        if (input.IsKeyPressed(KEY_INPUT_ESCAPE) || input.IsPadCancelPressed()) {
            SceneManager::GetInstance().ChangeScene(std::make_shared<CharacterSelectScene>());
            m_prevMouseLeft = currMouseLeft;
            m_prevMouseX = mouseX;
            m_prevMouseY = mouseY;
            return;
        }
    }
    else if (m_state == SelectState::CONFIRMING) {
        // --- 確認ダイアログ内のボタン当たり判定 ---
        const int modalW = 860;
        const int modalH = 440;
        const int modalX = (1920 - modalW) / 2;
        const int modalY = (1080 - modalH) / 2;

        const int btnW = 320;
        const int btnH = 56;
        const int btnY = modalY + modalH - 84;
        const int btn0X = modalX + 80;
        const int btn1X = modalX + modalW - 80 - btnW;

        // ボタン0: 出撃開始 (START GAME)
        if (mouseX >= btn0X && mouseX <= btn0X + btnW && mouseY >= btnY && mouseY <= btnY + btnH) {
            if (mouseMoved && m_confirmCursor != 0) m_confirmCursor = 0;
            if (mouseClicked && m_hasReleasedConfirm) {
                SoundManager::GetInstance().PlayTapSE();
                PlayerSelectionManager::GetInstance().SetSlot1Weapon(m_slot1Weapon);
                PlayerSelectionManager::GetInstance().SetSlot2Weapon(m_slot2Options[m_selectedSlot2Index]);
                SceneManager::GetInstance().ChangeScene(std::make_shared<GameScene>(PlayMode::SOLO));
                m_prevMouseLeft = currMouseLeft;
                m_prevMouseX = mouseX;
                m_prevMouseY = mouseY;
                return;
            }
        }

        // ボタン1: 装備を変更 (CANCEL)
        if (mouseX >= btn1X && mouseX <= btn1X + btnW && mouseY >= btnY && mouseY <= btnY + btnH) {
            if (mouseMoved && m_confirmCursor != 1) m_confirmCursor = 1;
            if (mouseClicked && m_hasReleasedConfirm) {
                SoundManager::GetInstance().PlayTapSE();
                m_state = SelectState::SELECTING;
                m_hasReleasedConfirm = false;
            }
        }

        // --- キーボード・ゲームパッド操作 ---
        if (input.IsKeyPressed(KEY_INPUT_LEFT) || input.IsKeyPressed(KEY_INPUT_RIGHT) ||
            input.IsKeyPressed(KEY_INPUT_A) || input.IsKeyPressed(KEY_INPUT_D) ||
            input.IsPadLeftPressed() || input.IsPadRightPressed()) {
            m_confirmCursor = (m_confirmCursor == 0) ? 1 : 0;
        }

        // Enter / Z / Space / パッドA で決定
        if (m_hasReleasedConfirm && (input.IsKeyPressed(KEY_INPUT_Z) || input.IsKeyPressed(KEY_INPUT_RETURN) || input.IsKeyPressed(KEY_INPUT_SPACE) || input.IsPadConfirmPressed())) {
            SoundManager::GetInstance().PlayTapSE();
            if (m_confirmCursor == 0) {
                PlayerSelectionManager::GetInstance().SetSlot1Weapon(m_slot1Weapon);
                PlayerSelectionManager::GetInstance().SetSlot2Weapon(m_slot2Options[m_selectedSlot2Index]);
                SceneManager::GetInstance().ChangeScene(std::make_shared<GameScene>(PlayMode::SOLO));
                m_prevMouseLeft = currMouseLeft;
                m_prevMouseX = mouseX;
                m_prevMouseY = mouseY;
                return;
            } else {
                m_state = SelectState::SELECTING;
                m_hasReleasedConfirm = false;
            }
        }

        // ESCキー / パッドBで確認をキャンセルして選択に戻る
        if (input.IsKeyPressed(KEY_INPUT_ESCAPE) || input.IsPadCancelPressed()) {
            SoundManager::GetInstance().PlayTapSE();
            m_state = SelectState::SELECTING;
            m_hasReleasedConfirm = false;
        }
    }

    m_prevMouseX = mouseX;
    m_prevMouseY = mouseY;
    m_prevMouseLeft = currMouseLeft;
}

void WeaponSelectScene::Draw() {
    int count = static_cast<int>(m_slot2Options.size());
    int cardW = 460;
    int cardH = 520;
    int gap = 48;
    int totalW = count * cardW + (count - 1) * gap;
    int startX = (1920 - totalW) / 2;
    int startY = 290;

    // 選択中カード背後を照らす懐中電灯スポットライト座標
    int spotX = (m_selectedSlot2Index >= 0 && m_selectedSlot2Index < count)
        ? (startX + m_selectedSlot2Index * (cardW + gap) + cardW / 2)
        : 960;
    int spotY = startY + cardH / 2 - 40;

    HudStyle::DrawTacticalBackdrop(1920, 1080, Get2DImage(m_bgImageIndex), spotX, spotY, 460);

    // マウス座標取得（ウィンドウ拡縮対応）
    int mouseX = 0, mouseY = 0;
    GetMousePoint(&mouseX, &mouseY);
    int drawW = 1920, drawH = 1080;
    GetDrawScreenSize(&drawW, &drawH);
    if (drawW > 0 && drawH > 0 && (drawW != 1920 || drawH != 1080)) {
        mouseX = static_cast<int>(mouseX * (1920.0f / drawW));
        mouseY = static_cast<int>(mouseY * (1080.0f / drawH));
    }

    // 画面左上: 戻るボタン
    const int backBtnX = 50;
    const int backBtnY = 40;
    const int backBtnW = 160;
    const int backBtnH = 44;
    bool isBackHovered = (mouseX >= backBtnX && mouseX <= backBtnX + backBtnW && mouseY >= backBtnY && mouseY <= backBtnY + backBtnH);

    HudStyle::DrawTexturedPanel(backBtnX, backBtnY, backBtnW, backBtnH,
                                Get2DImage(isBackHovered ? m_btnSelImgIndex : m_btnIdleImgIndex),
                                isBackHovered ? HudStyle::ColCardHover() : HudStyle::ColCard(),
                                isBackHovered ? HudStyle::ColAmber() : HudStyle::ColBorderSoft(),
                                235);
    FontManager::GetInstance().DrawStringCenter(backBtnX + backBtnW / 2, backBtnY + 12, "← 戻る", isBackHovered ? HudStyle::ColAmber() : HudStyle::ColText(), 18);

    // 選択中キャラを右上にシンプルに表示
    const auto& selChar = PlayerSelectionManager::GetInstance().GetSelection().selectedCharacter;
    char loadoutChip[128];
    snprintf(loadoutChip, sizeof(loadoutChip), "選択中キャラ：%s", selChar.name.c_str());
    int chipW = 300;
    int chipX = 1920 - 50 - chipW;
    HudStyle::DrawTexturedPanel(chipX, 40, chipW, 44, Get2DImage(m_btnIdleImgIndex), HudStyle::ColCard(), HudStyle::ColBorderSoft(), 225);
    FontManager::GetInstance().DrawStringCenter(chipX + chipW / 2, 52, loadoutChip, HudStyle::ColText(), 17);

    // ヘッダー（日本語＋血痕ブラシ下線）
    HudStyle::DrawSectionHeading(960, 52, "サブ武器選択", Get2DImage(m_underlineImgIndex), 46);

    // 固定メイン武器の1行バー（ハンドガン小アイコン付き）
    int mainBarW = 740;
    int mainBarX = (1920 - mainBarW) / 2;
    int mainBarY = 186;
    HudStyle::DrawTexturedPanel(mainBarX, mainBarY, mainBarW, 54, Get2DImage(m_stripImgIndex), HudStyle::ColCard(), HudStyle::ColBorderSoft(), 225);
    int hgHandle = Get2DImage(m_wpnHandgunImgIndex);
    if (hgHandle != -1) {
        DrawExtendGraph(mainBarX + 26, mainBarY + 4, mainBarX + 114, mainBarY + 50, hgHandle, TRUE);
    }
    char mainInfo[160];
    snprintf(mainInfo, sizeof(mainInfo), "メイン武器（固定）：ハンドガン　｜　威力 %d　装弾数 %d発",
             m_slot1Weapon.damage, m_slot1Weapon.maxAmmo);
    FontManager::GetInstance().DrawString(mainBarX + 132, mainBarY + 16, mainInfo, HudStyle::ColText(), 18);

    auto getWeaponNameJp = [](int wpnId) -> const char* {
        if (wpnId == 201) return "ショットガン";
        if (wpnId == 202) return "スナイパーライフル";
        if (wpnId == 203) return "サブマシンガン";
        return "ハンドガン";
    };

    // ----------------------------------------------------
    // SLOT 2 武器カード（手描きイラスト中心＋日本語ステータスバー）
    // ----------------------------------------------------
    for (int i = 0; i < count; i++) {
        const auto& wpn = m_slot2Options[i];
        int cardX = startX + i * (cardW + gap);
        bool isSelected = (i == m_selectedSlot2Index);
        int cardY = isSelected ? (startY - 12) : startY;

        HudStyle::DrawTexturedPanel(cardX, cardY, cardW, cardH,
                                    Get2DImage(isSelected ? m_cardSelImgIndex : m_cardIdleImgIndex),
                                    isSelected ? HudStyle::ColCardHover() : HudStyle::ColCard(),
                                    isSelected ? HudStyle::ColAmber() : HudStyle::ColBorderSoft(),
                                    isSelected ? 250 : 210);

        // 武器名（日本語・太字）
        const char* jpName = getWeaponNameJp(wpn.id);
        FontManager::GetInstance().DrawStringCenterShadow(cardX + cardW / 2, cardY + 28, jpName,
                                                          isSelected ? HudStyle::ColAmber() : HudStyle::ColText(), 28, 7);

        // 区切り線
        DrawLine(cardX + 46, cardY + 76, cardX + cardW - 46, cardY + 76, isSelected ? HudStyle::ColAmberDim() : HudStyle::ColBorderDim());

        // 手描き武器イラスト（未選択は少し暗くして懐中電灯コントラストを演出）
        int wpnImgIdx = (wpn.id == 201) ? m_wpnShotgunImgIndex
                      : (wpn.id == 202) ? m_wpnSniperImgIndex
                                        : m_wpnSmgImgIndex;
        int wpnHandle = Get2DImage(wpnImgIdx);
        if (wpnHandle != -1) {
            if (!isSelected) {
                SetDrawBright(152, 148, 138);
            }
            int drawW = isSelected ? 344 : 324;
            int drawH = isSelected ? 182 : 171;
            int drawX = cardX + (cardW - drawW) / 2;
            int drawY = cardY + 86 + (182 - drawH) / 2;
            DrawExtendGraph(drawX, drawY, drawX + drawW, drawY + drawH, wpnHandle, TRUE);
            if (!isSelected) {
                SetDrawBright(255, 255, 255);
            }
        } else {
            unsigned int bodyCol = isSelected ? HudStyle::ColText() : HudStyle::ColSubText();
            unsigned int accCol  = isSelected ? HudStyle::ColAmber() : HudStyle::ColBorder();
            HudStyle::DrawWeaponSilhouette(cardX + cardW / 2, cardY + 178, wpn.id, bodyCol, accCol);
        }

        // ステータスバー (威力 / 射程 / 装弾数)
        std::string lookupName = (wpn.id == 202) ? "SniperRifle" : (wpn.id == 203 ? "SubmachineGun" : "Shotgun");
        const WeaponData* wData = WeaponManager::GetInstance().GetWeaponData(lookupName);
        float rangeVal = wData ? wData->range : (wpn.id == 202 ? 500.0f : (wpn.id == 203 ? 200.0f : 150.0f));

        int statX = cardX + 44;
        int statW = cardW - 88;
        int statY = cardY + 278;

        char dmgBuf[16], rngBuf[16], ammoBuf[16];
        snprintf(dmgBuf, sizeof(dmgBuf), "%d", wpn.damage);
        snprintf(rngBuf, sizeof(rngBuf), "%.0f", rangeVal);
        snprintf(ammoBuf, sizeof(ammoBuf), "%d発", wpn.maxAmmo);

        unsigned int barDmg = isSelected ? HudStyle::ColEnemy() : HudStyle::ColBorder();
        unsigned int barSub = isSelected ? HudStyle::ColAlly() : HudStyle::ColBorder();
        HudStyle::DrawStatBar(statX, statY,      statW, "威力",   dmgBuf,  (std::min)(1.0f, wpn.damage / 36.0f),  barDmg, 14);
        HudStyle::DrawStatBar(statX, statY + 48, statW, "射程",   rngBuf,  (std::min)(1.0f, rangeVal / 500.0f),   barSub, 14);
        HudStyle::DrawStatBar(statX, statY + 96, statW, "装弾数", ammoBuf, (std::min)(1.0f, wpn.maxAmmo / 28.0f), barSub, 14);

        // 特徴1行テキスト
        const char* shortSummary = (wpn.id == 201) ? "近距離で頼れる散弾銃"
                                 : (wpn.id == 202) ? "遠くから一撃で狙い撃つ長射程銃"
                                                   : "連射力と装弾数に優れた短機関銃";
        FontManager::GetInstance().DrawStringCenter(cardX + cardW / 2, cardY + cardH - 54, shortSummary,
                                                    isSelected ? HudStyle::ColText() : HudStyle::ColSubText(), 16);
    }

    // 画面下部の控えめなキーバッジ案内
    {
        int hx = 670;
        hx += HudStyle::DrawKeyHintItem(hx, 936, "← / →", "選ぶ") + 28;
        hx += HudStyle::DrawKeyHintItem(hx, 936, "Enter / クリック", "決定") + 28;
        HudStyle::DrawKeyHintItem(hx, 936, "Esc", "戻る");
    }

    // ========================================================
    // 最終出撃確認モーダルダイアログ (CONFIRMING 状態)
    // ========================================================
    if (m_state == SelectState::CONFIRMING && m_selectedSlot2Index >= 0 && m_selectedSlot2Index < count) {
        const auto& selSlot2 = m_slot2Options[m_selectedSlot2Index];

        // 1. 全面暗転オーバーレイ
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 220);
        DrawBox(0, 0, 1920, 1080, GetColor(5, 6, 4), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 2. メインモーダル枠（手描き質感）
        const int modalW = 860;
        const int modalH = 440;
        const int modalX = (1920 - modalW) / 2;
        const int modalY = (1080 - modalH) / 2;

        HudStyle::DrawTexturedPanel(modalX, modalY, modalW, modalH, Get2DImage(m_modalImgIndex), HudStyle::ColBg(), HudStyle::ColBorder(), 252);

        int headerY = modalY + 38;
        FontManager::GetInstance().DrawStringCenterShadow(960, headerY, "この装備で出撃しますか？", HudStyle::ColText(), 28, 7);
        DrawLine(modalX + 60, headerY + 48, modalX + modalW - 60, headerY + 48, HudStyle::ColBorderSoft());

        // サマリー（3行・日本語表記）
        int infoCardX = modalX + 60;
        int infoCardY = headerY + 68;
        int infoCardW = modalW - 120;
        int infoCardH = 198;
        HudStyle::DrawTexturedPanel(infoCardX, infoCardY, infoCardW, infoCardH, Get2DImage(m_stripImgIndex), HudStyle::ColCard(), HudStyle::ColBorderSoft(), 230);

        int labelX = infoCardX + 34;
        int valX = infoCardX + 190;
        int rowY = infoCardY + 26;

        FontManager::GetInstance().DrawString(labelX, rowY, "キャラ", HudStyle::ColSubText(), 18);
        FontManager::GetInstance().DrawString(valX, rowY - 1, selChar.name.c_str(), HudStyle::ColText(), 21, 6);
        rowY += 56;

        DrawLine(infoCardX + 28, rowY - 14, infoCardX + infoCardW - 28, rowY - 14, HudStyle::ColBorderDim());
        FontManager::GetInstance().DrawString(labelX, rowY, "メイン武器", HudStyle::ColSubText(), 18);
        char slot1Line[128];
        snprintf(slot1Line, sizeof(slot1Line), "ハンドガン　（威力 %d ／ %d発）", m_slot1Weapon.damage, m_slot1Weapon.maxAmmo);
        FontManager::GetInstance().DrawString(valX, rowY - 1, slot1Line, HudStyle::ColAlly(), 20);
        rowY += 56;

        DrawLine(infoCardX + 28, rowY - 14, infoCardX + infoCardW - 28, rowY - 14, HudStyle::ColBorderDim());
        FontManager::GetInstance().DrawString(labelX, rowY, "サブ武器", HudStyle::ColAmber(), 18);
        char slot2Line[128];
        snprintf(slot2Line, sizeof(slot2Line), "%s　（威力 %d ／ %d発）", getWeaponNameJp(selSlot2.id), selSlot2.damage, selSlot2.maxAmmo);
        FontManager::GetInstance().DrawString(valX, rowY - 1, slot2Line, HudStyle::ColAmber(), 20, 6);

        // ボタン
        const int btnW = 320;
        const int btnH = 56;
        const int btnY = modalY + modalH - 84;
        const int btn0X = modalX + 80;
        const int btn1X = modalX + modalW - 80 - btnW;

        bool isSel0 = (m_confirmCursor == 0);
        HudStyle::DrawTexturedPanel(btn0X, btnY, btnW, btnH,
                                    Get2DImage(isSel0 ? m_btnSelImgIndex : m_btnIdleImgIndex),
                                    isSel0 ? HudStyle::ColCardHover() : HudStyle::ColCard(),
                                    isSel0 ? HudStyle::ColAmber() : HudStyle::ColBorderSoft(),
                                    245);
        FontManager::GetInstance().DrawStringCenter(btn0X + btnW / 2, btnY + 17, "出撃する", isSel0 ? HudStyle::ColAmber() : HudStyle::ColText(), 20, isSel0 ? 7 : 5);

        bool isSel1 = (m_confirmCursor == 1);
        HudStyle::DrawTexturedPanel(btn1X, btnY, btnW, btnH,
                                    Get2DImage(isSel1 ? m_btnSelImgIndex : m_btnIdleImgIndex),
                                    isSel1 ? HudStyle::ColCardHover() : HudStyle::ColCard(),
                                    isSel1 ? HudStyle::ColAmber() : HudStyle::ColBorderSoft(),
                                    245);
        FontManager::GetInstance().DrawStringCenter(btn1X + btnW / 2, btnY + 17, "選び直す", isSel1 ? HudStyle::ColText() : HudStyle::ColSubText(), 20, isSel1 ? 7 : 5);
    }
}

void WeaponSelectScene::Finalize() {
}

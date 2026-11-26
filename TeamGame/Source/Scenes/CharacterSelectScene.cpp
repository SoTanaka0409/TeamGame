#include "CharacterSelectScene.h"
#include "SoundManager.h"
#include "WeaponSelectScene.h"
#include "TitleScene.h"
#include "../Managers/SceneManager.h"
#include "../Managers/FontManager.h"
#include "../Managers/InputManager.h"
#include "../UI/HudStyle.h"
#include <DxLib.h>

CharacterSelectScene::CharacterSelectScene() {
}

CharacterSelectScene::~CharacterSelectScene() {
    Finalize();
}

void CharacterSelectScene::Init() {
    // タイトル〜キャラ選択までBGMを継続再生
    SoundManager::GetInstance().PlayBGM("title_bgm", 0.85f);
    // =========================================================================
    // 【2D画像配列への読み込み & 番号管理】
    // 一度読み込むと内部配列にキャッシュされ、配列番号（インデックス）で何度でも呼び出せます。
    // =========================================================================
    m_bgImageIndex = Load2DImage("character_select_bg.png");
    if (m_bgImageIndex == -1) {
        m_bgImageIndex = Load2DImage("select_bg.png");
    }
    if (m_bgImageIndex == -1) {
        m_bgImageIndex = Load2DImage("Titleu.jpg");
    }
    m_windowFrameIndex  = Load2DImage("ui_window.png");
    m_cardIdleImgIndex  = Load2DImage("Images/UI/PreGame/panel_card_idle.png");
    m_cardSelImgIndex   = Load2DImage("Images/UI/PreGame/panel_card_selected.png");
    m_modalImgIndex     = Load2DImage("Images/UI/PreGame/panel_modal.png");
    m_stripImgIndex     = Load2DImage("Images/UI/PreGame/panel_strip.png");
    m_btnIdleImgIndex   = Load2DImage("Images/UI/PreGame/btn_idle.png");
    m_btnSelImgIndex    = Load2DImage("Images/UI/PreGame/btn_selected.png");
    m_underlineImgIndex = Load2DImage("Images/UI/PreGame/header_underline.png");

    // UITextWindow による詳細説明枠の設定
    m_detailWindow.SetFixedSize(1200, 110);
    m_detailWindow.SetMaxTextWidth(1140);
    m_detailWindow.SetPadding(UI::Margin(30, 16, 30, 16));
    m_detailWindow.SetLineSpacing(6);
    if (m_windowFrameIndex != -1) {
        m_detailWindow.SetBgGraphHandle(Get2DImage(m_windowFrameIndex));
    }

    m_characters = PlayerSelectionManager::GetAvailableCharacters();
    m_selectedIndex = 0;
    m_state = SelectState::SELECTING;
    m_confirmCursor = 0;
    m_hasReleasedConfirm = false;

    // プレビュー画像のロード
    m_previewHandles.clear();
    for (const auto& chara : m_characters) {
        int handle = -1;
        if (!chara.previewPath.empty()) {
            handle = LoadGraph(chara.previewPath.c_str());
            if (handle == -1) {
                // 実行ディレクトリ差分およびASCII別名のフォールバック
                std::vector<std::string> fallbacks = {
                    "TeamGame/" + chara.previewPath,
                    "../" + chara.previewPath,
                    "../../" + chara.previewPath,
                    "Resource/Images/Characters/character_" + std::to_string(chara.id) + ".png",
                    "TeamGame/Resource/Images/Characters/character_" + std::to_string(chara.id) + ".png",
                    "Resource/GreenCharacter/preview.png"
                };
                for (const auto& fb : fallbacks) {
                    handle = LoadGraph(fb.c_str());
                    if (handle != -1) break;
                }
            }
        }
        m_previewHandles.push_back(handle);
    }
}

void CharacterSelectScene::Update() {
    int count = static_cast<int>(m_characters.size());
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

    // 誤進行防止: 決定キーが一度離されていることを検知
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
                auto titleScene = std::make_shared<TitleScene>();
                titleScene->state = TitleScene::TitleState::MAIN;
                titleScene->cursor = 0;
                SceneManager::GetInstance().ChangeScene(titleScene);
                m_prevMouseLeft = currMouseLeft;
                m_prevMouseX = mouseX;
                m_prevMouseY = mouseY;
                return;
            }
        }

        // --- キャラクターカードのタップ（マウスクリック＆ホバー）判定 ---
        int cardW = 316;
        int cardH = 440;
        int gap = 24;
        int totalW = count * cardW + (count - 1) * gap;
        int startX = (1920 - totalW) / 2;
        int startY = 185;

        for (int i = 0; i < count; i++) {
            int cardX = startX + i * (cardW + gap);
            if (mouseX >= cardX && mouseX <= cardX + cardW &&
                mouseY >= startY && mouseY <= startY + cardH) {
                if (mouseMoved && m_selectedIndex != i) {
                    m_selectedIndex = i;
                }
                if (mouseClicked && m_hasReleasedConfirm) {
                    m_selectedIndex = i;
                    SoundManager::GetInstance().PlayTapSE();
                    m_state = SelectState::CONFIRMING;
                    m_confirmCursor = 0;
                    m_hasReleasedConfirm = false;
                }
            }
        }

        // --- キーボード・ゲームパッド操作 ---
        if (input.IsKeyPressed(KEY_INPUT_LEFT) || input.IsKeyPressed(KEY_INPUT_A) || input.IsPadLeftPressed()) {
            m_selectedIndex = (m_selectedIndex - 1 + count) % count;
        }
        if (input.IsKeyPressed(KEY_INPUT_RIGHT) || input.IsKeyPressed(KEY_INPUT_D) || input.IsPadRightPressed()) {
            m_selectedIndex = (m_selectedIndex + 1) % count;
        }

        // 決定キー（Z, Enter, Space, パッドA）で確認画面へ
        if (m_hasReleasedConfirm && (input.IsKeyPressed(KEY_INPUT_Z) || input.IsKeyPressed(KEY_INPUT_RETURN) || input.IsKeyPressed(KEY_INPUT_SPACE) || input.IsPadConfirmPressed())) {
            if (m_selectedIndex >= 0 && m_selectedIndex < count) {
                SoundManager::GetInstance().PlayTapSE();
                m_state = SelectState::CONFIRMING;
                m_confirmCursor = 0;
                m_hasReleasedConfirm = false;
            }
        }

        // ESCキー / パッドBでタイトル画面へ戻る
        if (input.IsKeyPressed(KEY_INPUT_ESCAPE) || input.IsPadCancelPressed()) {
            auto titleScene = std::make_shared<TitleScene>();
            titleScene->state = TitleScene::TitleState::MAIN;
            titleScene->cursor = 0;
            SceneManager::GetInstance().ChangeScene(titleScene);
            m_prevMouseLeft = currMouseLeft;
            m_prevMouseX = mouseX;
            m_prevMouseY = mouseY;
            return;
        }
    }
    else if (m_state == SelectState::CONFIRMING) {
        // --- 確認ダイアログ内のボタン当たり判定 ---
        const int modalW = 880;
        const int modalH = 460;
        const int modalX = (1920 - modalW) / 2;
        const int modalY = (1080 - modalH) / 2;

        const int btnW = 340;
        const int btnH = 56;
        const int btnY = modalY + modalH - 84;
        const int btn0X = modalX + 70;
        const int btn1X = modalX + modalW - 70 - btnW;

        // ボタン0: 決定して武器選択へ (YES)
        if (mouseX >= btn0X && mouseX <= btn0X + btnW && mouseY >= btnY && mouseY <= btnY + btnH) {
            if (mouseMoved && m_confirmCursor != 0) m_confirmCursor = 0;
            if (mouseClicked && m_hasReleasedConfirm) {
                SoundManager::GetInstance().PlayTapSE();
                PlayerSelectionManager::GetInstance().SetSelectedCharacter(m_characters[m_selectedIndex]);
                SceneManager::GetInstance().ChangeScene(std::make_shared<WeaponSelectScene>());
                m_prevMouseLeft = currMouseLeft;
                m_prevMouseX = mouseX;
                m_prevMouseY = mouseY;
                return;
            }
        }

        // ボタン1: 選び直す (CANCEL)
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
                PlayerSelectionManager::GetInstance().SetSelectedCharacter(m_characters[m_selectedIndex]);
                SceneManager::GetInstance().ChangeScene(std::make_shared<WeaponSelectScene>());
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

void CharacterSelectScene::Draw() {
    int count = static_cast<int>(m_characters.size());
    int cardW = 316;
    int cardH = 440;
    int gap = 24;
    int totalW = count * cardW + (count - 1) * gap;
    int startX = (1920 - totalW) / 2;
    int startY = 185;

    // 選択中カード背後を照らす懐中電灯スポットライト座標
    int spotX = (m_selectedIndex >= 0 && m_selectedIndex < count)
        ? (startX + m_selectedIndex * (cardW + gap) + cardW / 2)
        : 960;
    int spotY = startY + cardH / 2 - 20;

    HudStyle::DrawTacticalBackdrop(1920, 1080, Get2DImage(m_bgImageIndex), spotX, spotY, 440);

    // 画面左上: 戻るボタン
    int mouseX = 0, mouseY = 0;
    GetMousePoint(&mouseX, &mouseY);
    int drawW = 1920, drawH = 1080;
    GetDrawScreenSize(&drawW, &drawH);
    if (drawW > 0 && drawH > 0 && (drawW != 1920 || drawH != 1080)) {
        mouseX = static_cast<int>(mouseX * (1920.0f / drawW));
        mouseY = static_cast<int>(mouseY * (1080.0f / drawH));
    }
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

    HudStyle::DrawSectionHeading(960, 52, "キャラクター選択", Get2DImage(m_underlineImgIndex), 46);

    for (int i = 0; i < count; i++) {
        const auto& chara = m_characters[i];
        int cardX = startX + i * (cardW + gap);
        bool isSelected = (i == m_selectedIndex);
        int cardY = isSelected ? (startY - 12) : startY;

        HudStyle::DrawTexturedPanel(cardX, cardY, cardW, cardH,
                                    Get2DImage(isSelected ? m_cardSelImgIndex : m_cardIdleImgIndex),
                                    isSelected ? HudStyle::ColCardHover() : HudStyle::ColCard(),
                                    isSelected ? HudStyle::ColAmber() : HudStyle::ColBorderSoft(),
                                    isSelected ? 250 : 210);

        // アバター領域（手描き立ち絵を主役に大きく表示）
        int avatarMargin = 20;
        int avatarH = 196;
        int avX1 = cardX + avatarMargin;
        int avY1 = cardY + 20;
        int avX2 = cardX + cardW - avatarMargin;
        int avY2 = avY1 + avatarH;

        DrawBox(avX1, avY1, avX2, avY2, HudStyle::ColCanvas(), TRUE);

        if (i < static_cast<int>(m_previewHandles.size()) && m_previewHandles[i] != -1) {
            int imgW = 0, imgH = 0;
            GetGraphSize(m_previewHandles[i], &imgW, &imgH);
            if (!isSelected) {
                SetDrawBright(152, 148, 138);
            }
            if (imgW > 0 && imgH > 0) {
                float targetW = static_cast<float>(avX2 - avX1 - 8);
                float targetH = static_cast<float>(avY2 - avY1 - 8);
                float scale = (std::min)(targetW / imgW, targetH / imgH);
                int fitW = static_cast<int>(imgW * scale);
                int fitH = static_cast<int>(imgH * scale);
                int fitX = avX1 + 4 + static_cast<int>((targetW - fitW) / 2.0f);
                int fitY = avY1 + 4 + static_cast<int>((targetH - fitH) / 2.0f);
                DrawExtendGraph(fitX, fitY, fitX + fitW, fitY + fitH, m_previewHandles[i], TRUE);
            } else {
                DrawExtendGraph(avX1 + 2, avY1 + 2, avX2 - 2, avY2 - 2, m_previewHandles[i], TRUE);
            }
            if (!isSelected) {
                SetDrawBright(255, 255, 255);
            }
        } else {
            int circleX = cardX + cardW / 2;
            int circleY = avY1 + avatarH / 2;
            DrawCircle(circleX, circleY, 34, isSelected ? HudStyle::ColAmber() : HudStyle::ColAlly(), TRUE);
            FontManager::GetInstance().DrawFormatString(circleX - 7, circleY - 12, GetColor(255, 255, 255), 24, "%d", chara.id);
        }
        DrawBox(avX1, avY1, avX2, avY2, isSelected ? HudStyle::ColAmberDim() : HudStyle::ColBorderDim(), FALSE);

        // キャラ名（太字でくっきり）
        int textY = avY2 + 14;
        int nameSize = FontManager::GetInstance().GetFittingFontSize(chara.name.c_str(), cardW - 36, 26, 18);
        FontManager::GetInstance().DrawStringCenterShadow(cardX + cardW / 2, textY, chara.name.c_str(), isSelected ? HudStyle::ColAmber() : HudStyle::ColText(), nameSize, 7);
        textY += 40;

        // ステータスバー (体力 / 素早さ)
        int statX = cardX + 26;
        int statW = cardW - 52;
        char hpVal[16], spdVal[16];
        snprintf(hpVal, sizeof(hpVal), "%d", chara.maxHp);
        snprintf(spdVal, sizeof(spdVal), "%.1f", chara.moveSpeed);
        HudStyle::DrawStatBar(statX, textY,      statW, "体力",   hpVal,  (std::min)(1.0f, chara.maxHp / 55.0f),    isSelected ? HudStyle::ColEnemy() : HudStyle::ColBorder(), 14);
        HudStyle::DrawStatBar(statX, textY + 44, statW, "素早さ", spdVal, (std::min)(1.0f, chara.moveSpeed / 2.2f), isSelected ? HudStyle::ColAlly() : HudStyle::ColBorder(), 14);
        textY += 94;

        // スキル表示（日本語ラベルでシンプルに1行）
        std::string skillLine = std::string("スキル：") + chara.skillName;
        int skillFit = FontManager::GetInstance().GetFittingFontSize(skillLine.c_str(), statW + 8, 15, 13);
        FontManager::GetInstance().DrawStringCenter(cardX + cardW / 2, textY, skillLine.c_str(), isSelected ? HudStyle::ColText() : HudStyle::ColSubText(), skillFit);
    }

    // 画面下部の詳細説明枠（手描き質感プレート）
    if (m_selectedIndex >= 0 && m_selectedIndex < count) {
        const auto& curChara = m_characters[m_selectedIndex];
        int infoX = startX;
        int infoY = startY + cardH + 34;
        int infoW = totalW;
        int infoH = 114;

        HudStyle::DrawTexturedPanel(infoX, infoY, infoW, infoH, Get2DImage(m_stripImgIndex), HudStyle::ColCard(), HudStyle::ColBorderSoft(), 238);

        FontManager::GetInstance().DrawFormatString(infoX + 34, infoY + 18, HudStyle::ColAmber(), 22, "%s", curChara.name.c_str());
        int nameW = FontManager::GetInstance().GetStringWidth(curChara.name.c_str(), 22);
        FontManager::GetInstance().DrawFormatString(infoX + 34 + nameW + 20, infoY + 21, HudStyle::ColSubText(), 17, "固有スキル：%s", curChara.skillName.c_str());
        FontManager::GetInstance().DrawStringWrapped(infoX + 34, infoY + 54, infoW - 68, curChara.description.c_str(), HudStyle::ColText(), 18, 4);
    }

    // 画面下部の控えめなキーバッジ案内
    {
        int hx = 670;
        hx += HudStyle::DrawKeyHintItem(hx, 936, "← / →", "選ぶ") + 28;
        hx += HudStyle::DrawKeyHintItem(hx, 936, "Enter / クリック", "決定") + 28;
        HudStyle::DrawKeyHintItem(hx, 936, "Esc", "戻る");
    }

    // ========================================================
    // 決定確認モーダルダイアログ (CONFIRMING 状態)
    // ========================================================
    if (m_state == SelectState::CONFIRMING && m_selectedIndex >= 0 && m_selectedIndex < count) {
        const auto& curChara = m_characters[m_selectedIndex];

        // 1. 全面暗転オーバーレイ
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 220);
        DrawBox(0, 0, 1920, 1080, GetColor(5, 6, 4), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 2. メインモーダル枠（手描き質感）
        const int modalW = 880;
        const int modalH = 460;
        const int modalX = (1920 - modalW) / 2;
        const int modalY = (1080 - modalH) / 2;

        HudStyle::DrawTexturedPanel(modalX, modalY, modalW, modalH, Get2DImage(m_modalImgIndex), HudStyle::ColBg(), HudStyle::ColBorder(), 252);

        int headerY = modalY + 38;
        FontManager::GetInstance().DrawStringCenterShadow(960, headerY, "このキャラクターで出撃しますか？", HudStyle::ColText(), 28, 7);
        DrawLine(modalX + 55, headerY + 48, modalX + modalW - 55, headerY + 48, HudStyle::ColBorderSoft());

        // キャラクター情報エリア
        int infoCardX = modalX + 55;
        int infoCardY = headerY + 68;
        int infoCardW = modalW - 110;
        int infoCardH = 216;

        HudStyle::DrawTexturedPanel(infoCardX, infoCardY, infoCardW, infoCardH, Get2DImage(m_stripImgIndex), HudStyle::ColCard(), HudStyle::ColBorderSoft(), 230);

        // アバター画像
        int avW = 176;
        int avH = infoCardH - 28;
        int avX = infoCardX + 18;
        int avY = infoCardY + 14;
        DrawBox(avX, avY, avX + avW, avY + avH, HudStyle::ColCanvas(), TRUE);
        if (m_selectedIndex < static_cast<int>(m_previewHandles.size()) && m_previewHandles[m_selectedIndex] != -1) {
            int imgW = 0, imgH = 0;
            GetGraphSize(m_previewHandles[m_selectedIndex], &imgW, &imgH);
            if (imgW > 0 && imgH > 0) {
                float targetW = static_cast<float>(avW - 6);
                float targetH = static_cast<float>(avH - 6);
                float scale = (std::min)(targetW / imgW, targetH / imgH);
                int fitW = static_cast<int>(imgW * scale);
                int fitH = static_cast<int>(imgH * scale);
                int fitX = avX + 3 + static_cast<int>((targetW - fitW) / 2.0f);
                int fitY = avY + 3 + static_cast<int>((targetH - fitH) / 2.0f);
                DrawExtendGraph(fitX, fitY, fitX + fitW, fitY + fitH, m_previewHandles[m_selectedIndex], TRUE);
            } else {
                DrawExtendGraph(avX + 2, avY + 2, avX + avW - 2, avY + avH - 2, m_previewHandles[m_selectedIndex], TRUE);
            }
        } else {
            int circleX = avX + avW / 2;
            int circleY = avY + avH / 2;
            DrawCircle(circleX, circleY, 34, HudStyle::ColAmber(), TRUE);
            FontManager::GetInstance().DrawFormatString(circleX - 7, circleY - 12, GetColor(255, 255, 255), 24, "%d", curChara.id);
        }
        DrawBox(avX, avY, avX + avW, avY + avH, HudStyle::ColAmberDim(), FALSE);

        // キャラステータス & スキル（日本語）
        int statX = avX + avW + 26;
        int textMaxW = (infoCardX + infoCardW - 24) - statX;
        int statY = infoCardY + 22;

        FontManager::GetInstance().DrawFormatString(statX, statY, HudStyle::ColAmber(), 26, "%s", curChara.name.c_str());
        statY += 42;

        FontManager::GetInstance().DrawFormatString(statX, statY, HudStyle::ColText(), 18, "体力：%d　　素早さ：%.1f　　スキル：%s",
                                                    curChara.maxHp, curChara.moveSpeed, curChara.skillName.c_str());
        statY += 38;

        FontManager::GetInstance().DrawStringWrapped(statX, statY, textMaxW, curChara.description.c_str(), HudStyle::ColSubText(), 18, 6);

        // 3. 2つのボタン
        const int btnW = 340;
        const int btnH = 56;
        const int btnY = modalY + modalH - 84;
        const int btn0X = modalX + 70;
        const int btn1X = modalX + modalW - 70 - btnW;

        bool isSel0 = (m_confirmCursor == 0);
        HudStyle::DrawTexturedPanel(btn0X, btnY, btnW, btnH,
                                    Get2DImage(isSel0 ? m_btnSelImgIndex : m_btnIdleImgIndex),
                                    isSel0 ? HudStyle::ColCardHover() : HudStyle::ColCard(),
                                    isSel0 ? HudStyle::ColAmber() : HudStyle::ColBorderSoft(),
                                    245);
        FontManager::GetInstance().DrawStringCenter(btn0X + btnW / 2, btnY + 17, "武器を選ぶ", isSel0 ? HudStyle::ColAmber() : HudStyle::ColText(), 20, isSel0 ? 7 : 5);

        bool isSel1 = (m_confirmCursor == 1);
        HudStyle::DrawTexturedPanel(btn1X, btnY, btnW, btnH,
                                    Get2DImage(isSel1 ? m_btnSelImgIndex : m_btnIdleImgIndex),
                                    isSel1 ? HudStyle::ColCardHover() : HudStyle::ColCard(),
                                    isSel1 ? HudStyle::ColAmber() : HudStyle::ColBorderSoft(),
                                    245);
        FontManager::GetInstance().DrawStringCenter(btn1X + btnW / 2, btnY + 17, "選び直す", isSel1 ? HudStyle::ColText() : HudStyle::ColSubText(), 20, isSel1 ? 7 : 5);
    }
}

void CharacterSelectScene::Finalize() {
    for (int h : m_previewHandles) {
        if (h != -1) {
            DeleteGraph(h);
        }
    }
    m_previewHandles.clear();
}

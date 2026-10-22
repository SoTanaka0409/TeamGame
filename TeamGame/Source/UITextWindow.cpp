#include "UITextWindow.h"

namespace UI {

UITextWindow::UITextWindow() {
}

void UITextWindow::SetText(const std::string& text) {
    if (m_rawText != text) {
        m_rawText = text;
        m_isDirty = true;
    }
}

void UITextWindow::SetMaxTextWidth(int maxTextWidth) {
    if (maxTextWidth > 0 && m_maxTextWidth != maxTextWidth) {
        m_maxTextWidth = maxTextWidth;
        m_isDirty = true;
    }
}

void UITextWindow::SetFontHandle(int fontHandle) {
    if (m_fontHandle != fontHandle) {
        m_fontHandle = fontHandle;
        m_isDirty = true;
    }
}

void UITextWindow::SetTextColor(unsigned int color) {
    m_textColor = color;
}

void UITextWindow::SetBgGraphHandle(int graphHandle) {
    m_bgGraphHandle = graphHandle;
}

void UITextWindow::SetPadding(const Margin& padding) {
    m_padding = padding;
    m_isDirty = true;
}

void UITextWindow::SetNineSliceMargin(const Margin& margin) {
    m_nineSliceMargin = margin;
    m_isDirty = true;
}

void UITextWindow::SetLineSpacing(int lineSpacing) {
    if (m_lineSpacing != lineSpacing) {
        m_lineSpacing = lineSpacing;
        m_isDirty = true;
    }
}

void UITextWindow::SetTextEncoding(TextEncoding encoding) {
    if (m_encoding != encoding) {
        m_encoding = encoding;
        m_isDirty = true;
    }
}

void UITextWindow::SetDrawTransparentFlag(int transparentFlag) {
    m_transFlag = transparentFlag;
}

void UITextWindow::SetTitle(const std::string& title, unsigned int titleColor) {
    if (m_title != title || m_titleColor != titleColor) {
        m_title = title;
        m_titleColor = titleColor;
        m_isDirty = true;
    }
}

void UITextWindow::SetBoxColor(unsigned int bgColor, unsigned int borderColor, int alpha) {
    m_boxBgColor = bgColor;
    m_boxBorderColor = borderColor;
    m_boxAlpha = alpha;
}

void UITextWindow::SetFixedSize(int width, int height) {
    if (m_fixedWidth != width || m_fixedHeight != height) {
        m_fixedWidth = width;
        m_fixedHeight = height;
        m_isDirty = true;
    }
}

int UITextWindow::GetWindowWidth() const {
    if (m_isDirty) const_cast<UITextWindow*>(this)->RecalculateLayout();
    return m_windowWidth;
}

int UITextWindow::GetWindowHeight() const {
    if (m_isDirty) const_cast<UITextWindow*>(this)->RecalculateLayout();
    return m_windowHeight;
}

int UITextWindow::GetTextWidth() const {
    if (m_isDirty) const_cast<UITextWindow*>(this)->RecalculateLayout();
    return m_calculatedTextWidth;
}

int UITextWindow::GetTextHeight() const {
    if (m_isDirty) const_cast<UITextWindow*>(this)->RecalculateLayout();
    return m_calculatedTextHeight;
}

const std::vector<std::string>& UITextWindow::GetWrappedLines() const {
    if (m_isDirty) const_cast<UITextWindow*>(this)->RecalculateLayout();
    return m_wrappedLines;
}

size_t UITextWindow::GetNextCharByteLen(const char* p) const {
    if (!p || *p == '\0') return 0;
    unsigned char c = static_cast<unsigned char>(*p);

    if (m_encoding == TextEncoding::UTF8) {
        if (c < 0x80) return 1;
        if ((c & 0xE0) == 0xC0) return 2;
        if ((c & 0xF0) == 0xE0) return 3;
        if ((c & 0xF8) == 0xF0) return 4;
        return 1;
    } else { // SJIS (Multi-Byte)
        if ((c >= 0x81 && c <= 0x9F) || (c >= 0xE0 && c <= 0xFC)) {
            return 2;
        }
        return 1;
    }
}

void UITextWindow::RecalculateLayout() {
    m_wrappedLines.clear();
    m_calculatedTextWidth = 0;
    m_calculatedTextHeight = 0;

    if (m_rawText.empty()) {
        m_windowWidth = m_padding.left + m_padding.right;
        m_windowHeight = m_padding.top + m_padding.bottom;
        m_isDirty = false;
        return;
    }

    // フォント高さの取得
    int fontHeight = (m_fontHandle == -1) ? 16 : GetFontSizeToHandle(m_fontHandle);
    if (fontHeight <= 0) fontHeight = 16;

    // 行（段落）処理用関数
    auto processParagraph = [&](const std::string& lineStr) {
        if (lineStr.empty()) {
            m_wrappedLines.push_back("");
            return;
        }

        const char* pStart = lineStr.c_str();
        size_t strLenBytes = lineStr.length();
        size_t currentLineStartByte = 0;
        size_t currentByteIdx = 0;

        while (currentByteIdx < strLenBytes) {
            size_t charLen = GetNextCharByteLen(pStart + currentByteIdx);
            if (charLen == 0) break;

            size_t nextByteIdx = currentByteIdx + charLen;

            // 先頭から nextByteIdx バイト分を試算
            int testWidth = GetDrawStringWidthToHandle(
                pStart + currentLineStartByte,
                static_cast<int>(nextByteIdx - currentLineStartByte),
                m_fontHandle
            );

            if (testWidth > m_maxTextWidth && currentByteIdx > currentLineStartByte) {
                // 最大幅を超えたため、currentByteIdx の位置で改行
                std::string lineSegment = lineStr.substr(currentLineStartByte, currentByteIdx - currentLineStartByte);
                m_wrappedLines.push_back(lineSegment);

                int lineW = GetDrawStringWidthToHandle(lineSegment.c_str(), static_cast<int>(lineSegment.length()), m_fontHandle);
                if (lineW > m_calculatedTextWidth) {
                    m_calculatedTextWidth = lineW;
                }

                currentLineStartByte = currentByteIdx;
            } else {
                currentByteIdx = nextByteIdx;
            }
        }

        // 行の末尾部分を追加
        if (currentLineStartByte < strLenBytes) {
            std::string lineSegment = lineStr.substr(currentLineStartByte);
            m_wrappedLines.push_back(lineSegment);

            int lineW = GetDrawStringWidthToHandle(lineSegment.c_str(), static_cast<int>(lineSegment.length()), m_fontHandle);
            if (lineW > m_calculatedTextWidth) {
                m_calculatedTextWidth = lineW;
            }
        }
    };

    // 改行コード (\r\n, \n, \r) で分解して処理
    size_t len = m_rawText.length();
    size_t i = 0;
    size_t pStartIdx = 0;

    while (i < len) {
        if (m_rawText[i] == '\r') {
            std::string sub = m_rawText.substr(pStartIdx, i - pStartIdx);
            processParagraph(sub);
            if (i + 1 < len && m_rawText[i + 1] == '\n') {
                i++;
            }
            pStartIdx = i + 1;
        } else if (m_rawText[i] == '\n') {
            std::string sub = m_rawText.substr(pStartIdx, i - pStartIdx);
            processParagraph(sub);
            pStartIdx = i + 1;
        }
        i++;
    }
    if (pStartIdx <= len) {
        std::string sub = m_rawText.substr(pStartIdx);
        processParagraph(sub);
    }

    // 全体高さの算出
    size_t lineCount = m_wrappedLines.size();
    if (lineCount > 0) {
        m_calculatedTextHeight = static_cast<int>(lineCount) * fontHeight + static_cast<int>(lineCount - 1) * m_lineSpacing;
    } else {
        m_calculatedTextHeight = 0;
    }

    // ウィンドウ全体の幅・高さ（最小幅・高さとして9スライス四隅の合計サイズを考慮）
    int minW = m_nineSliceMargin.left + m_nineSliceMargin.right;
    int minH = m_nineSliceMargin.top + m_nineSliceMargin.bottom;

    int autoW = (std::max)(m_calculatedTextWidth + m_padding.left + m_padding.right, minW);
    int autoH = (std::max)(m_calculatedTextHeight + m_padding.top + m_padding.bottom, minH);

    // タイトルが設定されている場合はヘッダー分の高さを追加
    if (!m_title.empty()) {
        autoH += fontHeight + m_lineSpacing + 8;
    }

    m_windowWidth = (m_fixedWidth > 0) ? m_fixedWidth : autoW;
    m_windowHeight = (m_fixedHeight > 0) ? m_fixedHeight : autoH;

    m_isDirty = false;
}

void UITextWindow::Draw9Slice(int x, int y, int width, int height) const {
    if (m_bgGraphHandle < 0) return;

    int srcW = 0, srcH = 0;
    if (GetGraphSize(m_bgGraphHandle, &srcW, &srcH) != 0) return;

    int mL = m_nineSliceMargin.left;
    int mT = m_nineSliceMargin.top;
    int mR = m_nineSliceMargin.right;
    int mB = m_nineSliceMargin.bottom;

    int srcCenterW = srcW - mL - mR;
    int srcCenterH = srcH - mT - mB;

    int destCenterW = width - mL - mR;
    int destCenterH = height - mT - mB;

    if (destCenterW < 0) destCenterW = 0;
    if (destCenterH < 0) destCenterH = 0;

    // 1. Top-Left (角)
    if (mL > 0 && mT > 0) {
        DrawRectExtendGraph(x, y, x + mL, y + mT, 0, 0, mL, mT, m_bgGraphHandle, m_transFlag);
    }
    // 2. Top (上辺)
    if (srcCenterW > 0 && mT > 0 && destCenterW > 0) {
        DrawRectExtendGraph(x + mL, y, x + mL + destCenterW, y + mT, mL, 0, srcCenterW, mT, m_bgGraphHandle, m_transFlag);
    }
    // 3. Top-Right (角)
    if (mR > 0 && mT > 0) {
        DrawRectExtendGraph(x + width - mR, y, x + width, y + mT, srcW - mR, 0, mR, mT, m_bgGraphHandle, m_transFlag);
    }

    // 4. Left (左辺)
    if (mL > 0 && srcCenterH > 0 && destCenterH > 0) {
        DrawRectExtendGraph(x, y + mT, x + mL, y + mT + destCenterH, 0, mT, mL, srcCenterH, m_bgGraphHandle, m_transFlag);
    }
    // 5. Center (中央)
    if (srcCenterW > 0 && srcCenterH > 0 && destCenterW > 0 && destCenterH > 0) {
        DrawRectExtendGraph(x + mL, y + mT, x + mL + destCenterW, y + mT + destCenterH, mL, mT, srcCenterW, srcCenterH, m_bgGraphHandle, m_transFlag);
    }
    // 6. Right (右辺)
    if (mR > 0 && srcCenterH > 0 && destCenterH > 0) {
        DrawRectExtendGraph(x + width - mR, y + mT, x + width, y + mT + destCenterH, srcW - mR, mT, mR, srcCenterH, m_bgGraphHandle, m_transFlag);
    }

    // 7. Bottom-Left (角)
    if (mL > 0 && mB > 0) {
        DrawRectExtendGraph(x, y + height - mB, x + mL, y + height, 0, srcH - mB, mL, mB, m_bgGraphHandle, m_transFlag);
    }
    // 8. Bottom (下辺)
    if (srcCenterW > 0 && mB > 0 && destCenterW > 0) {
        DrawRectExtendGraph(x + mL, y + height - mB, x + mL + destCenterW, y + height, mL, srcH - mB, srcCenterW, mB, m_bgGraphHandle, m_transFlag);
    }
    // 9. Bottom-Right (角)
    if (mR > 0 && mB > 0) {
        DrawRectExtendGraph(x + width - mR, y + height - mB, x + width, y + height, srcW - mR, srcH - mB, mR, mB, m_bgGraphHandle, m_transFlag);
    }
}

void UITextWindow::Draw(int x, int y) const {
    if (m_isDirty) {
        const_cast<UITextWindow*>(this)->RecalculateLayout();
    }

    // 1. 背景の描画（9スライス画像があれば画像を描画、なければ半透明ボックスを描画）
    if (m_bgGraphHandle >= 0) {
        Draw9Slice(x, y, m_windowWidth, m_windowHeight);
    } else if (m_drawBoxBg) {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, m_boxAlpha);
        DrawBox(x, y, x + m_windowWidth, y + m_windowHeight, m_boxBgColor, TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawBox(x, y, x + m_windowWidth, y + m_windowHeight, m_boxBorderColor, FALSE);
    }

    int fontHeight = (m_fontHandle == -1) ? 16 : GetFontSizeToHandle(m_fontHandle);
    if (fontHeight <= 0) fontHeight = 16;

    int currentY = y + m_padding.top;
    int drawX = x + m_padding.left;

    // 2. タイトル文字列の描画（設定されている場合）
    if (!m_title.empty()) {
        DrawStringToHandle(drawX, currentY, m_title.c_str(), m_titleColor, m_fontHandle);
        currentY += fontHeight + 6;
        DrawLine(drawX, currentY, x + m_windowWidth - m_padding.right, currentY, m_boxBorderColor);
        currentY += m_lineSpacing + 6;
    }

    // 3. 本文テキストの描画
    for (const auto& line : m_wrappedLines) {
        if (!line.empty()) {
            DrawStringToHandle(drawX, currentY, line.c_str(), m_textColor, m_fontHandle);
        }
        currentY += fontHeight + m_lineSpacing;
    }
}

} // namespace UI

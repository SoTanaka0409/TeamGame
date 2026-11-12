#include "FontManager.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <algorithm>

namespace {
    /**
     * @brief UTF-8文字のバイト長を安全に判定する（バッファ境界もチェック）
     */
    size_t GetUtf8CharByteLen(const char* p, size_t remainingBytes) {
        if (!p || remainingBytes == 0 || *p == '\0') return 0;
        unsigned char c = static_cast<unsigned char>(*p);
        size_t len = 1;
        if (c < 0x80) len = 1;
        else if ((c & 0xE0) == 0xC0) len = 2;
        else if ((c & 0xF0) == 0xE0) len = 3;
        else if ((c & 0xF8) == 0xF0) len = 4;
        else len = 1;
        return (len <= remainingBytes) ? len : remainingBytes;
    }

    bool IsAsciiWordChar(unsigned char c) {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
    }

    bool IsLineStartProhibited(const std::string& ch) {
        static const char* prohibited[] = {
            "、", "。", "，", "．", "！", "？", "：", "；", "）", "］", "」", "』", "】", "〉", "》",
            ",", ".", "!", "?", ":", ";", ")", "]", "}"
        };
        for (const char* p : prohibited) {
            if (ch == p) return true;
        }
        return false;
    }
}

FontManager& FontManager::GetInstance() {
    static FontManager instance;
    return instance;
}

FontManager::~FontManager() {
    Clear();
}

std::string FontManager::MakeKey(int size, int thick, const std::string& name, int type) const {
    return name + "_" + std::to_string(size) + "_" + std::to_string(thick) + "_" + std::to_string(type);
}

int FontManager::GetFont(int fontSize, int thick, const std::string& fontName, int fontType) {
    std::string key = MakeKey(fontSize, thick, fontName, fontType);
    auto it = m_fontMap.find(key);
    if (it != m_fontMap.end()) {
        return it->second;
    }

    const char* namePtr = fontName.empty() ? nullptr : fontName.c_str();
    int handle = CreateFontToHandle(namePtr, fontSize, thick, fontType);
    if (handle != -1) {
        m_fontMap[key] = handle;
    }
    return handle;
}

void FontManager::DrawString(int x, int y, const char* str, unsigned int color, int fontSize, int thick, const std::string& fontName) {
    if (!str) return;
    int handle = GetFont(fontSize, thick, fontName);
    if (handle != -1) {
        DrawStringToHandle(x, y, str, color, handle);
    } else {
        ::DrawString(x, y, str, color);
    }
}

void FontManager::DrawFormatString(int x, int y, unsigned int color, int fontSize, const char* format, ...) {
    if (!format) return;
    int handle = GetFont(fontSize);

    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (handle != -1) {
        DrawStringToHandle(x, y, buffer, color, handle);
    } else {
        ::DrawString(x, y, buffer, color);
    }
}

void FontManager::DrawStringCenter(int centerX, int y, const char* str, unsigned int color, int fontSize, int thick, const std::string& fontName) {
    if (!str) return;
    int width = GetStringWidth(str, fontSize, thick, fontName);
    int drawX = centerX - width / 2;
    DrawString(drawX, y, str, color, fontSize, thick, fontName);
}

void FontManager::DrawStringRight(int rightX, int y, const char* str, unsigned int color, int fontSize, int thick, const std::string& fontName) {
    if (!str) return;
    int width = GetStringWidth(str, fontSize, thick, fontName);
    int drawX = rightX - width;
    DrawString(drawX, y, str, color, fontSize, thick, fontName);
}

std::vector<std::string> FontManager::WrapText(const char* str, int maxWidth, int fontSize, int thick, const std::string& fontName) {
    std::vector<std::string> lines;
    if (!str || *str == '\0') return lines;

    if (maxWidth <= 0) {
        lines.emplace_back(str);
        return lines;
    }

    std::string raw(str);
    int handle = GetFont(fontSize, thick, fontName);

    auto measure = [&](const char* s, int byteLen) -> int {
        if (byteLen <= 0) return 0;
        if (handle != -1) return GetDrawStringWidthToHandle(s, byteLen, handle);
        return GetDrawStringWidth(s, byteLen);
    };

    auto processParagraph = [&](const std::string& para) {
        if (para.empty()) {
            lines.emplace_back("");
            return;
        }

        // UTF-8の文字単位のバイトオフセット配列を作成
        struct CharPos { size_t start; size_t len; };
        std::vector<CharPos> chars;
        const char* p = para.c_str();
        size_t totalBytes = para.length();
        size_t idx = 0;
        while (idx < totalBytes) {
            size_t clen = GetUtf8CharByteLen(p + idx, totalBytes - idx);
            if (clen == 0) break;
            chars.push_back({ idx, clen });
            idx += clen;
        }

        size_t lineStartChar = 0;
        size_t curChar = 0;

        while (curChar < chars.size()) {
            size_t startByte = chars[lineStartChar].start;
            size_t endByte = chars[curChar].start + chars[curChar].len;
            int w = measure(p + startByte, static_cast<int>(endByte - startByte));

            if (w > maxWidth && curChar > lineStartChar) {
                size_t breakChar = curChar;

                // 英単語途中の不自然な切断を回避（手前にスペースや記号境界があればそこで折り返す）
                std::string curChStr = para.substr(chars[curChar].start, chars[curChar].len);
                if (chars[curChar].len == 1 && IsAsciiWordChar(static_cast<unsigned char>(p[chars[curChar].start]))) {
                    size_t search = curChar;
                    while (search > lineStartChar) {
                        size_t prev = search - 1;
                        if (chars[prev].len == 1 && p[chars[prev].start] == ' ') {
                            if (prev - lineStartChar >= (curChar - lineStartChar) / 2) {
                                breakChar = prev + 1;
                            }
                            break;
                        }
                        if (chars[prev].len > 1 || !IsAsciiWordChar(static_cast<unsigned char>(p[chars[prev].start]))) {
                            if (prev > lineStartChar && (prev - lineStartChar) >= (curChar - lineStartChar) / 2) {
                                breakChar = search;
                            }
                            break;
                        }
                        search--;
                    }
                } else if (IsLineStartProhibited(curChStr) && breakChar > lineStartChar + 1) {
                    // 行頭禁則文字: 手前の1文字を次行へ送り、句読点だけが行頭に来るのを避ける
                    breakChar = curChar - 1;
                }

                if (breakChar <= lineStartChar) breakChar = curChar;

                size_t segEndByte = chars[breakChar - 1].start + chars[breakChar - 1].len;
                std::string seg = para.substr(startByte, segEndByte - startByte);
                while (!seg.empty() && seg.back() == ' ') {
                    seg.pop_back();
                }
                lines.push_back(seg);

                lineStartChar = breakChar;
                while (lineStartChar < chars.size() && chars[lineStartChar].len == 1 && p[chars[lineStartChar].start] == ' ') {
                    lineStartChar++;
                }
                curChar = lineStartChar;
            } else {
                curChar++;
            }
        }

        if (lineStartChar < chars.size()) {
            size_t startByte = chars[lineStartChar].start;
            lines.push_back(para.substr(startByte));
        }
    };

    size_t len = raw.length();
    size_t i = 0;
    size_t pStart = 0;
    while (i < len) {
        if (raw[i] == '\r') {
            processParagraph(raw.substr(pStart, i - pStart));
            if (i + 1 < len && raw[i + 1] == '\n') i++;
            pStart = i + 1;
        } else if (raw[i] == '\n') {
            processParagraph(raw.substr(pStart, i - pStart));
            pStart = i + 1;
        }
        i++;
    }
    if (pStart <= len) {
        std::string tail = raw.substr(pStart);
        if (!tail.empty() || lines.empty()) {
            processParagraph(tail);
        }
    }

    return lines;
}

int FontManager::DrawStringWrapped(int x, int y, int maxWidth, const char* str, unsigned int color, int fontSize, int lineSpacing, int thick, const std::string& fontName) {
    if (!str || *str == '\0') return 0;
    auto lines = WrapText(str, maxWidth, fontSize, thick, fontName);
    if (lines.empty()) return 0;

    int lineH = GetFontHeight(fontSize, thick, fontName);
    int curY = y;
    for (size_t i = 0; i < lines.size(); ++i) {
        DrawString(x, curY, lines[i].c_str(), color, fontSize, thick, fontName);
        curY += lineH + (i + 1 < lines.size() ? lineSpacing : 0);
    }
    return curY - y;
}

int FontManager::DrawStringWrappedCenter(int centerX, int y, int maxWidth, const char* str, unsigned int color, int fontSize, int lineSpacing, int thick, const std::string& fontName) {
    if (!str || *str == '\0') return 0;
    auto lines = WrapText(str, maxWidth, fontSize, thick, fontName);
    if (lines.empty()) return 0;

    int lineH = GetFontHeight(fontSize, thick, fontName);
    int curY = y;
    for (size_t i = 0; i < lines.size(); ++i) {
        DrawStringCenter(centerX, curY, lines[i].c_str(), color, fontSize, thick, fontName);
        curY += lineH + (i + 1 < lines.size() ? lineSpacing : 0);
    }
    return curY - y;
}

int FontManager::MeasureWrappedTextHeight(const char* str, int maxWidth, int fontSize, int lineSpacing, int thick, const std::string& fontName) {
    if (!str || *str == '\0') return 0;
    auto lines = WrapText(str, maxWidth, fontSize, thick, fontName);
    if (lines.empty()) return 0;
    int lineH = GetFontHeight(fontSize, thick, fontName);
    return static_cast<int>(lines.size()) * lineH + static_cast<int>(lines.size() - 1) * lineSpacing;
}

int FontManager::GetFittingFontSize(const char* str, int maxWidth, int preferredSize, int minSize, int thick, const std::string& fontName) {
    if (!str || *str == '\0' || maxWidth <= 0) return preferredSize;
    for (int sz = preferredSize; sz >= minSize; --sz) {
        if (GetStringWidth(str, sz, thick, fontName) <= maxWidth) {
            return sz;
        }
    }
    return minSize;
}

int FontManager::GetStringWidth(const char* str, int fontSize, int thick, const std::string& fontName) {
    if (!str) return 0;
    int handle = GetFont(fontSize, thick, fontName);
    if (handle != -1) {
        return GetDrawStringWidthToHandle(str, static_cast<int>(strlen(str)), handle);
    }
    return GetDrawStringWidth(str, static_cast<int>(strlen(str)));
}

int FontManager::GetFontHeight(int fontSize, int thick, const std::string& fontName) {
    int handle = GetFont(fontSize, thick, fontName);
    if (handle != -1) {
        int h = GetFontSizeToHandle(handle);
        if (h > 0) return h;
    }
    return fontSize;
}

void FontManager::Clear() {
    for (auto& pair : m_fontMap) {
        if (pair.second != -1) {
            DeleteFontToHandle(pair.second);
        }
    }
    m_fontMap.clear();
}

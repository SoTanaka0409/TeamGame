#include "FontManager.h"
#include <cstdarg>
#include <cstdio>

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

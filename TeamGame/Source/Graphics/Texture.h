#pragma once
#include <string>
#include "DxLib.h"

class Texture
{
private:
    int handle;
    std::string filePath;
    int width;
    int height;

public:
    Texture(const std::string& path) : filePath(path)
    {
        handle = LoadGraph(path.c_str());
        if (handle != -1) {
            GetGraphSize(handle, &width, &height);
        } else {
            width = 0;
            height = 0;
        }
    }

    ~Texture()
    {
        if (handle != -1) {
            DeleteGraph(handle);
            handle = -1;
        }
    }

    // コピー禁止（二重解放を防ぐため）
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // 基本的な描画
    void Draw(int x, int y, bool trans = true) const
    {
        if (handle != -1) {
            DrawGraph(x, y, handle, trans ? TRUE : FALSE);
        }
    }

    // 回転・拡縮描画
    void DrawRota(int x, int y, double extRate, double angle, bool trans = true, bool turn = false) const
    {
        if (handle != -1) {
            DrawRotaGraph(x, y, extRate, angle, handle, trans ? TRUE : FALSE, turn ? TRUE : FALSE);
        }
    }

    int GetHandle() const { return handle; }
    int GetWidth() const { return width; }
    int GetHeight() const { return height; }
    std::string GetFilePath() const { return filePath; }
};

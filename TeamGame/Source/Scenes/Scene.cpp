#include "Scene.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include "DebugManager.h"
#include "DxLib.h"
#include <vector>
#include <unordered_map>

namespace
{
    bool SceneFileExistsUtf8(const std::string& utf8Path)
    {
        if (utf8Path.empty()) return false;
        int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8Path.c_str(), -1, nullptr, 0);
        if (wlen <= 0) return false;
        std::wstring wpath(static_cast<size_t>(wlen), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8Path.c_str(), -1, &wpath[0], wlen);
        DWORD attr = GetFileAttributesW(wpath.c_str());
        return (attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY);
    }
}

Scene::Scene()
{
    objectManager = new ObjectManager();
    colliderManager = new ColliderManager();
    effectManager = new EffectManager();
}

Scene::~Scene()
{
    if (colliderManager)
    {
        colliderManager->Clear();
    }
    delete objectManager;
    delete colliderManager;
    delete effectManager;
}

void Scene::Update()
{
    objectManager->Update();
    colliderManager->CheckAllCollisions();
    objectManager->RemoveDestroyedObjects();
    effectManager->Update();
}

void Scene::Draw()
{
    if (effectManager)
    {
        effectManager->DrawGround();
    }
    objectManager->Draw();
    effectManager->Draw();
    if (DebugManager::GetInstance().IsDebugMode() && colliderManager)
    {
        colliderManager->DrawDebug();
    }
    if (effectManager)
    {
        effectManager->DrawForeground();
    }
}

int Scene::LoadGraphWithSearch(const std::string& fileName)
{
    if (fileName.empty()) return -1;

    // 既に直接開けるか確認
    if (SceneFileExistsUtf8(fileName))
    {
        int handle = LoadGraph(fileName.c_str());
        if (handle != -1) return handle;
    }

    // 複数の相対ディレクトリ候補を順番に検索
    const std::vector<std::string> baseDirs = {
        "Resource/",
        "TeamGame/Resource/",
        "TeamGame/TeamGame/Resource/",
        "../Resource/",
        "../../Resource/",
        "../TeamGame/Resource/",
        "../../TeamGame/Resource/",
        "../../TeamGame/TeamGame/Resource/",
        "Resource/Images/",
        "TeamGame/Resource/Images/",
        "../Resource/Images/",
        ""
    };

    for (const auto& dir : baseDirs)
    {
        std::string fullPath = dir + fileName;
        if (!SceneFileExistsUtf8(fullPath)) continue;
        int handle = LoadGraph(fullPath.c_str());
        if (handle != -1) return handle;
    }

    return -1;
}

void Scene::DrawFullScreenGraph(int handle, bool trans)
{
    if (handle != -1)
    {
        DrawExtendGraph(0, 0, 1920, 1080, handle, trans ? TRUE : FALSE);
    }
}

// 2D画像配列とパス-インデックス対応マップの静的実体
static std::vector<int> s_2DImageArray;
static std::unordered_map<std::string, int> s_pathTo2DIndex;

int Scene::Load2DImage(const std::string& fileName)
{
    if (fileName.empty()) return -1;

    // 既に読み込み済みの場合は配列番号（インデックス）をそのまま返す
    auto it = s_pathTo2DIndex.find(fileName);
    if (it != s_pathTo2DIndex.end())
    {
        return it->second;
    }

    // 新規ロード
    int handle = LoadGraphWithSearch(fileName);
    if (handle == -1)
    {
        s_pathTo2DIndex[fileName] = -1; // 失敗もキャッシュして再探索を防止
        return -1; // 読み込み失敗
    }

    // 配列の末尾に追加して番号（インデックス）を確定
    int newIndex = static_cast<int>(s_2DImageArray.size());
    s_2DImageArray.push_back(handle);
    s_pathTo2DIndex[fileName] = newIndex;

    return newIndex;
}

int Scene::Get2DImage(int imageIndex)
{
    if (imageIndex < 0 || imageIndex >= static_cast<int>(s_2DImageArray.size()))
    {
        return -1;
    }
    return s_2DImageArray[imageIndex];
}

void Scene::Draw2DImage(int imageIndex, int x, int y, bool trans)
{
    int handle = Get2DImage(imageIndex);
    if (handle != -1)
    {
        DrawGraph(x, y, handle, trans ? TRUE : FALSE);
    }
}

void Scene::DrawFullScreen2DImage(int imageIndex, bool trans)
{
    int handle = Get2DImage(imageIndex);
    if (handle != -1)
    {
        DrawExtendGraph(0, 0, 1920, 1080, handle, trans ? TRUE : FALSE);
    }
}

void Scene::Clear2DImages()
{
    for (int h : s_2DImageArray)
    {
        if (h != -1)
        {
            DeleteGraph(h);
        }
    }
    s_2DImageArray.clear();
    s_pathTo2DIndex.clear();
}



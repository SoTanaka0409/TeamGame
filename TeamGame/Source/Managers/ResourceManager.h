#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include "DxLib.h"
#include "../Graphics/Texture.h"
#include "../Graphics/Model.h"

class ResourceManager
{
private:
    // パスをキーにしてテクスチャをキャッシュ
    std::unordered_map<std::string, std::shared_ptr<Texture>> textures;
    
    // 3Dモデルの元データ（ベースモデル）のハンドルをキャッシュ
    std::unordered_map<std::string, int> baseModels;

    ResourceManager() {} // シングルトン

public:
    static ResourceManager& GetInstance()
    {
        static ResourceManager instance;
        return instance;
    }

    // -----------------------------------------------------
    // 2D画像の取得（一度読み込んだら使い回す）
    // -----------------------------------------------------
    std::shared_ptr<Texture> GetTexture(const std::string& path)
    {
        // まだ読み込まれていなければロードする
        if (textures.find(path) == textures.end()) {
            textures[path] = std::make_shared<Texture>(path);
        }
        return textures[path];
    }

    // -----------------------------------------------------
    // 3Dモデルの取得（ベースモデルから複製したものを返す）
    // -----------------------------------------------------
    std::shared_ptr<Model> GetModel(const std::string& path)
    {
        // ベースモデルがまだ読み込まれていなければロードする
        if (baseModels.find(path) == baseModels.end()) {
            int rawHandle = MV1LoadModel(path.c_str());
            baseModels[path] = rawHandle;
        }
        
        // 複製された独立したモデルインスタンスを生成して返す（メモリ節約・高速化）
        return std::make_shared<Model>(baseModels[path]);
    }

    // -----------------------------------------------------
    // 全リソースの解放（シーン切り替え時などに呼ぶ）
    // -----------------------------------------------------
    void Clear()
    {
        // shared_ptr なので map をクリアするだけで Texture のデストラクタが呼ばれ、画像が解放される
        textures.clear();
        
        // 3Dのベースモデルは手動で削除
        for (auto& pair : baseModels) {
            if (pair.second != -1) {
                MV1DeleteModel(pair.second);
            }
        }
        baseModels.clear();
    }
};

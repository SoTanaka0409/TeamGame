#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include "DxLib.h"
#include "../Graphics/Texture.h"
#include "../Graphics/Model.h"

/**
 * @brief リソース（画像、3Dモデルなど）を管理するシングルトンクラス
 * @details リソースの重複読み込みを防ぎ、メモリ効率の良い取得・管理を行う。
 */
class ResourceManager
{
private:
    // パスをキーにしてテクスチャをキャッシュ
    std::unordered_map<std::string, std::shared_ptr<Texture>> textures;
    
    // 3Dモデルの元データ（ベースモデル）のハンドルをキャッシュ
    std::unordered_map<std::string, int> baseModels;

    /**
     * @brief コンストラクタ
     */
    ResourceManager() {} // シングルトン

public:
    /**
     * @brief インスタンスの取得
     * @return ResourceManager& リソースマネージャのシングルトンインスタンス
     */
    static ResourceManager& GetInstance()
    {
        static ResourceManager instance;
        return instance;
    }

    /**
     * @brief 2D画像の取得（一度読み込んだら使い回す）
     * @param path 画像ファイルのパス
     * @return std::shared_ptr<Texture> 画像のテクスチャへのポインタ
     * @details まだ読み込まれていない場合はロードし、既に読み込まれている場合はキャッシュを返す。
     */
    std::shared_ptr<Texture> GetTexture(const std::string& path)
    {
        // まだ読み込まれていなければロードする
        if (textures.find(path) == textures.end()) {
            textures[path] = std::make_shared<Texture>(path);
        }
        return textures[path];
    }

    /**
     * @brief 3Dモデルの取得（ベースモデルから複製したものを返す）
     * @param path 3Dモデルファイルのパス
     * @return std::shared_ptr<Model> 複製された独立したモデルインスタンス
     * @details ベースモデルが読み込まれていなければロードし、そこからインスタンスを生成して返す（メモリ節約・高速化）。
     */
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

    /**
     * @brief 全リソースの解放（シーン切り替え時などに呼ぶ）
     * @details キャッシュされているテクスチャとベースモデルをすべて破棄し、メモリを解放する。
     */
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

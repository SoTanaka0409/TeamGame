#pragma once
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief 武器のデータを表す構造体
 * @details 武器ごとの性能（射程、拡散角度、弾速など）やモデル、UIのパスなどを保持する。
 */
struct WeaponData
{
    std::string name;
    float range;          // 射程距離
    float spreadAngle;    // 弾のブレの大きさ(度)
    float bulletRadius;   // 弾の当たり判定サイズ
    float bulletSpeed;    // 弾の速度
    int maxAmmo;          // 最大装弾数
    int fireInterval;     // 発射間隔(フレーム数)
    int reloadTime;       // リロード時間(フレーム数)
    int damage = 1;       // 1発あたりのダメージ
    int pelletCount = 1;  // 1回の射撃で発射する弾丸数
    std::string modelPath; // 3Dモデルのパス
    std::string uiImagePath; // UI用画像のパス
    int uiImageHandle = -1;  // 画像のハンドル
};

/**
 * @brief 武器のデータを管理するシングルトンクラス
 * @details CSVファイルからの武器データの読み込み、取得、およびリソースのクリーンアップを行う。
 */
class WeaponManager
{
private:
    std::unordered_map<std::string, WeaponData> weaponDatabase;

    /**
     * @brief コンストラクタ
     */
    WeaponManager() {}

    /**
     * @brief デストラクタ
     */
    ~WeaponManager() {}

public:
    /**
     * @brief インスタンスの取得
     * @return WeaponManager& 武器マネージャのシングルトンインスタンス
     */
    static WeaponManager& GetInstance()
    {
        static WeaponManager instance;
        return instance;
    }

    WeaponManager(const WeaponManager&) = delete;
    WeaponManager& operator=(const WeaponManager&) = delete;

    /**
     * @brief CSVから武器データを読み込む
     * @param filePath CSVファイルのパス
     * @details 指定したCSVファイルを解析し、WeaponDataとしてデータベースに登録する。
     */
    void LoadFromCSV(const std::string& filePath);

    /**
     * @brief 武器データの取得
     * @param weaponName 武器の名前
     * @return const WeaponData* 該当する武器データのポインタ。見つからない場合はnullptrを返す。
     * @details 名前を指定して武器データベースから武器の性能データを取得する。
     */
    const WeaponData* GetWeaponData(const std::string& weaponName) const;

    /**
     * @brief リソースの解放
     * @details 読み込んだUI画像などのリソースを解放し、データベースをクリアする。
     */
    void Cleanup();
};

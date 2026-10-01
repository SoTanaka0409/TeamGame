#pragma once
#include <string>

/**
 * @brief デバッグ情報を管理するシングルトンクラス
 * @details デバッグモードの切り替え、デバッグ情報のUI表示、デバッグ用キー入力の処理を行う。
 */
class DebugManager
{
private:
    bool m_isDebugMode = false;

    /**
     * @brief コンストラクタ
     * @details インスタンスの直接生成を防ぐためにプライベート化。
     */
    DebugManager() = default;

public:
    /**
     * @brief インスタンスの取得
     * @return DebugManager& デバッグマネージャの唯一のインスタンス
     * @details シングルトンインスタンスを返す。
     */
    static DebugManager& GetInstance()
    {
        static DebugManager instance;
        return instance;
    }

    /**
     * @brief デストラクタ
     */
    ~DebugManager() = default;

    // コピー・代入の禁止
    DebugManager(const DebugManager&) = delete;
    DebugManager& operator=(const DebugManager&) = delete;

    /**
     * @brief デバッグモード状態の取得
     * @return bool デバッグモードが有効な場合はtrue、そうでない場合はfalse
     * @details 現在のデバッグモードの有効・無効状態を返す。
     */
    bool IsDebugMode() const { return m_isDebugMode; }

    /**
     * @brief デバッグモードの設定
     * @param enable 有効にする場合はtrue、無効にする場合はfalse
     * @details デバッグモードの有効・無効を強制的に設定する。
     */
    void SetDebugMode(bool enable) { m_isDebugMode = enable; }

    /**
     * @brief デバッグモードのトグル
     * @details デバッグモードの有効・無効状態を切り替える。
     */
    void ToggleDebugMode() { m_isDebugMode = !m_isDebugMode; }

    /**
     * @brief キー入力更新（Tab, F1キーなどのデバッグ操作監視）
     * @details 毎フレーム呼ばれ、デバッグに関連する入力処理を行う。
     */
    void Update();

    /**
     * @brief デバッグ情報および操作ガイドのUI描画
     * @param stageName 現在のステージ名
     * @param playerX プレイヤーのX座標
     * @param playerY プレイヤーのY座標
     * @param enemyCount 敵の数
     * @details 画面上にデバッグテキストを表示する。
     */
    void DrawDebugOverlay(const std::string& stageName, float playerX, float playerY, int enemyCount);
};

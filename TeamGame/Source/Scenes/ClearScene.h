#pragma once
#include "Scene.h"
#include <string>

/**
 * @brief ゲームクリア時のリザルト情報を保持する構造体
 * @details クリアタイム、倒した敵の数、スコア、ランクなどの情報を格納します。
 */
struct ClearStats
{
    float clearTimeSec = 0.0f;       ///< クリアタイム（秒）
    int defeatedEnemies = 0;         ///< 倒した敵の数
    int totalEnemies = 0;            ///< 全敵の数（スポーン総数など）
    int rankScore = 0;               ///< 算出されたスコア
    std::string rankName = "S";      ///< 評価ランク（"S", "A", "B" など）
};

/**
 * @brief ゲームクリア画面を管理するシーンクラス
 * @details ゲームクリア時の演出、リザルト情報（ClearStats）の表示、タイトルへの遷移などを制御します。
 */
class ClearScene : public Scene
{
  private:
    ClearStats stats;                ///< 表示するクリア情報
    int menuCursor = 0;              ///< メニューのカーソル位置
    float animTimer = 0.0f;          ///< 演出用のアニメーションタイマー

    bool prevUp = false;             ///< 前フレームでの上入力状態
    bool prevDown = false;           ///< 前フレームでの下入力状態
    bool prevEnter = false;          ///< 前フレームでの決定入力状態

  public:
    /**
     * @brief コンストラクタ
     * @param stats クリア結果として表示する統計情報
     * @details シーン生成時にクリア情報を受け取ります。
     */
    ClearScene(const ClearStats& stats = ClearStats());

    /**
     * @brief デストラクタ
     * @details リソースの解放を行います。
     */
    ~ClearScene() override;

    /**
     * @brief 初期化処理
     * @details アニメーションタイマー等の初期化を行います。
     */
    void Init() override;

    /**
     * @brief 更新処理
     * @details 毎フレーム呼ばれ、メニューの入力処理やアニメーションの更新を行います。
     */
    void Update() override;

    /**
     * @brief 描画処理
     * @details 毎フレーム呼ばれ、クリアテキスト、リザルトスコア、メニュー項目を描画します。
     */
    void Draw() override;
};

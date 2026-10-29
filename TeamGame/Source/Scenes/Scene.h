#pragma once
#include "ColliderManager.h"
#include "ObjectManager.h"
#include "EffectManager.h"

/**
 * @brief 全てのシーンの基底クラス
 * @details 各シーン（タイトル、ゲーム本編、リザルト等）に共通するオブジェクト管理、当たり判定管理、エフェクト管理の機能を提供するクラスです。
 */
class Scene
{
  protected:
    ObjectManager *objectManager;      ///< オブジェクトを管理するマネージャー
    ColliderManager *colliderManager;  ///< 当たり判定を管理するマネージャー
    EffectManager *effectManager;      ///< エフェクトを管理するマネージャー

  public:
    /**
     * @brief コンストラクタ
     * @details 各種マネージャーのインスタンスを生成して初期化します。
     */
    Scene();

    /**
     * @brief デストラクタ
     * @details メモリリークを防ぐため、確保した各種マネージャーのメモリを解放します。
     */
    virtual ~Scene();

    /**
     * @brief シーンの初期化処理
     * @details シーン開始時に一度だけ呼ばれる初期化処理を記述します。派生クラスでオーバーライドして使用します。
     */
    virtual void Init()
    {
    }

    /**
     * @brief シーンの更新処理
     * @details 毎フレーム呼ばれる更新処理です。オブジェクトやエフェクトの更新、当たり判定のチェックを行います。
     */
    virtual void Update();

    /**
     * @brief シーンの描画処理
     * @details 毎フレーム呼ばれる描画処理です。管理しているオブジェクトやエフェクトを描画します。
     */
    virtual void Draw();

    /**
     * @brief シーンの終了処理
     * @details シーン終了時に一度だけ呼ばれる終了処理を記述します。派生クラスでオーバーライドして使用します。
     */
    virtual void Finalize()
    {
    }

    /**
     * @brief ObjectManagerを取得する
     * @return オブジェクトマネージャーのポインタ
     * @details シーンに登録されているオブジェクトを管理するマネージャーを取得します。
     */
    ObjectManager *GetObjectManager() const
    {
        return objectManager;
    }

    /**
     * @brief ColliderManagerを取得する
     * @return 当たり判定マネージャーのポインタ
     * @details シーン内の当たり判定を管理するマネージャーを取得します。
     */
    ColliderManager *GetColliderManager() const
    {
        return colliderManager;
    }

    /**
     * @brief EffectManagerを取得する
     * @return エフェクトマネージャーのポインタ
     * @details シーン内のエフェクトを管理するマネージャーを取得します。
     */
    EffectManager *GetEffectManager() const
    {
        return effectManager;
    }

    /**
     * @brief 現在のステージを取得する
     * @return ステージの定数ポインタ（ステージがない場合はnullptr）
     * @details ステージ情報を持つシーン（GameSceneなど）でオーバーライドし、ステージ情報を返します。
     */
    virtual const class Stage* GetStage() const
    {
        return nullptr;
    }

    /**
     * @brief 画像ファイルを複数の候補パスから検索して読み込む共通ヘルパー関数
     * @param fileName 画像ファイル名または相対パス（例: "title_bg.png", "Resource/title_bg.png" など）
     * @return int 読み込んだDxLibのグラフィックハンドル（読み込み失敗時やファイルが無い場合は -1）
     * @details "Resource/", "TeamGame/Resource/", "../Resource/", カレントディレクトリ等
     *          複数のパスを自動検索するため、作業フォルダの差異を気にせず画像ファイルを指定可能です。
     */
    static int LoadGraphWithSearch(const std::string& fileName);

    /**
     * @brief 画面全体（1920x1080）に拡大描画するヘルパー関数
     * @param handle 描画するグラフィックハンドル
     * @param trans 透過描画を行うかどうか（デフォルト: FALSE）
     */
    static void DrawFullScreenGraph(int handle, bool trans = false);

    // ========================================================
    // 【2D画像配列キャッシュ・番号（インデックス）管理システム】
    // タイトルやリザルト画面などで使用する2D画像を一度読み込んだら配列として保持し、
    // 配列の番号（インデックス）を指定して何度でも呼び出せるようにします。
    // ========================================================

    /**
     * @brief 2D画像をロードして内部配列に保存し、その番号（インデックス）を取得する
     * @param fileName 画像ファイル名または相対パス
     * @return int 配列番号（0始まりのインデックス）。読み込み失敗時は -1
     * @details 一度読み込まれた画像は再読み込みせず、既に保存されている配列番号を返します。
     */
    static int Load2DImage(const std::string& fileName);

    /**
     * @brief 配列番号（インデックス）を指定してDxLibグラフィックハンドルを取得する
     * @param imageIndex Load2DImageで取得した配列番号
     * @return int グラフィックハンドル（無効な番号の場合は -1）
     */
    static int Get2DImage(int imageIndex);

    /**
     * @brief 配列番号（インデックス）を指定して2D画像を描画する
     * @param imageIndex 配列番号
     * @param x 描画基準X座標
     * @param y 描画基準Y座標
     * @param trans 透過描画フラグ（デフォルト: true）
     */
    static void Draw2DImage(int imageIndex, int x, int y, bool trans = true);

    /**
     * @brief 配列番号（インデックス）を指定して2D画像を全画面（1920x1080）に描画する
     * @param imageIndex 配列番号
     * @param trans 透過描画フラグ（デフォルト: false）
     */
    static void DrawFullScreen2DImage(int imageIndex, bool trans = false);

    /**
     * @brief 内部配列に保存されている2D画像をすべて破棄・解放する
     */
    static void Clear2DImages();
};

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
};

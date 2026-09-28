#pragma once
#include "Vector2.h"
#include <string>
#include "WeaponManager.h"

/**
 * @brief 武器の基底クラス
 * @details 全ての武器（ハンドガン、ショットガンなど）のベースとなるクラス。共通のパラメータや発射処理などのインターフェースを持つ
 */
class Weapon
{
  protected:
    int coolTimeTimer;
    std::string weaponName;
    const WeaponData* data;
    int currentAmmo;
    bool isReloading;
    int reloadTimer;

  public:
    /**
     * @brief コンストラクタ
     * @param name 武器の名前
     * @details 指定された名前の武器データを取得し、初期化を行う
     */
    Weapon(const std::string &name)
        : weaponName(name), coolTimeTimer(0), isReloading(false), reloadTimer(0)
    {
        data = WeaponManager::GetInstance().GetWeaponData(name);
        if (data) {
            currentAmmo = data->maxAmmo;
        } else {
            currentAmmo = 0;
        }
    }
    virtual ~Weapon() {}

    /**
     * @brief 毎フレームの更新処理
     * @details クールタイムやリロードのタイマーを減算する
     */
    virtual void Update()
    {
        if (isReloading) {
            if (reloadTimer > 0) reloadTimer--;
            if (reloadTimer <= 0) {
                isReloading = false;
                if (data) currentAmmo = data->maxAmmo;
            }
        }
        else {
            if (coolTimeTimer > 0)
                coolTimeTimer--;
        }
    }

    /**
     * @brief 弾を発射する（純粋仮想関数）
     * @param pos 発射位置
     * @param dir 発射方向（正規化ベクトル）
     * @param teamId チームID（0:プレイヤー、1:敵など）
     * @param additionalSpread 追加の拡散角度
     * @details 派生クラスで具体的な弾の発射処理を実装する
     */
    virtual void Fire(const Vector2 &pos, const Vector2 &dir, int teamId = 0, float additionalSpread = 0.0f) = 0;
    
    /**
     * @brief リロード処理
     * @details 弾薬が最大でない場合、リロード状態に移行しタイマーを設定する
     */
    virtual void Reload()
    {
        if (!isReloading && data && currentAmmo < data->maxAmmo) {
            isReloading = true;
            reloadTimer = data->reloadTime;
        }
    }

    /**
     * @brief 発射可能か判定する
     * @return 発射可能なら true
     * @details リロード中でない、クールタイムが終わっている、弾薬がある場合に true を返す
     */
    bool CanFire() const
    {
        return !isReloading && coolTimeTimer <= 0 && currentAmmo > 0;
    }
    
    /**
     * @brief 弾薬を消費する
     * @param amount 消費量（デフォルト1）
     * @details 現在の弾薬を指定量減らし、0未満にならないようにする
     */
    void UseAmmo(int amount = 1)
    {
        currentAmmo -= amount;
        if (currentAmmo <= 0) currentAmmo = 0;
    }

    /**
     * @brief クールタイムをリセットする
     * @details 発射後のクールタイムタイマーをデータに基づいて設定する
     */
    void ResetCoolTime()
    {
        if (data) coolTimeTimer = data->fireInterval;
    }

    /**
     * @brief 武器名を取得する
     * @return 武器名
     */
    std::string GetName() const { return weaponName; }
    int GetCurrentAmmo() const { return currentAmmo; }
    int GetMaxAmmo() const { return data ? data->maxAmmo : 0; }
    bool IsReloading() const { return isReloading; }
    const WeaponData* GetData() const { return data; }
    virtual float GetMoveSpreadPenalty() const { return 15.0f; }
};

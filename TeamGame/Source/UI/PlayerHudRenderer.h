#pragma once

class Player;

/**
 * @brief プレイヤーに紐づくHUD・画面オーバーレイ描画を専任で担うクラス
 * @details Player本体のゲームロジック（移動・射撃・スキル）からUI描画責務を分離し、
 *          キャラクター別HUDプレート画像の寿命をRAIIで安全に管理します。
 */
class PlayerHudRenderer
{
public:
    PlayerHudRenderer() = default;
    ~PlayerHudRenderer();

    PlayerHudRenderer(const PlayerHudRenderer&) = delete;
    PlayerHudRenderer& operator=(const PlayerHudRenderer&) = delete;

    /**
     * @brief 左下のキャラクターHUDプレート（HP / MP / スキル / 武器・弾薬バー）を描画
     */
    void DrawStatusHud(const Player& player, int screenX, int screenY);

    /**
     * @brief 低HP時の画面外枠パルス赤枠・危険警告を描画
     */
    void DrawBloodSplatterOverlay(const Player& player, int screenWidth = 1920, int screenHeight = 1080) const;

    /**
     * @brief 悪魔 HighRisk スキル発動中の敵位置・距離・方向矢印インジケータを描画
     */
    void DrawHighRiskEnemyIndicators(const Player& player) const;

    /**
     * @brief ゲームパッド等のオートエイム発動中におけるターゲットロックオンレティクルを描画
     */
    void DrawAutoAimReticle(const Player& player) const;

private:
    void EnsureHudPlateLoaded(int characterId);

private:
    int m_hudPlateHandle = -1;
    int m_loadedCharacterId = -1;
    bool m_hudPlateLoadAttempted = false;
};

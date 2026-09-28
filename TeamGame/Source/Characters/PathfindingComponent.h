#pragma once
#include <vector>
#include "Vector2.h"

class Stage;

/**
 * @brief パスファインディング（経路探索）機能を提供するコンポーネント
 * @details A*アルゴリズムを使用して、スタート位置から目標位置までの最短経路を計算し、移動方向を決定します。
 */
class PathfindingComponent
{
private:
    std::vector<Vector2> pathWaypoints; ///< 経路のウェイポイントのリスト
    int currentWaypointIndex;           ///< 現在目指しているウェイポイントのインデックス
    
    int lastTargetGridX;                ///< 最後に目標としたグリッドのX座標
    int lastTargetGridY;                ///< 最後に目標としたグリッドのY座標

    int stuckFrames;                    ///< スタック（進行不能）しているフレーム数
    Vector2 lastPos;                    ///< 前回の位置（スタック判定用）

public:
    /**
     * @brief コンストラクタ
     */
    PathfindingComponent();

    /**
     * @brief デストラクタ
     */
    ~PathfindingComponent() = default;

    /**
     * @brief A*アルゴリズムでスタートからゴールまでの経路を計算する
     * @param startPos スタート位置
     * @param targetPos 目標位置
     * @param stage ステージ情報のポインタ
     * @param cellSize グリッドのセルサイズ
     * @return 経路が見つかった場合はtrue、見つからなかった場合はfalse
     */
    bool CalculatePath(const Vector2& startPos, const Vector2& targetPos, const Stage* stage, float cellSize);

    /**
     * @brief 経路に沿って次に進むべき方向（ベクトル）を取得する
     * @param currentPos 現在位置
     * @param speed 移動速度
     * @param cellSize グリッドのセルサイズ
     * @return 次に進むべき方向の正規化ベクトル。経路がない場合や到着した場合はゼロベクトルを返す。
     */
    Vector2 GetMoveDirection(const Vector2& currentPos, float speed, float cellSize);

    /**
     * @brief 現在有効な経路を持っているか確認する
     * @return 経路を持っている場合はtrue、持っていない場合はfalse
     */
    bool HasPath() const;

    /**
     * @brief 現在の経路をクリアする
     */
    void ClearPath();
};

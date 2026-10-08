#include "PathfindingComponent.h"
#include "../Stage/Stage.h"
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <cmath>
#include <algorithm>

struct AStarNode {
    int x, y;
    float gCost; // スタートからの実コスト
    float hCost; // ゴールまでの推定コスト
    float fCost() const { return gCost + hCost; }
    AStarNode* parent;

    AStarNode(int _x, int _y, float _g = 0, float _h = 0, AStarNode* _p = nullptr)
        : x(_x), y(_y), gCost(_g), hCost(_h), parent(_p) {}
};

struct CompareNode {
    bool operator()(const AStarNode* a, const AStarNode* b) const {
        if (a->fCost() == b->fCost()) return a->hCost > b->hCost;
        return a->fCost() > b->fCost();
    }
};

// 座標をハッシュ化するためのヘルパー
struct GridHash {
    std::size_t operator()(const std::pair<int, int>& p) const {
        return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 1);
    }
};

PathfindingComponent::PathfindingComponent()
    : currentWaypointIndex(0), lastTargetGridX(-1), lastTargetGridY(-1), stuckFrames(0), lastPos(0,0)
{
}

bool PathfindingComponent::CalculatePath(const Vector2& startPos, const Vector2& targetPos, const Stage* stage, float cellSize)
{
    if (!stage || cellSize <= 0.0f) return false;

    int startX = static_cast<int>(startPos.x / cellSize);
    int startY = static_cast<int>(startPos.y / cellSize);
    int goalX = static_cast<int>(targetPos.x / cellSize);
    int goalY = static_cast<int>(targetPos.y / cellSize);

    if (stage->IsSolidWall(startX, startY)) {
        for (int r = 1; r <= 3; ++r) {
            bool found = false;
            for (int dy = -r; dy <= r && !found; ++dy) {
                for (int dx = -r; dx <= r && !found; ++dx) {
                    int nx = startX + dx;
                    int ny = startY + dy;
                    if (!stage->IsOutOfBounds(nx, ny) && !stage->IsSolidWall(nx, ny)) {
                        startX = nx;
                        startY = ny;
                        found = true;
                    }
                }
            }
            if (found) break;
        }
    }

    if (stage->IsSolidWall(goalX, goalY)) {
        bool foundValid = false;
        int bestDist = 999999;
        int validX = goalX;
        int validY = goalY;
        for (int r = 1; r <= 6; ++r) {
            for (int dy = -r; dy <= r; ++dy) {
                for (int dx = -r; dx <= r; ++dx) {
                    int nx = goalX + dx;
                    int ny = goalY + dy;
                    if (!stage->IsOutOfBounds(nx, ny) && !stage->IsSolidWall(nx, ny)) {
                        int dist = std::abs(nx - startX) + std::abs(ny - startY);
                        if (dist < bestDist) {
                            bestDist = dist;
                            validX = nx;
                            validY = ny;
                            foundValid = true;
                        }
                    }
                }
            }
            if (foundValid) break;
        }
        if (foundValid) {
            goalX = validX;
            goalY = validY;
        } else {
            return false;
        }
    }

    if (HasPath() && goalX == lastTargetGridX && goalY == lastTargetGridY) {
        return true;
    }

    lastTargetGridX = goalX;
    lastTargetGridY = goalY;
    ClearPath();

    std::priority_queue<AStarNode*, std::vector<AStarNode*>, CompareNode> openSet;
    std::unordered_map<std::pair<int, int>, AStarNode*, GridHash> allNodes;
    std::unordered_set<std::pair<int, int>, GridHash> closedSet;

    auto getManhattan = [](int x1, int y1, int x2, int y2) {
        return std::abs(x1 - x2) + std::abs(y1 - y2);
    };

    AStarNode* startNode = new AStarNode(startX, startY, 0, getManhattan(startX, startY, goalX, goalY));
    openSet.push(startNode);
    allNodes[{startX, startY}] = startNode;

    const int dx[] = {0, 1, 0, -1, 1, 1, -1, -1};
    const int dy[] = {-1, 0, 1, 0, -1, 1, 1, -1};

    bool found = false;
    AStarNode* targetNode = nullptr;

    // 計算回数制限（重くなりすぎないように）
    int loopCount = 0;
    while (!openSet.empty() && loopCount < 500) {
        loopCount++;
        AStarNode* current = openSet.top();
        openSet.pop();

        if (current->x == goalX && current->y == goalY) {
            found = true;
            targetNode = current;
            break;
        }

        closedSet.insert({current->x, current->y});

        for (int i = 0; i < 8; ++i) {
            int nx = current->x + dx[i];
            int ny = current->y + dy[i];

            if (nx < 0 || nx >= stage->GetWidth() || ny < 0 || ny >= stage->GetHeight()) continue;
            if (stage->IsSolidWall(nx, ny)) continue;
            
            // 斜め移動の壁抜け防止
            if (i >= 4) {
                if (stage->IsSolidWall(current->x, ny) || stage->IsSolidWall(nx, current->y)) continue;
            }

            if (closedSet.count({nx, ny})) continue;

            float moveCost = (i < 4) ? 1.0f : 1.414f;
            float newGCost = current->gCost + moveCost;

            AStarNode* neighbor = nullptr;
            if (allNodes.count({nx, ny})) {
                neighbor = allNodes[{nx, ny}];
                if (newGCost < neighbor->gCost) {
                    neighbor->gCost = newGCost;
                    neighbor->parent = current;
                }
            } else {
                neighbor = new AStarNode(nx, ny, newGCost, getManhattan(nx, ny, goalX, goalY), current);
                allNodes[{nx, ny}] = neighbor;
                openSet.push(neighbor);
            }
        }
    } 

    if (found && targetNode) {
        AStarNode* curr = targetNode;
        while (curr != nullptr) {
            // グリッドの中心座標をウェイポイントとして記録
            pathWaypoints.push_back(Vector2((curr->x + 0.5f) * cellSize, (curr->y + 0.5f) * cellSize));
            curr = curr->parent;
        }
        std::reverse(pathWaypoints.begin(), pathWaypoints.end());
        currentWaypointIndex = 0;
        
        // スタート位置のノードは除外する（既にそこにいるため）
        if (pathWaypoints.size() > 1) {
            currentWaypointIndex = 1;
        }
    }

    for (auto pair : allNodes) {
        delete pair.second;
    }

    return found;
}

Vector2 PathfindingComponent::GetMoveDirection(const Vector2& currentPos, float speed, float cellSize)
{
    if (!HasPath()) return Vector2(0, 0);

    // スタック判定
    float dxL = currentPos.x - lastPos.x;
    float dyL = currentPos.y - lastPos.y;
    if (std::sqrt(dxL*dxL + dyL*dyL) < speed * 0.2f) {
        stuckFrames++;
    } else {
        stuckFrames = 0;
    }
    lastPos = currentPos;

    // 長時間スタックしたら経路を放棄して再計算を促す
    if (stuckFrames > 30) {
        ClearPath();
        stuckFrames = 0;
        return Vector2(0, 0);
    }

    Vector2 targetWaypoint = pathWaypoints[currentWaypointIndex];
    float dx = targetWaypoint.x - currentPos.x;
    float dy = targetWaypoint.y - currentPos.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    // ウェイポイントに到達したら次のウェイポイントへ
    if (dist < cellSize * 0.5f) {
        currentWaypointIndex++;
        if (!HasPath()) {
            return Vector2(0, 0);
        }
        targetWaypoint = pathWaypoints[currentWaypointIndex];
        dx = targetWaypoint.x - currentPos.x;
        dy = targetWaypoint.y - currentPos.y;
        dist = std::sqrt(dx * dx + dy * dy);
    }

    if (dist > 0.0001f) {
        return Vector2(dx / dist, dy / dist);
    }
    return Vector2(0, 0);
}

bool PathfindingComponent::HasPath() const
{
    return currentWaypointIndex < pathWaypoints.size();
}

void PathfindingComponent::ClearPath()
{
    pathWaypoints.clear();
    currentWaypointIndex = 0;
    stuckFrames = 0;
}

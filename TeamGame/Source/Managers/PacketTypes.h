#pragma once
#include <stdint.h>

/**
 * @brief パケットの種類を定義する列挙型
 * @details ネットワーク通信で送信されるデータの種別を表す。
 */
enum class PacketType : uint8_t {
    STAGE_INIT,   // ステージ初期化
    PLAYER_STATE, // プレイヤー状態
    ENEMY_STATE,  // 敵状態
    SHOOT         // 射撃アクション
};

#pragma pack(push, 1)

/**
 * @brief パケットの共通ヘッダ
 * @details すべてのパケットの先頭に付与され、パケットの種類を識別する。
 */
struct PacketHeader {
    PacketType type;
};

/**
 * @brief ステージ初期化パケット
 * @details シード値やテーマなど、ステージ生成に必要な情報を同期する。
 */
struct PacketStageInit {
    PacketHeader header;
    unsigned int seed;
    int themeIdx;
    int varIdx;
};

/**
 * @brief プレイヤー状態パケット
 * @details プレイヤーの座標、向き、状態などの情報を同期する。
 */
struct PacketPlayerState {
    PacketHeader header;
    float x;
    float y;
    float facingX;
    float facingY;
    bool isLightOn;
    bool isInBush;
    int currentWeaponIndex;
};

/**
 * @brief 敵データ構造体
 * @details 敵1体の状態を保持する。
 */
struct EnemyData {
    int id;
    float x;
    float y;
    int hp;
    bool isActive;
};

/**
 * @brief 敵状態パケット
 * @details 複数の敵の状態を一括して同期する。最大50体。
 */
struct PacketEnemyState {
    PacketHeader header;
    int enemyCount;
    EnemyData enemies[50]; // Max 50 enemies for now to keep packet size simple
};

/**
 * @brief 射撃アクションパケット
 * @details プレイヤーの射撃アクション（武器、位置、方向）を同期する。
 */
struct PacketShoot {
    PacketHeader header;
    int weaponIndex;
    float x;
    float y;
    float dirX;
    float dirY;
};

#pragma pack(pop)

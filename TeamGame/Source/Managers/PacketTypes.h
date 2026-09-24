#pragma once
#include <stdint.h>

enum class PacketType : uint8_t {
    STAGE_INIT,
    PLAYER_STATE,
    ENEMY_STATE,
    SHOOT
};

#pragma pack(push, 1)

struct PacketHeader {
    PacketType type;
};

struct PacketStageInit {
    PacketHeader header;
    unsigned int seed;
    int themeIdx;
    int varIdx;
};

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

struct EnemyData {
    int id;
    float x;
    float y;
    int hp;
    bool isActive;
};

struct PacketEnemyState {
    PacketHeader header;
    int enemyCount;
    EnemyData enemies[50]; // Max 50 enemies for now to keep packet size simple
};

struct PacketShoot {
    PacketHeader header;
    int weaponIndex;
    float x;
    float y;
    float dirX;
    float dirY;
};

#pragma pack(pop)

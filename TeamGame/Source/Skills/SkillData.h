#pragma once
#include <string>
#include <vector>

// 大分類タグ
enum class SkillMajorTag {
    StatusBuff,   // 自分や味方のステータス上昇・回復
    Debuff,       // 敵への妨害
    Trap          // 設置型トラップ
};

// 小分類タグ
enum class SkillMinorTag {
    None,
    Heal,         // 回復
    AttackUp,     // 攻撃力アップ
    SpeedUp,      // 移動速度アップ
    AttackDown,   // 攻撃力ダウン（デバフ）
    SpeedDown,    // 移動速度ダウン（デバフ）
    Blind,        // 目くらまし
    Stun,         // スタン（行動不能）
    HomingKill,   // 味方Bot特攻自爆
    Transparency, // ステルス透明化
    PanicShoot,   // パニック射撃（錯乱/同士討ち）
    CleanTime,    // 衝撃波（吹き飛ばし）
    HighRisk      // 強制音発生＆敵位置探知
};

// CSVから読み込むデータ構造
struct SkillData {
    int id;
    std::string name;
    std::string description; // スキルの説明文
    std::string imagePath;   // アイコン画像のファイルパス
    SkillMajorTag majorTag;
    SkillMinorTag minorTag;
    float effectValue; // 回復量やバフ倍率、デバフの効果範囲など
    int duration;      // 効果時間（フレーム。0なら即効性）
    int coolTime;      // クールタイム（フレーム）
};

// スキルデータを一括管理・ロードするクラス
class SkillDataManager {
private:
    std::vector<SkillData> skillDatabase;

    SkillDataManager();

    SkillMajorTag ParseMajorTag(const std::string& str) const;
    SkillMinorTag ParseMinorTag(const std::string& str) const;

public:
    static SkillDataManager& GetInstance();

    // CSVファイルの読み込み
    bool LoadFromCSV(const std::string& filePath);

    // データ検索
    const SkillData* GetSkillData(int id) const;
};

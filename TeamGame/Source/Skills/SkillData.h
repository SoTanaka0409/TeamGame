#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>

// 大分類タグ
enum class SkillMajorTag {
    StatusBuff,   // 自分や味方のステータス上昇・回復
    Debuff,       // 敵への妨害
    Trap          // 設置型トラップ（今回は未実装・拡張用）
};

// 小分類タグ
enum class SkillMinorTag {
    None,
    Heal,         // 回復
    AttackUp,     // 攻撃力アップ
    SpeedUp,      // 移動速度アップ
    Blind,        // 目くらまし
    Stun          // スタン（行動不能）
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

    SkillDataManager() {} // シングルトン

    SkillMajorTag ParseMajorTag(const std::string& str) {
        if (str == "Debuff") return SkillMajorTag::Debuff;
        if (str == "Trap") return SkillMajorTag::Trap;
        return SkillMajorTag::StatusBuff; // デフォルト
    }

    SkillMinorTag ParseMinorTag(const std::string& str) {
        if (str == "Heal") return SkillMinorTag::Heal;
        if (str == "AttackUp") return SkillMinorTag::AttackUp;
        if (str == "SpeedUp") return SkillMinorTag::SpeedUp;
        if (str == "Blind") return SkillMinorTag::Blind;
        if (str == "Stun") return SkillMinorTag::Stun;
        return SkillMinorTag::None;
    }

public:
    static SkillDataManager& GetInstance() {
        static SkillDataManager instance;
        return instance;
    }

    // CSVファイルの読み込み
    bool LoadFromCSV(const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) return false;

        std::string line;
        // ヘッダー行を読み飛ばす場合は以下を有効に
        // std::getline(file, line); 

        while (std::getline(file, line)) {
            if (line.empty()) continue;

            std::stringstream ss(line);
            std::string cell;
            SkillData data;

            // ID, Name, Description, ImagePath, MajorTag, MinorTag, EffectValue, Duration, CoolTime
            std::getline(ss, cell, ','); data.id = std::stoi(cell);
            std::getline(ss, cell, ','); data.name = cell;
            std::getline(ss, cell, ','); data.description = cell;
            std::getline(ss, cell, ','); data.imagePath = cell;
            std::getline(ss, cell, ','); data.majorTag = ParseMajorTag(cell);
            std::getline(ss, cell, ','); data.minorTag = ParseMinorTag(cell);
            std::getline(ss, cell, ','); data.effectValue = std::stof(cell);
            std::getline(ss, cell, ','); data.duration = std::stoi(cell);
            std::getline(ss, cell, ','); data.coolTime = std::stoi(cell);

            skillDatabase.push_back(data);
        }
        return true;
    }

    const SkillData* GetSkillData(int id) const {
        for (const auto& skill : skillDatabase) {
            if (skill.id == id) return &skill;
        }
        return nullptr;
    }
};

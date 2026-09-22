/*--------------------------------------------------------------------

ファイル名：StageObjective.cpp
お仕事　　：ステージごとのチャレンジ目標

必要数はステージが進むほど増え、
ボーナスは難易度が上がるほど大きくなる。

    EASY    ×1.0
    NORMAL  ×1.8
    HARD    ×3.0

----------------------------------------------------------------------*/

#include "StageObjective.h"

#include <chrono>
#include <random>

namespace
{
    float BonusScale(DifficultyType difficulty)
    {
        switch (difficulty)
        {
        case DifficultyType::Easy:   return 1.0f;
        case DifficultyType::Normal: return 1.8f;
        case DifficultyType::Hard:   return 3.0f;
        }

        return 1.0f;
    }

    std::wstring Num(int value)
    {
        return std::to_wstring(value);
    }

    StageObjective Make(
        ObjectiveType type,
        int target,
        int baseBonus,
        DifficultyType difficulty,
        const std::wstring& text
    )
    {
        StageObjective objective;

        objective.type = type;
        objective.target = target;
        objective.text = text;

        objective.bonus =
            static_cast<int>(baseBonus * BonusScale(difficulty));

        return objective;
    }
}

std::vector<StageObjective> GetObjectiveCandidates(
    int stageIndex,
    DifficultyType difficulty
)
{
    // ステージが進むほど条件を厳しくする
    const int step = stageIndex + 1;

    std::vector<StageObjective> list;

    list.push_back(Make(
        ObjectiveType::NoDamage, 0, 2000, difficulty,
        L"一度も被弾せずにクリアする"
    ));

    const int seconds = 150 + stageIndex * 30;

    list.push_back(Make(
        ObjectiveType::TimeLimit, seconds, 1600, difficulty,
        Num(seconds) + L"秒以内にクリアする"
    ));

    const int headShots = 8 + step * 4;

    list.push_back(Make(
        ObjectiveType::HeadShot, headShots, 1400, difficulty,
        L"ヘッドショットを" + Num(headShots) + L"回決める"
    ));

    const int elites = step;

    list.push_back(Make(
        ObjectiveType::EliteKill, elites, 1800, difficulty,
        L"エリートを" + Num(elites) + L"体倒す"
    ));

    const int combo = 8 + step * 3;

    list.push_back(Make(
        ObjectiveType::Combo, combo, 1500, difficulty,
        L"最大コンボを" + Num(combo) + L"以上にする"
    ));

    const int pickups = 3 + step;

    list.push_back(Make(
        ObjectiveType::Pickup, pickups, 1200, difficulty,
        L"装備を" + Num(pickups) + L"個拾う"
    ));

    return list;
}

StageObjective RollObjective(int stageIndex, DifficultyType difficulty)
{
    const std::vector<StageObjective> list =
        GetObjectiveCandidates(stageIndex, difficulty);

    if (list.empty())
    {
        return StageObjective();
    }

    static std::mt19937 rng{
        static_cast<unsigned>(
            std::chrono::high_resolution_clock::now()
            .time_since_epoch().count()
        )
    };

    std::uniform_int_distribution<int> dist(
        0, static_cast<int>(list.size()) - 1
    );

    return list[dist(rng)];
}

//------------------------------------------------------------
// その目標を達成しやすくするシナジー
//------------------------------------------------------------
std::vector<SynergyType> GetObjectiveSynergies(ObjectiveType type)
{
    switch (type)
    {
    case ObjectiveType::NoDamage:
        // 硬さと回避で被弾を避ける
        return { SynergyType::Guardian, SynergyType::Velocity };

    case ObjectiveType::TimeLimit:
        // 手数と機動力で早く片付ける
        return { SynergyType::Assault, SynergyType::Velocity };

    case ObjectiveType::HeadShot:
        return { SynergyType::Redeye, SynergyType::Hunter };

    case ObjectiveType::EliteKill:
        return { SynergyType::Executioner, SynergyType::Hunter };

    case ObjectiveType::Combo:
        // 弾切れを起こさず撃ち続ける
        return { SynergyType::Assault, SynergyType::Gunner };

    case ObjectiveType::Pickup:
        return { SynergyType::Fortune };
    }

    return {};
}

std::wstring FormatObjectiveProgress(
    const StageObjective& objective,
    int progress
)
{
    if (objective.type == ObjectiveType::NoDamage)
    {
        return (progress > 0) ? L"失敗" : L"継続中";
    }

    if (objective.type == ObjectiveType::TimeLimit)
    {
        const int left = objective.target - progress;

        if (left <= 0)
        {
            return L"時間切れ";
        }

        return L"残り " + std::to_wstring(left) + L" 秒";
    }

    return std::to_wstring(progress) + L" / " +
           std::to_wstring(objective.target);
}

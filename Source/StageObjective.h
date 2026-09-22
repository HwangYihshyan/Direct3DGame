/*--------------------------------------------------------------------

ファイル名：StageObjective.h
お仕事　　：ステージごとのチャレンジ目標

ステージ開始時に、難易度に応じた目標を1つ選ぶ。
達成してクリアすると、難易度が高いほど大きなボーナスが入る。

目標は「そのステージの中だけ」で数える。
前のステージの成績は持ち越さない。

----------------------------------------------------------------------*/

#ifndef STAGEOBJECTIVE_H
#define STAGEOBJECTIVE_H

#include <string>
#include <vector>

#include "CommonLevel.h"
#include "EquipmentTypes.h"

enum class ObjectiveType
{
    NoDamage,     // 被弾しないでクリアする
    TimeLimit,    // 制限時間内にクリアする
    HeadShot,     // HeadShotを一定回数決める
    EliteKill,    // Eliteを一定数倒す
    Combo,        // 最大コンボを一定以上にする
    Pickup        // 装備を一定数拾う
};

struct StageObjective
{
    ObjectiveType type = ObjectiveType::NoDamage;

    // 達成に必要な数（NoDamage は 0）
    int target = 0;

    // 達成したときの加点
    int bonus = 0;

    // 表示用
    std::wstring text;
};

// そのステージで出る可能性のある目標をすべて返す（レベル選択の一覧用）
std::vector<StageObjective> GetObjectiveCandidates(
    int stageIndex,
    DifficultyType difficulty
);

// ステージ開始時に1つ選ぶ
StageObjective RollObjective(int stageIndex, DifficultyType difficulty);

// その目標を助けるシナジー（宝箱の抽選で優遇する）
std::vector<SynergyType> GetObjectiveSynergies(ObjectiveType type);

// 現在の進み具合を「3 / 5」のような文字列にする
std::wstring FormatObjectiveProgress(
    const StageObjective& objective,
    int progress
);

#endif

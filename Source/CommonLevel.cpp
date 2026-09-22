/*--------------------------------------------------------------------

ファイル名：CommonLevel.cpp
お仕事　　：難易度とステージ別設定

Level_1 / Level_2 / Level_3 を別ファイルに分けると
中身がテーブル1行ずつになってしまうため、
1ファイルにまとめて管理している。

レアリティ確率そのものは EquipmentGenerator.cpp が持つ。
（設計書 19-9 のRarity表と同じ値）

----------------------------------------------------------------------*/

#include "CommonLevel.h"

namespace
{
    //                              HP    ATK   SPD   数    Elite  Drop
    const DifficultyModifier EASY   { 0.75f, 0.75f, 0.90f, 0.75f, 0.80f, 1.20f };
    const DifficultyModifier NORMAL { 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f };
    const DifficultyModifier HARD   { 1.45f, 1.35f, 1.12f, 1.30f, 1.40f, 1.15f };

    const LevelSetting LEVELS[] =
    {
        {
            StageType::Forest,
            L"ステージ1 - 森林",
            L"シナジーの効果を発動する",
            240,
            L"B ～ D"
        },
        {
            StageType::Suburb,
            L"ステージ2 - 平原",
            L"必要な装備を集める",
            270,
            L"A ～ B"
        },
        {
            StageType::Town,
            L"ステージ3 - 郊外",
            L"最終ビルドを完成させる",
            300,
            L"SS ～ A"
        },
        {
            StageType::Boss,
            L"最終 - ボス",
            L"ボスを撃破する",
            180,
            L"SSS ～ S"
        }
    };
}

const wchar_t* ToString(DifficultyType value)
{
    switch (value)
    {
    case DifficultyType::Easy:   return L"イージー";
    case DifficultyType::Normal: return L"ノーマル";
    case DifficultyType::Hard:   return L"ハード";
    }

    return L"ノーマル";
}

const DifficultyModifier& GetDifficultyModifier(DifficultyType difficulty)
{
    switch (difficulty)
    {
    case DifficultyType::Easy:   return EASY;
    case DifficultyType::Normal: return NORMAL;
    case DifficultyType::Hard:   return HARD;
    }

    return NORMAL;
}

const LevelSetting& GetLevelSetting(int stageIndex)
{
    if (stageIndex < 0)
    {
        stageIndex = 0;
    }

    if (stageIndex > 3)
    {
        stageIndex = 3;
    }

    return LEVELS[stageIndex];
}

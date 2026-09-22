/*--------------------------------------------------------------------

ファイル名：CommonLevel.h
お仕事　　：難易度とステージ別設定

設計書 Ver.3 19-9.1 ～ 19-9.4
    CommonLevel  共通の難易度補正
    Level_1 森林 / Level_2 平原 / Level_3 郊外

----------------------------------------------------------------------*/

#ifndef COMMONLEVEL_H
#define COMMONLEVEL_H

#include "EquipmentTypes.h"

enum class DifficultyType
{
    Easy,
    Normal,
    Hard
};

const wchar_t* ToString(DifficultyType value);

//------------------------------------------------------------
// 難易度による共通補正（設計書 19-9.1）
//------------------------------------------------------------
struct DifficultyModifier
{
    float enemyHp = 1.0f;
    float enemyAttack = 1.0f;
    float enemySpeed = 1.0f;
    float enemyCount = 1.0f;    // 同時出現数の倍率
    float eliteRate = 1.0f;
    float dropRate = 1.0f;
};

const DifficultyModifier& GetDifficultyModifier(DifficultyType difficulty);

//------------------------------------------------------------
// ステージ固有設定（設計書 19-9.2 ～ 19-9.4）
//------------------------------------------------------------
struct LevelSetting
{
    StageType stage;
    const wchar_t* displayName;
    const wchar_t* objective;      // そのステージの目標
    int targetSeconds;             // 目安クリア時間
    const wchar_t* recommendGear;  // 推奨装備
};

const LevelSetting& GetLevelSetting(int stageIndex);

#endif

/*--------------------------------------------------------------------

ファイル名：Rarity.h
お仕事　　：レアリティ倍率テーブル

設計書　　：Ver.2 「5. レアリティ倍率」「5-2. レアリティごとの価値」

----------------------------------------------------------------------*/

#ifndef RARITY_H
#define RARITY_H

#include "EquipmentTypes.h"

struct RarityData
{
    Rarity rarity;

    // 基礎攻撃力／防具メインステータスに掛ける倍率
    float multiplier;

    // ランダムOPの個数
    int randomOptionCount;

    // シナジー個数の下限／上限
    int minSynergyCount;
    int maxSynergyCount;

    // SSSのみ固有能力を持つ
    bool hasMythicEffect;

    // 2個目のシナジーが付く確率（%）
    int secondSynergyChance;
};

class RarityTable
{
public:
    static const RarityData& Get(Rarity rarity);
};

#endif

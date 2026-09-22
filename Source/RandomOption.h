/*--------------------------------------------------------------------

ファイル名：RandomOption.h
お仕事　　：ランダムOPの数値範囲テーブル

設計書　　：Ver.2 「9. 武器ランダムOP」「10. 防具ランダムOP」

----------------------------------------------------------------------*/

#ifndef RANDOMOPTION_H
#define RANDOMOPTION_H

#include "EquipmentTypes.h"

struct OptionRange
{
    float minValue;
    float maxValue;
};

struct RandomOption
{
    RandomOptionType type;
    float value;
};

class RandomOptionTable
{
public:
    // 武器OPの範囲（レアリティD＝OPなしなので 0,0 を返す）
    static OptionRange GetWeaponRange(
        RandomOptionType type,
        Rarity rarity
    );

    // 防具OPの範囲（部位ごとにプールが違う）
    static OptionRange GetArmorRange(
        ArmorSlot slot,
        RandomOptionType type,
        Rarity rarity
    );
};

#endif

/*--------------------------------------------------------------------

ファイル名：ArmorData.h
お仕事　　：防具1個分のデータ

設計書 5.
「防具は、主にその部位のメインステータスに倍率を掛ける」
→ mainStatValue にだけレアリティ倍率を掛ける。

----------------------------------------------------------------------*/

#ifndef ARMORDATA_H
#define ARMORDATA_H

#include <string>
#include <vector>

#include "EquipmentTypes.h"
#include "RandomOption.h"

struct ArmorData
{
    ArmorSlot slot = ArmorSlot::Head;
    std::wstring name;

    // その部位で出るランダムOP候補（設計書 10.）
    std::vector<RandomOptionType> optionPool;

    // その部位で出やすいシナジー（設計書 4.）
    std::vector<SynergyType> preferredSynergies;

    //--------------------------------------------------------
    // メインステータス
    // レアリティ倍率が掛かるのはここだけ
    //--------------------------------------------------------
    RandomOptionType mainStatType = RandomOptionType::Defense;
    float mainStatBase = 0.0f;

    // 全部位共通の素の防御力（倍率は掛けない）
    float baseDefense = 0.0f;

    //--------------------------------------------------------
    // ドロップ時に決まる部分
    //--------------------------------------------------------
    Rarity rarity = Rarity::Common;

    // レアリティ倍率適用後のメインステータス
    float mainStatValue = 0.0f;

    std::vector<RandomOption> randomOptions;
    std::vector<SynergyType> synergies;
};

#endif

/*--------------------------------------------------------------------

ファイル名：Rarity.cpp
お仕事　　：レアリティ倍率テーブル

設計書 5-1. 推奨レアリティ倍率
    D   ×1.00 / C ×1.10 / B ×1.22 / A ×1.36
    S   ×1.52 / SS ×1.70 / SSS ×1.90

設計書 3-2. レアリティ別 シナジー数（2個目が付く確率）
    D 0% / C 10% / B 25% / A 45% / S 65% / SS 85% / SSS 100%

----------------------------------------------------------------------*/

#include "Rarity.h"

namespace
{
    //                 rarity              mult   OP  min max mythic  2個目%
    const RarityData TABLE[] =
    {
        { Rarity::Common,    1.00f, 0, 1, 1, false,   0 },
        { Rarity::Uncommon,  1.10f, 1, 1, 2, false,  10 },
        { Rarity::Rare,      1.22f, 1, 1, 2, false,  25 },
        { Rarity::Special,   1.36f, 2, 1, 2, false,  45 },
        { Rarity::Epic,      1.52f, 2, 1, 2, false,  65 },
        { Rarity::Legendary, 1.70f, 3, 1, 2, false,  85 },
        { Rarity::Mythic,    1.90f, 3, 2, 2, true,  100 }
    };
}

const RarityData& RarityTable::Get(Rarity rarity)
{
    int index = static_cast<int>(rarity);

    if (index < 0 || index >= RARITY_COUNT)
    {
        index = 0;
    }

    return TABLE[index];
}

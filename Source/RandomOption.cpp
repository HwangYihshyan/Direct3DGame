/*--------------------------------------------------------------------

ファイル名：RandomOption.cpp
お仕事　　：ランダムOPの数値範囲テーブル

配列は C / B / A / S / SS / SSS の6段階。
D（Common）はランダムOPを持たないので範囲を引かない。

----------------------------------------------------------------------*/

#include "RandomOption.h"

namespace
{
    // Common を除いた 6 段階の添字へ変換
    int Index(Rarity r)
    {
        return static_cast<int>(r) - 1;
    }

    OptionRange Pick(Rarity r, const OptionRange(&values)[6])
    {
        if (r == Rarity::Common)
        {
            return { 0.0f, 0.0f };
        }

        const int index = Index(r);

        if (index < 0 || index > 5)
        {
            return { 0.0f, 0.0f };
        }

        return values[index];
    }
}

//------------------------------------------------------------
// 武器OP
// 設計書 9-1. 武器OP 数値範囲
//------------------------------------------------------------
OptionRange RandomOptionTable::GetWeaponRange(
    RandomOptionType type,
    Rarity rarity
)
{
    static const OptionRange attack[6] = { {3,5},{5,8},{8,12},{12,16},{16,22},{22,30} };
    static const OptionRange critRate[6] = { {2,3},{3,5},{5,7},{7,10},{10,13},{13,18} };
    static const OptionRange critDamage[6] = { {5,10},{10,15},{15,22},{22,30},{30,40},{40,55} };
    static const OptionRange fireRate[6] = { {3,5},{5,8},{8,12},{12,16},{16,20},{20,25} };
    static const OptionRange reload[6] = { {4,7},{7,10},{10,14},{14,19},{19,25},{25,32} };
    static const OptionRange magazine[6] = { {5,8},{8,12},{12,18},{18,25},{25,35},{35,45} };
    static const OptionRange headshot[6] = { {6,10},{10,15},{15,22},{22,30},{30,40},{40,55} };
    static const OptionRange armorPen[6] = { {3,5},{5,8},{8,12},{12,17},{17,23},{23,30} };

    switch (type)
    {
    case RandomOptionType::AttackPercent:    return Pick(rarity, attack);
    case RandomOptionType::CritRate:         return Pick(rarity, critRate);
    case RandomOptionType::CritDamage:       return Pick(rarity, critDamage);
    case RandomOptionType::FireRate:         return Pick(rarity, fireRate);
    case RandomOptionType::ReloadSpeed:      return Pick(rarity, reload);
    case RandomOptionType::MagazineSize:     return Pick(rarity, magazine);
    case RandomOptionType::HeadShotDamage:   return Pick(rarity, headshot);
    case RandomOptionType::ArmorPenetration: return Pick(rarity, armorPen);
    default: return { 0.0f, 0.0f };
    }
}

//------------------------------------------------------------
// 防具OP
// 設計書 10. 防具ランダムOP
//------------------------------------------------------------
OptionRange RandomOptionTable::GetArmorRange(
    ArmorSlot slot,
    RandomOptionType type,
    Rarity rarity
)
{
    // HEAD
    static const OptionRange critRate[6] = { {2,3},{3,5},{5,7},{7,10},{10,13},{13,16} };
    static const OptionRange critDamage[6] = { {5,8},{8,12},{12,18},{18,25},{25,35},{35,45} };
    static const OptionRange headshot[6] = { {5,10},{10,15},{15,22},{22,30},{30,40},{40,50} };
    static const OptionRange detection[6] = { {5,8},{8,12},{12,18},{18,25},{25,35},{35,45} };

    // BODY
    static const OptionRange hpDef[6] = { {4,6},{6,9},{9,13},{13,18},{18,24},{24,30} };
    static const OptionRange reduction[6] = { {1,1},{1,2},{2,3},{3,4},{4,6},{6,8} };

    // LEG
    static const OptionRange reserve[6] = { {8,12},{12,18},{18,25},{25,35},{35,45},{45,60} };
    static const OptionRange magazine[6] = { {5,8},{8,12},{12,18},{18,25},{25,35},{35,45} };
    static const OptionRange reload[6] = { {4,7},{7,10},{10,14},{14,19},{19,25},{25,32} };

    // ARM
    static const OptionRange attack[6] = { {3,5},{5,8},{8,12},{12,16},{16,22},{22,28} };
    static const OptionRange fireRate[6] = { {3,5},{5,8},{8,12},{12,16},{16,20},{20,25} };
    static const OptionRange handling[6] = { {5,8},{8,12},{12,18},{18,25},{25,35},{35,45} };

    // BOOTS
    static const OptionRange moveSpeed[6] = { {2,3},{3,5},{5,7},{7,10},{10,13},{13,16} };
    static const OptionRange dodge[6] = { {1,2},{2,3},{3,5},{5,7},{7,9},{9,11} };
    static const OptionRange dashCD[6] = { {3,5},{5,8},{8,12},{12,16},{16,20},{20,25} };
    static const OptionRange dashDist[6] = { {4,7},{7,10},{10,14},{14,19},{19,25},{25,32} };

    if (slot == ArmorSlot::Head)
    {
        if (type == RandomOptionType::CritRate)       return Pick(rarity, critRate);
        if (type == RandomOptionType::CritDamage)     return Pick(rarity, critDamage);
        if (type == RandomOptionType::HeadShotDamage) return Pick(rarity, headshot);
        if (type == RandomOptionType::DetectionRange) return Pick(rarity, detection);
    }

    if (slot == ArmorSlot::Body)
    {
        if (type == RandomOptionType::MaxHP ||
            type == RandomOptionType::Defense ||
            type == RandomOptionType::Shield)
        {
            return Pick(rarity, hpDef);
        }

        if (type == RandomOptionType::DamageReduction) return Pick(rarity, reduction);
    }

    if (slot == ArmorSlot::Leg)
    {
        if (type == RandomOptionType::ReserveAmmo)  return Pick(rarity, reserve);
        if (type == RandomOptionType::MagazineSize ||
            type == RandomOptionType::AmmoDrop)     return Pick(rarity, magazine);
        if (type == RandomOptionType::ReloadSpeed)  return Pick(rarity, reload);
    }

    if (slot == ArmorSlot::Arm)
    {
        if (type == RandomOptionType::AttackPercent) return Pick(rarity, attack);
        if (type == RandomOptionType::FireRate)      return Pick(rarity, fireRate);
        if (type == RandomOptionType::RecoilControl ||
            type == RandomOptionType::WeaponHandling) return Pick(rarity, handling);
    }

    if (slot == ArmorSlot::Boots)
    {
        if (type == RandomOptionType::MoveSpeed)    return Pick(rarity, moveSpeed);
        if (type == RandomOptionType::Dodge)        return Pick(rarity, dodge);
        if (type == RandomOptionType::DashCooldown) return Pick(rarity, dashCD);
        if (type == RandomOptionType::DashDistance) return Pick(rarity, dashDist);
    }

    return { 0.0f, 0.0f };
}

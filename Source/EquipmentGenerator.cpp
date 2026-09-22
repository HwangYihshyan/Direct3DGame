/*--------------------------------------------------------------------

ファイル名：EquipmentGenerator.cpp
お仕事　　：装備1個を生成する

----------------------------------------------------------------------*/

#include "EquipmentGenerator.h"
#include "EquipmentDatabase.h"
#include "Rarity.h"
#include "RandomOption.h"

#include <algorithm>
#include <array>
#include <chrono>

namespace
{
    struct RarityWeight
    {
        Rarity rarity;
        int weight;
    };

    //--------------------------------------------------------
    // 設計書 7. ステージ1～3 レアリティ確率
    //--------------------------------------------------------
    std::vector<RarityWeight> StageTable(StageType stage)
    {
        switch (stage)
        {
        case StageType::Forest:
            return {
                { Rarity::Common,    45 },
                { Rarity::Uncommon,  35 },
                { Rarity::Rare,      15 },
                { Rarity::Special,    4 },
                { Rarity::Epic,       1 },
                { Rarity::Legendary,  0 },
                { Rarity::Mythic,     0 }
            };

        case StageType::Suburb:
            return {
                { Rarity::Common,    10 },
                { Rarity::Uncommon,  30 },
                { Rarity::Rare,      30 },
                { Rarity::Special,   20 },
                { Rarity::Epic,       8 },
                { Rarity::Legendary,  2 },
                { Rarity::Mythic,     0 }
            };

        case StageType::Town:
            return {
                { Rarity::Common,     2 },
                { Rarity::Uncommon,  10 },
                { Rarity::Rare,      25 },
                { Rarity::Special,   30 },
                { Rarity::Epic,      20 },
                { Rarity::Legendary, 10 },
                { Rarity::Mythic,     3 }
            };

        case StageType::Boss:
            return {
                { Rarity::Common,     0 },
                { Rarity::Uncommon,   0 },
                { Rarity::Rare,       5 },
                { Rarity::Special,   20 },
                { Rarity::Epic,      35 },
                { Rarity::Legendary, 30 },
                { Rarity::Mythic,    10 }
            };
        }

        return {};
    }

    //--------------------------------------------------------
    // 設計書 8. Fortuneシナジーによるレアリティ補正
    //
    // Fortune 4：D/Cの合計から10ポイントを B/A/S へ再配分
    // Fortune 6：STAGE 3 の SSS 確率 +2ポイント
    //--------------------------------------------------------
    void ApplyFortune(
        std::vector<RarityWeight>& table,
        int fortuneLevel,
        StageType stage
    )
    {
        if (fortuneLevel < 4)
        {
            return;
        }

        int remaining = 10;

        // D から削る
        int take = std::min(table[0].weight, remaining);
        table[0].weight -= take;
        remaining -= take;

        // 足りなければ C から削る
        take = std::min(table[1].weight, remaining);
        table[1].weight -= take;
        remaining -= take;

        const int moved = 10 - remaining;

        // B / A / S へ 3 : 4 : 3 で配分
        const int toRare = moved * 3 / 10;
        const int toSpecial = moved * 4 / 10;
        const int toEpic = moved - toRare - toSpecial;

        table[2].weight += toRare;
        table[3].weight += toSpecial;
        table[4].weight += toEpic;

        if (fortuneLevel >= 6 &&
            stage == StageType::Town &&
            table[4].weight >= 2)
        {
            table[4].weight -= 2;
            table[6].weight += 2;
        }
    }

    //--------------------------------------------------------
    // 最低レアリティ保証
    // Fortune 6「各ステージ最初の宝箱は最低B以上」用
    //--------------------------------------------------------
    void ApplyMinRarity(
        std::vector<RarityWeight>& table,
        Rarity minRarity
    )
    {
        const int floorIndex = static_cast<int>(minRarity);

        if (floorIndex <= 0)
        {
            return;
        }

        int moved = 0;

        for (int i = 0; i < floorIndex; ++i)
        {
            moved += table[i].weight;
            table[i].weight = 0;
        }

        if (moved <= 0)
        {
            return;
        }

        // 保証ライン以上の枠が全部0なら、保証ラインへ全部寄せる
        int aboveTotal = 0;

        for (int i = floorIndex; i < RARITY_COUNT; ++i)
        {
            aboveTotal += table[i].weight;
        }

        if (aboveTotal <= 0)
        {
            table[floorIndex].weight += moved;
            return;
        }

        // 元の比率を保ったまま上位へ再配分
        int distributed = 0;

        for (int i = floorIndex; i < RARITY_COUNT; ++i)
        {
            const int add = moved * table[i].weight / aboveTotal;
            table[i].weight += add;
            distributed += add;
        }

        table[floorIndex].weight += (moved - distributed);
    }

    //--------------------------------------------------------
    // シナジー10種類
    //--------------------------------------------------------
    const std::array<SynergyType, SYNERGY_COUNT> ALL_SYNERGIES =
    {
        SynergyType::Assault,
        SynergyType::Hunter,
        SynergyType::Demolition,
        SynergyType::Guardian,
        SynergyType::Velocity,
        SynergyType::Gunner,
        SynergyType::Executioner,
        SynergyType::Vampire,
        SynergyType::Redeye,
        SynergyType::Fortune
    };

    //--------------------------------------------------------
    // 武器OP候補8種類（設計書 9.）
    //--------------------------------------------------------
    const std::array<RandomOptionType, 8> WEAPON_OPTIONS =
    {
        RandomOptionType::AttackPercent,
        RandomOptionType::CritRate,
        RandomOptionType::CritDamage,
        RandomOptionType::FireRate,
        RandomOptionType::ReloadSpeed,
        RandomOptionType::MagazineSize,
        RandomOptionType::HeadShotDamage,
        RandomOptionType::ArmorPenetration
    };
}

//============================================================

EquipmentGenerator::EquipmentGenerator()
{
    m_rng.seed(
        static_cast<unsigned>(
            std::chrono::high_resolution_clock::now()
            .time_since_epoch().count()
            )
    );
}

int EquipmentGenerator::RandomInt(int minValue, int maxValue)
{
    if (minValue > maxValue)
    {
        std::swap(minValue, maxValue);
    }

    std::uniform_int_distribution<int> dist(minValue, maxValue);
    return dist(m_rng);
}

float EquipmentGenerator::RandomFloat(float minValue, float maxValue)
{
    if (minValue > maxValue)
    {
        std::swap(minValue, maxValue);
    }

    if (minValue == maxValue)
    {
        return minValue;
    }

    std::uniform_real_distribution<float> dist(minValue, maxValue);
    return dist(m_rng);
}

void EquipmentGenerator::SetPreferredSynergies(
    const std::vector<SynergyType>& list,
    int chancePercent
)
{
    m_preferred = list;
    m_preferredChance = std::clamp(chancePercent, 0, 85);
}

void EquipmentGenerator::ClearPreferredSynergies()
{
    m_preferred.clear();
    m_preferredChance = 0;
}

//------------------------------------------------------------
// レアリティ抽選
//------------------------------------------------------------
Rarity EquipmentGenerator::RollRarity(
    StageType stage,
    int fortuneLevel,
    Rarity minRarity
)
{
    auto table = StageTable(stage);

    if (table.empty())
    {
        return Rarity::Common;
    }

    ApplyFortune(table, fortuneLevel, stage);
    ApplyMinRarity(table, minRarity);

    int total = 0;

    for (const auto& entry : table)
    {
        total += entry.weight;
    }

    if (total <= 0)
    {
        return minRarity;
    }

    const int roll = RandomInt(1, total);

    int sum = 0;

    for (const auto& entry : table)
    {
        sum += entry.weight;

        if (roll <= sum)
        {
            return entry.rarity;
        }
    }

    return Rarity::Common;
}

//------------------------------------------------------------
// シナジー個数
// 設計書 3-2. レアリティ別 シナジー数
//------------------------------------------------------------
int EquipmentGenerator::RollSynergyCount(Rarity rarity)
{
    const RarityData& data = RarityTable::Get(rarity);

    if (data.secondSynergyChance <= 0)
    {
        return data.minSynergyCount;
    }

    if (RandomInt(1, 100) <= data.secondSynergyChance)
    {
        return data.maxSynergyCount;
    }

    return data.minSynergyCount;
}

//------------------------------------------------------------
// 銃のシナジー：10種類から完全ランダム
// 設計書 3-1. 重複なし
//------------------------------------------------------------
std::vector<SynergyType> EquipmentGenerator::RollWeaponSynergies(int count)
{
    if (count < 1)
    {
        count = 1;
    }

    std::vector<SynergyType> result;

    //--------------------------------------------------------
    // ステージ目標に合うシナジーを、決めた確率で先に入れる。
    // 外れた場合は今までどおり10種類から選ぶ。
    //--------------------------------------------------------
    if (!m_preferred.empty() && m_preferredChance > 0)
    {
        if (RandomInt(1, 100) <= m_preferredChance)
        {
            const int index = RandomInt(
                0, static_cast<int>(m_preferred.size()) - 1
            );

            result.push_back(m_preferred[index]);
        }
    }

    std::vector<SynergyType> pool(
        ALL_SYNERGIES.begin(),
        ALL_SYNERGIES.end()
    );

    std::shuffle(pool.begin(), pool.end(), m_rng);

    for (SynergyType type : pool)
    {
        if (static_cast<int>(result.size()) >= count)
        {
            break;
        }

        if (std::find(result.begin(), result.end(), type) == result.end())
        {
            result.push_back(type);
        }
    }

    return result;
}

//------------------------------------------------------------
// 防具のシナジー：部位ごとの得意プールを優先
// 設計書 4.
//   1個目 60% は得意プール / 40% は全10種
//   2個目 50% は得意プール / 50% は全10種
//------------------------------------------------------------
std::vector<SynergyType> EquipmentGenerator::RollArmorSynergies(
    const ArmorData& armor,
    int count
)
{
    std::vector<SynergyType> result;

    if (count < 1)
    {
        count = 1;
    }

    // 無限ループ防止
    int guard = 0;

    while (static_cast<int>(result.size()) < count && guard < 200)
    {
        ++guard;

        //----------------------------------------------------
        // まず、ステージ目標に合うものを優先して入れる
        //----------------------------------------------------
        if (result.empty() &&
            !m_preferred.empty() &&
            m_preferredChance > 0 &&
            RandomInt(1, 100) <= m_preferredChance)
        {
            const int index = RandomInt(
                0, static_cast<int>(m_preferred.size()) - 1
            );

            result.push_back(m_preferred[index]);
            continue;
        }

        const int chance = result.empty() ? 60 : 50;

        SynergyType picked;

        if (RandomInt(1, 100) <= chance &&
            !armor.preferredSynergies.empty())
        {
            const int index = RandomInt(
                0,
                static_cast<int>(armor.preferredSynergies.size()) - 1
            );

            picked = armor.preferredSynergies[index];
        }
        else
        {
            const int index = RandomInt(
                0,
                static_cast<int>(ALL_SYNERGIES.size()) - 1
            );

            picked = ALL_SYNERGIES[index];
        }

        // 重複禁止
        if (std::find(result.begin(), result.end(), picked) == result.end())
        {
            result.push_back(picked);
        }
    }

    return result;
}

//------------------------------------------------------------
// 武器ランダムOP
// 設計書 11. 同名OPは重複しない
//------------------------------------------------------------
std::vector<RandomOption> EquipmentGenerator::RollWeaponOptions(
    Rarity rarity,
    int count
)
{
    std::vector<RandomOptionType> pool(
        WEAPON_OPTIONS.begin(),
        WEAPON_OPTIONS.end()
    );

    std::shuffle(pool.begin(), pool.end(), m_rng);

    std::vector<RandomOption> result;

    for (int i = 0; i < count && i < static_cast<int>(pool.size()); ++i)
    {
        const OptionRange range =
            RandomOptionTable::GetWeaponRange(pool[i], rarity);

        RandomOption option;
        option.type = pool[i];
        option.value = RandomFloat(range.minValue, range.maxValue);

        result.push_back(option);
    }

    return result;
}

//------------------------------------------------------------
// 防具ランダムOP
//------------------------------------------------------------
std::vector<RandomOption> EquipmentGenerator::RollArmorOptions(
    const ArmorData& armor,
    Rarity rarity,
    int count
)
{
    std::vector<RandomOptionType> pool = armor.optionPool;

    std::shuffle(pool.begin(), pool.end(), m_rng);

    std::vector<RandomOption> result;

    for (int i = 0; i < count && i < static_cast<int>(pool.size()); ++i)
    {
        const OptionRange range =
            RandomOptionTable::GetArmorRange(armor.slot, pool[i], rarity);

        RandomOption option;
        option.type = pool[i];
        option.value = RandomFloat(range.minValue, range.maxValue);

        result.push_back(option);
    }

    return result;
}

//------------------------------------------------------------
// SSS Mythic固有能力
// 設計書 15. 完成装備例「Blood Bombardment」など
//------------------------------------------------------------
MythicEffectType EquipmentGenerator::RollMythicWeaponEffect()
{
    const auto& list = GetWeaponMythicList();

    return list[RandomInt(0, static_cast<int>(list.size()) - 1)];
}

MythicEffectType EquipmentGenerator::RollMythicArmorEffect()
{
    const auto& list = GetArmorMythicList();

    return list[RandomInt(0, static_cast<int>(list.size()) - 1)];
}

//============================================================
// 生成本体
//============================================================

GeneratedWeapon EquipmentGenerator::GenerateWeapon(
    WeaponType type,
    StageType stage,
    int fortuneLevel,
    Rarity minRarity
)
{
    GeneratedWeapon result;

    const WeaponData* base = EquipmentDatabase::FindWeapon(type);

    if (!base)
    {
        return result;
    }

    result.data = *base;
    result.data.rarity = RollRarity(stage, fortuneLevel, minRarity);

    const RarityData& rarityData = RarityTable::Get(result.data.rarity);

    //--------------------------------------------------------
    // 設計書 5.
    // 武器は「基礎攻撃力」にだけレアリティ倍率を掛ける。
    // Fire Rate や Crit へは掛けない（ランダムOP側で伸ばす）。
    //--------------------------------------------------------
    result.finalAttack = result.data.baseAttack * rarityData.multiplier;

    result.data.randomOptions = RollWeaponOptions(
        result.data.rarity,
        rarityData.randomOptionCount
    );

    result.data.synergies = RollWeaponSynergies(
        RollSynergyCount(result.data.rarity)
    );

    if (result.data.rarity == Rarity::Mythic)
    {
        result.mythicEffect = RollMythicWeaponEffect();
    }

    return result;
}

GeneratedArmor EquipmentGenerator::GenerateArmor(
    ArmorSlot slot,
    StageType stage,
    int fortuneLevel,
    Rarity minRarity
)
{
    GeneratedArmor result;

    const ArmorData* base = EquipmentDatabase::FindArmor(slot);

    if (!base)
    {
        return result;
    }

    result.data = *base;
    result.data.rarity = RollRarity(stage, fortuneLevel, minRarity);

    const RarityData& rarityData = RarityTable::Get(result.data.rarity);

    //--------------------------------------------------------
    // 設計書 5.
    // 防具は「その部位のメインステータス」にだけ倍率を掛ける。
    //--------------------------------------------------------
    result.data.mainStatValue =
        result.data.mainStatBase * rarityData.multiplier;

    result.data.randomOptions = RollArmorOptions(
        result.data,
        result.data.rarity,
        rarityData.randomOptionCount
    );

    result.data.synergies = RollArmorSynergies(
        result.data,
        RollSynergyCount(result.data.rarity)
    );

    if (result.data.rarity == Rarity::Mythic)
    {
        result.mythicEffect = RollMythicArmorEffect();
    }

    return result;
}

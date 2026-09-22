/*--------------------------------------------------------------------

ファイル名：Inventory.cpp
お仕事　　：取得した装備の保持と比較

----------------------------------------------------------------------*/

#include "Inventory.h"
#include "Rarity.h"

#include <algorithm>

void Inventory::Clear()
{
    m_items.clear();
    m_totalPickup = 0;
    m_mythicCount = 0;
    m_bestRarity = Rarity::Common;

    m_knownWeapons.clear();
    m_knownArmors.clear();
}

const EquipmentDrop* Inventory::Get(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_items.size()))
    {
        return nullptr;
    }

    return &m_items[index];
}

bool Inventory::IsKnown(const EquipmentDrop& item) const
{
    if (!item.dropped)
    {
        return true;
    }

    if (item.isWeapon)
    {
        const int id = static_cast<int>(item.weapon.data.type);

        return std::find(m_knownWeapons.begin(), m_knownWeapons.end(), id)
            != m_knownWeapons.end();
    }

    const int id = static_cast<int>(item.armor.data.slot);

    return std::find(m_knownArmors.begin(), m_knownArmors.end(), id)
        != m_knownArmors.end();
}

void Inventory::NotifyPickup(const EquipmentDrop& item)
{
    if (!item.dropped)
    {
        return;
    }

    //--------------------------------------------------------
    // 覚えておいて、次からは NEW を出さない
    //--------------------------------------------------------
    if (item.isWeapon)
    {
        const int id = static_cast<int>(item.weapon.data.type);

        if (std::find(m_knownWeapons.begin(), m_knownWeapons.end(), id)
            == m_knownWeapons.end())
        {
            m_knownWeapons.push_back(id);
        }
    }
    else
    {
        const int id = static_cast<int>(item.armor.data.slot);

        if (std::find(m_knownArmors.begin(), m_knownArmors.end(), id)
            == m_knownArmors.end())
        {
            m_knownArmors.push_back(id);
        }
    }

    ++m_totalPickup;

    if (item.GetRarity() > m_bestRarity)
    {
        m_bestRarity = item.GetRarity();
    }

    if (item.GetRarity() == Rarity::Mythic)
    {
        ++m_mythicCount;
    }
}

bool Inventory::Add(const EquipmentDrop& item)
{
    if (!item.dropped)
    {
        return false;
    }

    //--------------------------------------------------------
    // 満杯なら、一番評価の低いものを捨てて入れ替える
    //--------------------------------------------------------
    if (static_cast<int>(m_items.size()) >= CAPACITY)
    {
        auto weakest = std::min_element(
            m_items.begin(),
            m_items.end(),
            [](const EquipmentDrop& a, const EquipmentDrop& b)
            {
                return EvaluateScore(a) < EvaluateScore(b);
            }
        );

        if (weakest == m_items.end())
        {
            return false;
        }

        if (EvaluateScore(*weakest) >= EvaluateScore(item))
        {
            // 拾ったものの方が弱いので入れない
            return false;
        }

        m_items.erase(weakest);
    }

    m_items.push_back(item);

    return true;
}

bool Inventory::Remove(int index)
{
    if (index < 0 || index >= static_cast<int>(m_items.size()))
    {
        return false;
    }

    m_items.erase(m_items.begin() + index);

    return true;
}

//------------------------------------------------------------
// 装備の強さの目安を数値にする
//
// 厳密な強さはビルド次第なので、
// あくまで「拾うかどうかの参考」程度の値にとどめる。
//------------------------------------------------------------
float Inventory::EvaluateScore(const EquipmentDrop& item)
{
    if (!item.dropped)
    {
        return 0.0f;
    }

    const RarityData& rarity = RarityTable::Get(item.GetRarity());

    // レアリティ倍率が主、OP個数とシナジー個数を加点
    float score = rarity.multiplier * 100.0f;

    if (item.isWeapon)
    {
        score += item.weapon.finalAttack * 0.05f;

        for (const RandomOption& option : item.weapon.data.randomOptions)
        {
            score += option.value * 0.8f;
        }

        score += item.weapon.data.synergies.size() * 25.0f;

        if (item.weapon.mythicEffect != MythicEffectType::None)
        {
            score += 120.0f;
        }
    }
    else
    {
        score += item.armor.data.mainStatValue * 0.5f;

        for (const RandomOption& option : item.armor.data.randomOptions)
        {
            score += option.value * 0.8f;
        }

        score += item.armor.data.synergies.size() * 25.0f;

        if (item.armor.mythicEffect != MythicEffectType::None)
        {
            score += 120.0f;
        }
    }

    return score;
}

//------------------------------------------------------------
// 装備中のものと比べる
//------------------------------------------------------------
CompareResult Inventory::Compare(
    const EquipmentDrop& incoming,
    const PlayerLoadout& loadout
)
{
    if (!incoming.dropped)
    {
        return CompareResult::NoEquipped;
    }

    EquipmentDrop equipped;

    if (incoming.isWeapon)
    {
        const bool isMain =
            (incoming.weapon.data.category == WeaponCategory::Main);

        if (isMain && loadout.HasMainWeapon())
        {
            equipped.dropped = true;
            equipped.isWeapon = true;
            equipped.weapon = loadout.GetMainWeapon();
        }
        else if (!isMain && loadout.HasSubWeapon())
        {
            equipped.dropped = true;
            equipped.isWeapon = true;
            equipped.weapon = loadout.GetSubWeapon();
        }
    }
    else
    {
        const ArmorSlot slot = incoming.armor.data.slot;

        if (loadout.HasArmor(slot))
        {
            equipped.dropped = true;
            equipped.isWeapon = false;
            equipped.armor = loadout.GetArmor(slot);
        }
    }

    if (!equipped.dropped)
    {
        return CompareResult::NoEquipped;
    }

    const float incomingScore = EvaluateScore(incoming);
    const float equippedScore = EvaluateScore(equipped);

    // 5%以内の差は「ほぼ同等」とみなす
    if (incomingScore > equippedScore * 1.05f)
    {
        return CompareResult::Better;
    }

    if (incomingScore < equippedScore * 0.95f)
    {
        return CompareResult::Worse;
    }

    return CompareResult::Similar;
}

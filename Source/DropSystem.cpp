/*--------------------------------------------------------------------

ファイル名：DropSystem.cpp
お仕事　　：敵撃破時のドロップ判定

----------------------------------------------------------------------*/

#include "DropSystem.h"

#include <array>
#include <algorithm>
#include <chrono>

namespace
{
    const std::array<WeaponType, 17> WEAPON_LIST =
    {
        WeaponType::AssaultRifle,
        WeaponType::BurstRifle,
        WeaponType::BattleRifle,
        WeaponType::PumpShotgun,
        WeaponType::AutoShotgun,
        WeaponType::ScoutRifle,
        WeaponType::BoltActionRifle,
        WeaponType::AntiMaterialRifle,
        WeaponType::LightMachineGun,
        WeaponType::HeavyMachineGun,
        WeaponType::CombatPistol,
        WeaponType::AutoPistol,
        WeaponType::HeavyRevolver,
        WeaponType::MagnumRevolver,
        WeaponType::CompactSMG,
        WeaponType::TacticalSMG,
        WeaponType::VampireSMG
    };

    const std::array<ArmorSlot, ARMOR_SLOT_COUNT> ARMOR_LIST =
    {
        ArmorSlot::Head,
        ArmorSlot::Body,
        ArmorSlot::Leg,
        ArmorSlot::Arm,
        ArmorSlot::Boots
    };

    const std::wstring EMPTY_NAME = L"";
}

//============================================================

Rarity EquipmentDrop::GetRarity() const
{
    return isWeapon ? weapon.data.rarity : armor.data.rarity;
}

const std::wstring& EquipmentDrop::GetName() const
{
    if (!dropped)
    {
        return EMPTY_NAME;
    }

    return isWeapon ? weapon.data.name : armor.data.name;
}

//============================================================

DropSystem::DropSystem()
{
    m_rng.seed(
        static_cast<unsigned>(
            std::chrono::high_resolution_clock::now()
            .time_since_epoch().count()
            )
    );
}

//------------------------------------------------------------
// 目標に合うシナジーを優遇する
//
//   基本 30%
//   Fortune 2 → +12% / 4 → +24% / 6 → +36%
//   上限 78%
//------------------------------------------------------------
void DropSystem::SetObjectiveSynergies(
    const std::vector<SynergyType>& list,
    int fortuneLevel
)
{
    if (list.empty())
    {
        m_generator.ClearPreferredSynergies();
        return;
    }

    int chance = 30;

    if (fortuneLevel >= 6)      chance += 36;
    else if (fortuneLevel >= 4) chance += 24;
    else if (fortuneLevel >= 2) chance += 12;

    m_generator.SetPreferredSynergies(list, std::min(chance, 78));
}

void DropSystem::NotifyStageStart()
{
    m_firstChestTaken = false;
}

int DropSystem::RandomInt(int minValue, int maxValue)
{
    if (minValue > maxValue)
    {
        std::swap(minValue, maxValue);
    }

    std::uniform_int_distribution<int> dist(minValue, maxValue);
    return dist(m_rng);
}

//------------------------------------------------------------
// ドロップ率
//------------------------------------------------------------
float DropSystem::GetDropChance(DropSource source, int fortuneLevel)
{
    float chance = 0.0f;

    switch (source)
    {
    case DropSource::NormalEnemy:   chance = 18.0f;  break;
    case DropSource::EliteEnemy:    chance = 55.0f;  break;
    case DropSource::TreasureChest: chance = 100.0f; break;
    case DropSource::Boss:          chance = 100.0f; break;
    }

    // 設計書 8. Fortune によるDrop率上昇
    if (fortuneLevel >= 6)      chance += 18.0f;
    else if (fortuneLevel >= 4) chance += 12.0f;
    else if (fortuneLevel >= 2) chance += 8.0f;

    if (chance > 100.0f)
    {
        chance = 100.0f;
    }

    return chance;
}

WeaponType DropSystem::RollWeaponType()
{
    return WEAPON_LIST[
        RandomInt(0, static_cast<int>(WEAPON_LIST.size()) - 1)
    ];
}

ArmorSlot DropSystem::RollArmorSlot()
{
    return ARMOR_LIST[
        RandomInt(0, static_cast<int>(ARMOR_LIST.size()) - 1)
    ];
}

//------------------------------------------------------------
// 装備1個を作る
//------------------------------------------------------------
EquipmentDrop DropSystem::MakeDrop(
    StageType stage,
    DropSource source,
    int fortuneLevel,
    Rarity minRarity
)
{
    EquipmentDrop drop;

    drop.dropped = true;

    // 武器／防具カテゴリ抽選
    drop.isWeapon = (RandomInt(0, 1) == 0);

    // ボス報酬は専用テーブルを使う
    const StageType rollStage =
        (source == DropSource::Boss) ? StageType::Boss : stage;

    if (drop.isWeapon)
    {
        drop.weapon = m_generator.GenerateWeapon(
            RollWeaponType(),
            rollStage,
            fortuneLevel,
            minRarity
        );
    }
    else
    {
        drop.armor = m_generator.GenerateArmor(
            RollArmorSlot(),
            rollStage,
            fortuneLevel,
            minRarity
        );
    }

    return drop;
}

//------------------------------------------------------------
// 通常のドロップ判定
//------------------------------------------------------------
std::vector<EquipmentDrop> DropSystem::TryDrop(
    StageType stage,
    DropSource source,
    int fortuneLevel
)
{
    std::vector<EquipmentDrop> result;

    const float chance = GetDropChance(source, fortuneLevel);

    // 小数第2位まで見るため 10000 分率で判定
    if (RandomInt(1, 10000) > static_cast<int>(chance * 100.0f))
    {
        return result;
    }

    Rarity minRarity = Rarity::Common;

    //--------------------------------------------------------
    // 設計書 8. Fortune 6
    // 各ステージ最初の宝箱は最低B以上
    //--------------------------------------------------------
    if (fortuneLevel >= 6 &&
        source == DropSource::TreasureChest &&
        !m_firstChestTaken)
    {
        minRarity = Rarity::Rare;
        m_firstChestTaken = true;
    }

    result.push_back(
        MakeDrop(stage, source, fortuneLevel, minRarity)
    );

    //--------------------------------------------------------
    // 設計書 8. Fortune 6
    // Elite撃破時、追加装備1個確定
    //--------------------------------------------------------
    if (fortuneLevel >= 6 && source == DropSource::EliteEnemy)
    {
        result.push_back(
            MakeDrop(stage, source, fortuneLevel, Rarity::Common)
        );
    }

    return result;
}

//------------------------------------------------------------
// 必ず1個出す
//------------------------------------------------------------
EquipmentDrop DropSystem::ForceDrop(
    StageType stage,
    DropSource source,
    int fortuneLevel
)
{
    Rarity minRarity = Rarity::Common;

    if (fortuneLevel >= 6 &&
        source == DropSource::TreasureChest &&
        !m_firstChestTaken)
    {
        minRarity = Rarity::Rare;
        m_firstChestTaken = true;
    }

    return MakeDrop(stage, source, fortuneLevel, minRarity);
}

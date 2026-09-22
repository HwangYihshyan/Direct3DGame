/*--------------------------------------------------------------------

ファイル名：EquipmentGenerator.h
お仕事　　：装備1個を生成する

設計書 14. 装備生成フロー
    武器種／防具部位抽選
      → ステージ別レアリティ抽選
      → レアリティ倍率適用
      → ランダムOP抽選
      → シナジー抽選
      → SSSならMythic固有能力

----------------------------------------------------------------------*/

#ifndef EQUIPMENTGENERATOR_H
#define EQUIPMENTGENERATOR_H

#include <random>
#include <string>
#include <vector>

#include "WeaponData.h"
#include "ArmorData.h"
#include "MythicEffect.h"

struct GeneratedWeapon
{
    WeaponData data;

    // baseAttack × レアリティ倍率
    float finalAttack = 0.0f;

    // SSSのみ固有能力を持つ（設計書 5-2.）
    MythicEffectType mythicEffect = MythicEffectType::None;
};

struct GeneratedArmor
{
    ArmorData data;
    MythicEffectType mythicEffect = MythicEffectType::None;
};

class EquipmentGenerator
{
public:
    EquipmentGenerator();

    GeneratedWeapon GenerateWeapon(
        WeaponType type,
        StageType stage,
        int fortuneLevel = 0,
        Rarity minRarity = Rarity::Common
    );

    GeneratedArmor GenerateArmor(
        ArmorSlot slot,
        StageType stage,
        int fortuneLevel = 0,
        Rarity minRarity = Rarity::Common
    );

    //--------------------------------------------------------
    // 目標に合うシナジーを出やすくする
    //   chancePercent の確率で、この中から選ぶ
    //--------------------------------------------------------
    void SetPreferredSynergies(
        const std::vector<SynergyType>& list,
        int chancePercent
    );

    void ClearPreferredSynergies();

    Rarity RollRarity(
        StageType stage,
        int fortuneLevel = 0,
        Rarity minRarity = Rarity::Common
    );

private:
    std::mt19937 m_rng;

    std::vector<SynergyType> m_preferred;
    int m_preferredChance = 0;

    float RandomFloat(float minValue, float maxValue);
    int RandomInt(int minValue, int maxValue);

    int RollSynergyCount(Rarity rarity);

    std::vector<SynergyType> RollWeaponSynergies(int count);

    std::vector<SynergyType> RollArmorSynergies(
        const ArmorData& armor,
        int count
    );

    std::vector<RandomOption> RollWeaponOptions(
        Rarity rarity,
        int count
    );

    std::vector<RandomOption> RollArmorOptions(
        const ArmorData& armor,
        Rarity rarity,
        int count
    );

    MythicEffectType RollMythicWeaponEffect();
    MythicEffectType RollMythicArmorEffect();
};

#endif

/*--------------------------------------------------------------------

ファイル名：DropSystem.h
お仕事　　：敵撃破時のドロップ判定

設計書 14. 装備生成フロー
    敵撃破 → 装備Drop判定 → 武器／防具カテゴリ抽選 → …

設計書 8. Fortuneシナジーによるレアリティ補正
    Fortune 2：Drop率 +8%
    Fortune 4：Drop率 +12%
    Fortune 6：Drop率 +18% / Elite撃破時 追加装備1個確定
                各ステージ最初の宝箱は最低B以上

----------------------------------------------------------------------*/

#ifndef DROPSYSTEM_H
#define DROPSYSTEM_H

#include <random>
#include <vector>

#include "EquipmentGenerator.h"

struct EquipmentDrop
{
    bool dropped = false;
    bool isWeapon = true;

    GeneratedWeapon weapon;
    GeneratedArmor armor;

    Rarity GetRarity() const;
    const std::wstring& GetName() const;
};

class DropSystem
{
public:
    DropSystem();

    //--------------------------------------------------------
    // ステージ目標に合うシナジーを出やすくする
    //   Fortune の段階が高いほど確率が上がる
    //--------------------------------------------------------
    void SetObjectiveSynergies(
        const std::vector<SynergyType>& list,
        int fortuneLevel
    );

    // ステージが変わったら呼ぶ（宝箱保証をリセット）
    void NotifyStageStart();

    // 1回のドロップ判定。Fortune 6 では2個返ることがある
    std::vector<EquipmentDrop> TryDrop(
        StageType stage,
        DropSource source,
        int fortuneLevel = 0
    );

    // 必ず1個出す（宝箱・ボス報酬用）
    EquipmentDrop ForceDrop(
        StageType stage,
        DropSource source,
        int fortuneLevel = 0
    );

private:
    std::mt19937 m_rng;
    EquipmentGenerator m_generator;

    bool m_firstChestTaken = false;

    int RandomInt(int minValue, int maxValue);

    float GetDropChance(DropSource source, int fortuneLevel);

    WeaponType RollWeaponType();
    ArmorSlot RollArmorSlot();

    EquipmentDrop MakeDrop(
        StageType stage,
        DropSource source,
        int fortuneLevel,
        Rarity minRarity
    );
};

#endif

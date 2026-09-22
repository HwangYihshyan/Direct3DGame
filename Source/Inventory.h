/*--------------------------------------------------------------------

ファイル名：Inventory.h
お仕事　　：取得した装備の保持

設計書 Ver.3 19-3.4
    最低要件：Main Weapon 1 / Sub Weapon 1 / Armor 5部位
    将来：所持品リスト / 装備比較 / 売却 / 廃棄

装備中のものは PlayerLoadout が持つ。
Inventory は「拾ったが装備していないもの」を預かる。

----------------------------------------------------------------------*/

#ifndef INVENTORY_H
#define INVENTORY_H

#include <vector>

#include "DropSystem.h"
#include "PlayerLoadout.h"

// 拾った装備が装備中のものより強いかの目安
enum class CompareResult
{
    Better,
    Similar,
    Worse,
    NoEquipped
};

class Inventory
{
public:
    void Clear();

    // 所持品へ追加する。上限を超えたら一番弱いものを捨てる
    bool Add(const EquipmentDrop& item);

    bool Remove(int index);

    int GetCount() const { return static_cast<int>(m_items.size()); }
    int GetCapacity() const { return CAPACITY; }

    const std::vector<EquipmentDrop>& GetItems() const { return m_items; }

    const EquipmentDrop* Get(int index) const;

    // 統計（リザルト用）
    int GetTotalPickup() const { return m_totalPickup; }
    Rarity GetBestRarity() const { return m_bestRarity; }
    int GetMythicCount() const { return m_mythicCount; }

    void NotifyPickup(const EquipmentDrop& item);

    //--------------------------------------------------------
    // 一度でも取ったことがあるか
    //   武器は種類ごと、防具は部位ごとに覚える
    //--------------------------------------------------------
    bool IsKnown(const EquipmentDrop& item) const;

    //--------------------------------------------------------
    // 装備中との比較
    //--------------------------------------------------------
    static CompareResult Compare(
        const EquipmentDrop& incoming,
        const PlayerLoadout& loadout
    );

    // 比較に使うスコア（数値が大きいほど強い目安）
    static float EvaluateScore(const EquipmentDrop& item);

private:
    static constexpr int CAPACITY = 12;

    std::vector<EquipmentDrop> m_items;

    int m_totalPickup = 0;
    int m_mythicCount = 0;
    Rarity m_bestRarity = Rarity::Common;

    std::vector<int> m_knownWeapons;
    std::vector<int> m_knownArmors;
};

#endif

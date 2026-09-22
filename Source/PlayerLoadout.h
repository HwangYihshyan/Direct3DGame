/*--------------------------------------------------------------------

ファイル名：PlayerLoadout.h
お仕事　　：プレイヤーの装備スロットと、最終ステータスの合算

スロット：
    MAIN 武器 ×1
    SUB  武器 ×1
    HEAD / BODY / LEG / ARM / BOOTS ×各1

積み上げ順：
    ① 基礎値
    ② 防具メインステータス（レアリティ倍率適用済み）
    ③ 防具ランダムOP
    ④ シナジー
    ⑤ 武器ランダムOP（その武器を構えている間だけ）

----------------------------------------------------------------------*/

#ifndef PLAYERLOADOUT_H
#define PLAYERLOADOUT_H

#include <vector>

#include "EquipmentGenerator.h"
#include "PlayerStats.h"
#include "SynergySystem.h"
#include "MythicEffect.h"
#include "PlayerCharacter.h"

//------------------------------------------------------------
// 実際に撃つときに使う武器性能
//------------------------------------------------------------
struct EffectiveWeapon
{
    bool valid = false;

    WeaponType type = WeaponType::AssaultRifle;
    std::wstring name;
    Rarity rarity = Rarity::Common;
    FireMode fireMode = FireMode::FullAuto;

    // 1発（散弾なら1ペレット）のダメージ
    float attack = 0.0f;

    float shotInterval = 0.1f;   // 秒
    int magazineSize = 30;
    int reserveMax = 240;
    float reloadTime = 2.0f;

    float critRate = 0.05f;
    float critDamage = 0.5f;
    float headShotMultiplier = 2.0f;
    float headShotDamageBonus = 0.0f;
    float armorPenetration = 0.0f;

    int pelletCount = 1;
    float spreadDegree = 2.0f;
    float bulletSpeed = 1500.0f;
    float bulletRange = 1400.0f;
    int pierceCount = 0;

    float moveSpeedScale = 1.0f;
    float recoilKick = 1.0f;

    // 命中時に敵を押し戻す距離（px・1ペレットぶん）
    float knockBack = 0.0f;

    float falloffStart = 0.0f;
    float falloffEnd = 0.0f;
    float falloffMinScale = 1.0f;

    // 距離による威力減衰
    float GetFalloffScale(float distance) const;
};

//------------------------------------------------------------
// 装備スロット
//------------------------------------------------------------
class PlayerLoadout
{
public:
    PlayerLoadout();

    void Clear();

    // 操作キャラクターを設定する（基礎ステータスが変わる）
    void SetCharacter(PlayerType type);

    PlayerType GetCharacterType() const { return m_characterType; }
    const PlayerCharacterData& GetCharacter() const;

    // 初期装備を配る
    void GiveStarterGear();

    //--------------------------------------------------------
    // 装備
    //--------------------------------------------------------
    void EquipWeapon(const GeneratedWeapon& weapon);
    void EquipArmor(const GeneratedArmor& armor);

    bool HasMainWeapon() const { return m_hasMain; }
    bool HasSubWeapon() const { return m_hasSub; }
    bool HasArmor(ArmorSlot slot) const;

    const GeneratedWeapon& GetMainWeapon() const { return m_mainWeapon; }
    const GeneratedWeapon& GetSubWeapon() const { return m_subWeapon; }
    const GeneratedArmor& GetArmor(ArmorSlot slot) const;

    //--------------------------------------------------------
    // ステータス
    //--------------------------------------------------------
    void Recalculate();

    const PlayerStats& GetStats() const { return m_stats; }

    SynergySystem& GetSynergy() { return m_synergy; }
    const SynergySystem& GetSynergy() const { return m_synergy; }

    // 装備中の全シナジーを1本のリストにする
    std::vector<SynergyType> CollectSynergies() const;

    // 装備中のSSS固有能力を集める
    std::vector<MythicEffectType> CollectMythicEffects() const;

    MythicRuntime& GetMythic() { return m_mythic; }
    const MythicRuntime& GetMythic() const { return m_mythic; }

    //--------------------------------------------------------
    // 実効武器性能
    //--------------------------------------------------------
    EffectiveWeapon BuildEffectiveWeapon(bool useMain) const;

private:
    void AccumulateArmor(const GeneratedArmor& armor);
    void ApplyOption(const RandomOption& option, bool fromArmor);

private:
    GeneratedWeapon m_mainWeapon;
    GeneratedWeapon m_subWeapon;

    bool m_hasMain = false;
    bool m_hasSub = false;

    GeneratedArmor m_armors[ARMOR_SLOT_COUNT];
    bool m_hasArmor[ARMOR_SLOT_COUNT] = {};

    PlayerStats m_stats;
    SynergySystem m_synergy;
    MythicRuntime m_mythic;

    PlayerType m_characterType = PlayerType::Player1;

    // 割合系の一時集計
    float m_maxHpPercent = 0.0f;
    float m_defensePercent = 0.0f;
};

#endif

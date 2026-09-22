/*--------------------------------------------------------------------

ファイル名：PlayerLoadout.cpp
お仕事　　：装備スロット管理とステータス合算

----------------------------------------------------------------------*/

#include "PlayerLoadout.h"
#include "EquipmentDatabase.h"
#include "Rarity.h"

#include <algorithm>
#include <cmath>

namespace
{
    // 防具のメインステータス／ランダムOPは % 表記なので 100 で割る
    constexpr float PERCENT = 0.01f;

    const GeneratedArmor EMPTY_ARMOR{};
}

//============================================================
// EffectiveWeapon
//============================================================

float EffectiveWeapon::GetFalloffScale(float distance) const
{
    if (falloffEnd <= falloffStart)
    {
        return 1.0f;
    }

    if (distance <= falloffStart)
    {
        return 1.0f;
    }

    if (distance >= falloffEnd)
    {
        return falloffMinScale;
    }

    const float t =
        (distance - falloffStart) / (falloffEnd - falloffStart);

    return 1.0f + (falloffMinScale - 1.0f) * t;
}

//============================================================
// PlayerLoadout
//============================================================

PlayerLoadout::PlayerLoadout()
{
    Clear();
}

void PlayerLoadout::Clear()
{
    m_hasMain = false;
    m_hasSub = false;

    m_mainWeapon = GeneratedWeapon();
    m_subWeapon = GeneratedWeapon();

    for (int i = 0; i < ARMOR_SLOT_COUNT; ++i)
    {
        m_hasArmor[i] = false;
        m_armors[i] = GeneratedArmor();
    }

    m_stats.Reset();
    m_synergy.Clear();
    m_synergy.ResetRuntime();
    m_mythic.SetEquipped({});
    m_mythic.Reset();

    m_maxHpPercent = 0.0f;
    m_defensePercent = 0.0f;
}

//------------------------------------------------------------
// 初期装備
//
// D（Common）の Assault Rifle と Combat Pistol を配る。
// 初期シナジーはキャラクターごとに変える。
//
//   VANGUARD  Guardian ＋ Redeye
//             硬さと弱点狙いで、離れて撃ち合う形にする
//   MARKSMAN  Assault ＋ Gunner
//             手数で押す形にする
//------------------------------------------------------------
void PlayerLoadout::GiveStarterGear()
{
    const WeaponData* mainBase =
        EquipmentDatabase::FindWeapon(WeaponType::AssaultRifle);

    const WeaponData* subBase =
        EquipmentDatabase::FindWeapon(WeaponType::CombatPistol);

    if (mainBase)
    {
        GeneratedWeapon w;
        w.data = *mainBase;
        w.data.rarity = Rarity::Common;
        w.data.synergies =
        {
            (m_characterType == PlayerType::Player1)
                ? SynergyType::Guardian
                : SynergyType::Assault
        };
        w.finalAttack =
            w.data.baseAttack * RarityTable::Get(Rarity::Common).multiplier;

        m_mainWeapon = w;
        m_hasMain = true;
    }

    if (subBase)
    {
        GeneratedWeapon w;
        w.data = *subBase;
        w.data.rarity = Rarity::Common;
        w.data.synergies =
        {
            (m_characterType == PlayerType::Player1)
                ? SynergyType::Redeye
                : SynergyType::Gunner
        };
        w.finalAttack =
            w.data.baseAttack * RarityTable::Get(Rarity::Common).multiplier;

        m_subWeapon = w;
        m_hasSub = true;
    }

    Recalculate();
}

//------------------------------------------------------------
// 装備する
//------------------------------------------------------------
void PlayerLoadout::EquipWeapon(const GeneratedWeapon& weapon)
{
    if (weapon.data.category == WeaponCategory::Main)
    {
        m_mainWeapon = weapon;
        m_hasMain = true;
    }
    else
    {
        m_subWeapon = weapon;
        m_hasSub = true;
    }

    Recalculate();
}

void PlayerLoadout::EquipArmor(const GeneratedArmor& armor)
{
    const int index = static_cast<int>(armor.data.slot);

    if (index < 0 || index >= ARMOR_SLOT_COUNT)
    {
        return;
    }

    m_armors[index] = armor;
    m_hasArmor[index] = true;

    Recalculate();
}

bool PlayerLoadout::HasArmor(ArmorSlot slot) const
{
    const int index = static_cast<int>(slot);

    if (index < 0 || index >= ARMOR_SLOT_COUNT)
    {
        return false;
    }

    return m_hasArmor[index];
}

const GeneratedArmor& PlayerLoadout::GetArmor(ArmorSlot slot) const
{
    const int index = static_cast<int>(slot);

    if (index < 0 || index >= ARMOR_SLOT_COUNT)
    {
        return EMPTY_ARMOR;
    }

    return m_armors[index];
}

//------------------------------------------------------------
// 装備中の全シナジーを集める
//------------------------------------------------------------
std::vector<SynergyType> PlayerLoadout::CollectSynergies() const
{
    std::vector<SynergyType> result;

    if (m_hasMain)
    {
        result.insert(
            result.end(),
            m_mainWeapon.data.synergies.begin(),
            m_mainWeapon.data.synergies.end()
        );
    }

    if (m_hasSub)
    {
        result.insert(
            result.end(),
            m_subWeapon.data.synergies.begin(),
            m_subWeapon.data.synergies.end()
        );
    }

    for (int i = 0; i < ARMOR_SLOT_COUNT; ++i)
    {
        if (!m_hasArmor[i])
        {
            continue;
        }

        result.insert(
            result.end(),
            m_armors[i].data.synergies.begin(),
            m_armors[i].data.synergies.end()
        );
    }

    return result;
}

//------------------------------------------------------------
// 装備中のSSS固有能力を集める
// 設計書 5-2.「SSSのみ固有能力を持つ」
//------------------------------------------------------------
std::vector<MythicEffectType> PlayerLoadout::CollectMythicEffects() const
{
    std::vector<MythicEffectType> result;

    if (m_hasMain && m_mainWeapon.mythicEffect != MythicEffectType::None)
    {
        result.push_back(m_mainWeapon.mythicEffect);
    }

    if (m_hasSub && m_subWeapon.mythicEffect != MythicEffectType::None)
    {
        result.push_back(m_subWeapon.mythicEffect);
    }

    for (int i = 0; i < ARMOR_SLOT_COUNT; ++i)
    {
        if (!m_hasArmor[i])
        {
            continue;
        }

        if (m_armors[i].mythicEffect != MythicEffectType::None)
        {
            result.push_back(m_armors[i].mythicEffect);
        }
    }

    return result;
}


//------------------------------------------------------------
// ランダムOPを1個ぶん反映する
//------------------------------------------------------------
void PlayerLoadout::ApplyOption(
    const RandomOption& option,
    bool fromArmor
)
{
    const float v = option.value * PERCENT;

    switch (option.type)
    {
    case RandomOptionType::AttackPercent:
        m_stats.attackPercent += v;
        break;

    case RandomOptionType::CritRate:
        m_stats.critRate += v;
        break;

    case RandomOptionType::CritDamage:
        m_stats.critDamage += v;
        break;

    case RandomOptionType::FireRate:
        m_stats.fireRatePercent += v;
        break;

    case RandomOptionType::ReloadSpeed:
        m_stats.reloadSpeedPercent += v;
        break;

    case RandomOptionType::MagazineSize:
        m_stats.magazinePercent += v;
        break;

    case RandomOptionType::HeadShotDamage:
        m_stats.headShotDamagePercent += v;
        break;

    case RandomOptionType::ArmorPenetration:
        m_stats.armorPenetration += v;
        break;

    case RandomOptionType::MaxHP:
        m_maxHpPercent += v;
        break;

    case RandomOptionType::Defense:
        m_defensePercent += v;
        break;

    case RandomOptionType::DamageReduction:
        m_stats.damageReduction += v;
        break;

    case RandomOptionType::Shield:
        m_stats.shieldPercent += v;
        break;

    case RandomOptionType::ReserveAmmo:
        m_stats.reserveAmmoPercent += v;
        break;

    case RandomOptionType::AmmoDrop:
        m_stats.ammoDropPercent += v;
        break;

    case RandomOptionType::RecoilControl:
        m_stats.recoilControl += v;
        break;

    case RandomOptionType::WeaponHandling:
        m_stats.weaponHandling += v;
        break;

    case RandomOptionType::MoveSpeed:
        m_stats.moveSpeedPercent += v;
        break;

    case RandomOptionType::Dodge:
        m_stats.dodge += v;
        break;

    case RandomOptionType::DashCooldown:
        m_stats.dashCooldownReduction += v;
        break;

    case RandomOptionType::DashDistance:
        m_stats.dashDistancePercent += v;
        break;

    case RandomOptionType::DetectionRange:
        m_stats.detectionPercent += v;
        break;
    }

    (void)fromArmor;
}

//------------------------------------------------------------
// 防具1個ぶんを合算
//------------------------------------------------------------
void PlayerLoadout::AccumulateArmor(const GeneratedArmor& armor)
{
    // 素の防御力（倍率なし）
    m_stats.defense += armor.data.baseDefense;

    //--------------------------------------------------------
    // メインステータス（レアリティ倍率適用済み）
    //--------------------------------------------------------
    const float mainValue = armor.data.mainStatValue;

    switch (armor.data.mainStatType)
    {
    case RandomOptionType::MaxHP:
        // BODY のみ実HP加算
        m_stats.maxHP += mainValue;
        break;

    case RandomOptionType::CritRate:
        m_stats.critRate += mainValue * PERCENT;
        break;

    case RandomOptionType::ReserveAmmo:
        m_stats.reserveAmmoPercent += mainValue * PERCENT;
        break;

    case RandomOptionType::AttackPercent:
        m_stats.attackPercent += mainValue * PERCENT;
        break;

    case RandomOptionType::MoveSpeed:
        m_stats.moveSpeedPercent += mainValue * PERCENT;
        break;

    default:
        break;
    }

    //--------------------------------------------------------
    // ランダムOP
    //--------------------------------------------------------
    for (const RandomOption& option : armor.data.randomOptions)
    {
        ApplyOption(option, true);
    }
}

//------------------------------------------------------------
// ステータス再計算
//------------------------------------------------------------
void PlayerLoadout::SetCharacter(PlayerType type)
{
    m_characterType = type;

    Recalculate();
}

const PlayerCharacterData& PlayerLoadout::GetCharacter() const
{
    return GetPlayerCharacter(m_characterType);
}

void PlayerLoadout::Recalculate()
{
    m_stats.Reset();

    //--------------------------------------------------------
    // ① 基礎値はキャラクターごとに違う（設計書 19-7.6）
    //--------------------------------------------------------
    const PlayerCharacterData& character = GetCharacter();

    m_stats.maxHP = character.baseHP;
    m_stats.defense = character.baseDefense;
    m_stats.critRate = character.baseCritRate;
    m_stats.attackPercent += character.baseAttackPercent;
    m_stats.moveSpeedPercent += character.baseMoveSpeedPercent;

    m_maxHpPercent = 0.0f;
    m_defensePercent = 0.0f;

    //--------------------------------------------------------
    // ② ③ 防具
    //--------------------------------------------------------
    for (int i = 0; i < ARMOR_SLOT_COUNT; ++i)
    {
        if (m_hasArmor[i])
        {
            AccumulateArmor(m_armors[i]);
        }
    }

    // 割合系を最後に掛ける
    m_stats.maxHP *= (1.0f + m_maxHpPercent);
    m_stats.defense *= (1.0f + m_defensePercent);

    //--------------------------------------------------------
    // ④ シナジー
    //--------------------------------------------------------
    m_synergy.CountSynergies(CollectSynergies());
    m_synergy.ApplyPassiveEffects(m_stats);

    //--------------------------------------------------------
    // ⑤ SSS固有能力
    //--------------------------------------------------------
    m_mythic.SetEquipped(CollectMythicEffects());

    //--------------------------------------------------------
    // 上限クランプ
    //--------------------------------------------------------
    m_stats.Clamp();
}

//------------------------------------------------------------
// 実効武器性能を組み立てる
//------------------------------------------------------------
EffectiveWeapon PlayerLoadout::BuildEffectiveWeapon(bool useMain) const
{
    EffectiveWeapon result;

    const bool has = useMain ? m_hasMain : m_hasSub;

    if (!has)
    {
        return result;
    }

    const GeneratedWeapon& source =
        useMain ? m_mainWeapon : m_subWeapon;

    const WeaponData& base = source.data;

    result.valid = true;
    result.type = base.type;
    result.name = base.name;
    result.rarity = base.rarity;
    result.fireMode = base.fireMode;

    //--------------------------------------------------------
    // 武器自身のランダムOPを取り出す
    //--------------------------------------------------------
    float weaponAttackPercent = 0.0f;
    float weaponCritRate = 0.0f;
    float weaponCritDamage = 0.0f;
    float weaponFireRate = 0.0f;
    float weaponReload = 0.0f;
    float weaponMagazine = 0.0f;
    float weaponHeadShot = 0.0f;
    float weaponArmorPen = 0.0f;

    for (const RandomOption& option : base.randomOptions)
    {
        const float v = option.value * PERCENT;

        switch (option.type)
        {
        case RandomOptionType::AttackPercent:    weaponAttackPercent += v; break;
        case RandomOptionType::CritRate:         weaponCritRate += v; break;
        case RandomOptionType::CritDamage:       weaponCritDamage += v; break;
        case RandomOptionType::FireRate:         weaponFireRate += v; break;
        case RandomOptionType::ReloadSpeed:      weaponReload += v; break;
        case RandomOptionType::MagazineSize:     weaponMagazine += v; break;
        case RandomOptionType::HeadShotDamage:   weaponHeadShot += v; break;
        case RandomOptionType::ArmorPenetration: weaponArmorPen += v; break;
        default: break;
        }
    }

    //--------------------------------------------------------
    // 攻撃力
    // finalAttack は「基礎攻撃力 × レアリティ倍率」まで済んでいる
    //--------------------------------------------------------
    const float attackMultiplier =
        1.0f + m_stats.attackPercent + weaponAttackPercent;

    result.attack = source.finalAttack * attackMultiplier;

    //--------------------------------------------------------
    // 連射速度
    //--------------------------------------------------------
    const float fireRateMultiplier =
        1.0f + m_stats.fireRatePercent + weaponFireRate;

    float rpm = base.roundsPerMinute * fireRateMultiplier;

    if (rpm < 10.0f)
    {
        rpm = 10.0f;
    }

    result.shotInterval = 60.0f / rpm;

    //--------------------------------------------------------
    // マガジン／リザーブ
    //--------------------------------------------------------
    const float magMultiplier =
        1.0f + m_stats.magazinePercent + weaponMagazine;

    result.magazineSize = static_cast<int>(
        std::floor(base.magazineSize * magMultiplier + 0.5f)
        );

    if (result.magazineSize < 1)
    {
        result.magazineSize = 1;
    }

    const float reserveMultiplier =
        1.0f + m_stats.reserveAmmoPercent;

    result.reserveMax = static_cast<int>(
        std::floor(base.reserveAmmo * reserveMultiplier + 0.5f)
        );

    //--------------------------------------------------------
    // リロード
    //--------------------------------------------------------
    const float reloadMultiplier =
        1.0f + m_stats.reloadSpeedPercent + weaponReload;

    result.reloadTime = base.reloadTime / std::max(reloadMultiplier, 0.2f);

    //--------------------------------------------------------
    // Crit / HeadShot / 貫通
    //--------------------------------------------------------
    result.critRate =
        std::min(m_stats.critRate + weaponCritRate, 1.0f);

    result.critDamage =
        m_stats.critDamage + weaponCritDamage;

    result.headShotMultiplier = base.headShotMultiplier;

    result.headShotDamageBonus =
        m_stats.headShotDamagePercent + weaponHeadShot;

    result.armorPenetration =
        std::min(m_stats.armorPenetration + weaponArmorPen, 0.90f);

    //--------------------------------------------------------
    // 弾道
    //--------------------------------------------------------
    result.pelletCount = base.pelletCount;

    // Recoil Control / Weapon Handling で拡散が縮む
    const float spreadReduction =
        std::min(m_stats.recoilControl + m_stats.weaponHandling * 0.5f, 0.70f);

    result.spreadDegree = base.spreadDegree * (1.0f - spreadReduction);

    result.bulletSpeed = base.bulletSpeed;
    result.bulletRange = base.bulletRange;
    result.pierceCount = base.pierceCount;

    result.moveSpeedScale = base.moveSpeedScale;
    result.recoilKick = base.recoilKick * (1.0f - spreadReduction);

    result.knockBack = base.knockBack;

    result.falloffStart = base.falloffStart;
    result.falloffEnd = base.falloffEnd;
    result.falloffMinScale = base.falloffMinScale;

    return result;
}

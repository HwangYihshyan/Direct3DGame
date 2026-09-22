/*--------------------------------------------------------------------

ファイル名：MythicEffect.cpp
お仕事　　：SSS（Mythic）装備の固有能力

----------------------------------------------------------------------*/

#include "MythicEffect.h"

#include <algorithm>
#include <random>

namespace
{
    std::mt19937& Rng()
    {
        static std::mt19937 rng{ std::random_device{}() };
        return rng;
    }

    bool RollPercent(int percent)
    {
        std::uniform_int_distribution<int> dist(1, 100);
        return dist(Rng()) <= percent;
    }

    // EndlessMagazine の効果時間
    constexpr float ENDLESS_TIME = 3.0f;

    // OverclockCore の効果時間と必要命中数
    constexpr float OVERCLOCK_TIME = 5.0f;
    constexpr int OVERCLOCK_HITS = 20;

    // DeadeyeProtocol の効果時間
    constexpr float DEADEYE_TIME = 2.0f;

    // EmergencyShield の間隔
    constexpr float SHIELD_INTERVAL = 15.0f;

    // PerfectChamber は8発ごと
    constexpr int PERFECT_CHAMBER_SHOTS = 8;

    const std::array<MythicEffectType, MYTHIC_WEAPON_COUNT> WEAPON_LIST =
    {
        MythicEffectType::BloodBombardment,
        MythicEffectType::PerfectChamber,
        MythicEffectType::EndlessMagazine,
        MythicEffectType::DeadeyeProtocol,
        MythicEffectType::OverclockCore
    };

    const std::array<MythicEffectType, MYTHIC_ARMOR_COUNT> ARMOR_LIST =
    {
        MythicEffectType::LastStand,
        MythicEffectType::EmergencyShield,
        MythicEffectType::CombatFlow,
        MythicEffectType::AmmoFabricator,
        MythicEffectType::FortuneArmor
    };
}

//============================================================
// 名前と説明
//============================================================

const wchar_t* GetMythicName(MythicEffectType type)
{
    switch (type)
    {
    case MythicEffectType::BloodBombardment: return L"血の砲撃";
    case MythicEffectType::PerfectChamber:   return L"完全装填";
    case MythicEffectType::EndlessMagazine:  return L"無限弾倉";
    case MythicEffectType::DeadeyeProtocol:  return L"精密射撃";
    case MythicEffectType::OverclockCore:    return L"加速機関";
    case MythicEffectType::LastStand:        return L"背水の陣";
    case MythicEffectType::EmergencyShield:  return L"緊急シールド";
    case MythicEffectType::CombatFlow:       return L"戦闘の流れ";
    case MythicEffectType::AmmoFabricator:   return L"弾薬生成";
    case MythicEffectType::FortuneArmor:     return L"幸運の護り";
    case MythicEffectType::None:             break;
    }

    return L"";
}

const wchar_t* GetMythicDescription(MythicEffectType type)
{
    switch (type)
    {
    case MythicEffectType::BloodBombardment:
        return L"敵撃破時20%で武器ダメージ150%の爆発";

    case MythicEffectType::PerfectChamber:
        return L"8発目のダメージ +100%";

    case MythicEffectType::EndlessMagazine:
        return L"リロード直後3秒間、弾薬を消費しない";

    case MythicEffectType::DeadeyeProtocol:
        return L"ヘッドショット時2秒間 クリティカル率 +30%";

    case MythicEffectType::OverclockCore:
        return L"20回命中で5秒間 連射速度 +35%";

    case MythicEffectType::LastStand:
        return L"HP30%以下で 防御力 +35%";

    case MythicEffectType::EmergencyShield:
        return L"15秒ごとに最大HP15%のシールド";

    case MythicEffectType::CombatFlow:
        return L"回避成功でダッシュ再使用時間を30%回復";

    case MythicEffectType::AmmoFabricator:
        return L"敵撃破時10%で弾薬を補充";

    case MythicEffectType::FortuneArmor:
        return L"エリート撃破時15%で追加装備";

    case MythicEffectType::None:
        break;
    }

    return L"";
}

bool IsWeaponMythic(MythicEffectType type)
{
    return std::find(WEAPON_LIST.begin(), WEAPON_LIST.end(), type)
        != WEAPON_LIST.end();
}

const std::array<MythicEffectType, MYTHIC_WEAPON_COUNT>& GetWeaponMythicList()
{
    return WEAPON_LIST;
}

const std::array<MythicEffectType, MYTHIC_ARMOR_COUNT>& GetArmorMythicList()
{
    return ARMOR_LIST;
}

//============================================================
// MythicRuntime
//============================================================

void MythicRuntime::SetEquipped(
    const std::vector<MythicEffectType>& equipped
)
{
    m_equipped.clear();

    // 同じ能力を2個持っても効果は重複しない
    for (MythicEffectType type : equipped)
    {
        if (type == MythicEffectType::None)
        {
            continue;
        }

        if (!Has(type))
        {
            m_equipped.push_back(type);
        }
    }
}

bool MythicRuntime::Has(MythicEffectType type) const
{
    return std::find(m_equipped.begin(), m_equipped.end(), type)
        != m_equipped.end();
}

bool MythicRuntime::HasAny() const
{
    return !m_equipped.empty();
}

void MythicRuntime::Reset()
{
    m_shotCount = 0;
    m_endlessTimer = 0.0f;
    m_hitCount = 0;
    m_overclockTimer = 0.0f;
    m_deadeyeTimer = 0.0f;
    m_shieldTimer = 0.0f;
    m_pendingShield = 0.0f;
}

void MythicRuntime::Update(float deltaTime)
{
    const auto countDown = [deltaTime](float& timer)
        {
            if (timer > 0.0f)
            {
                timer -= deltaTime;

                if (timer < 0.0f)
                {
                    timer = 0.0f;
                }
            }
        };

    countDown(m_endlessTimer);
    countDown(m_overclockTimer);
    countDown(m_deadeyeTimer);

    //--------------------------------------------------------
    // Emergency Shield：15秒ごと
    //--------------------------------------------------------
    if (Has(MythicEffectType::EmergencyShield))
    {
        m_shieldTimer += deltaTime;

        if (m_shieldTimer >= SHIELD_INTERVAL)
        {
            m_shieldTimer -= SHIELD_INTERVAL;
            m_pendingShield += 1.0f;   // 付与回数として数える
        }
    }
    else
    {
        m_shieldTimer = 0.0f;
    }
}

//------------------------------------------------------------
// Perfect Chamber：8発目のDamage +100%
//------------------------------------------------------------
float MythicRuntime::OnShotFired()
{
    if (!Has(MythicEffectType::PerfectChamber))
    {
        return 1.0f;
    }

    ++m_shotCount;

    if (m_shotCount >= PERFECT_CHAMBER_SHOTS)
    {
        m_shotCount = 0;
        return 2.0f;
    }

    return 1.0f;
}

//------------------------------------------------------------
// Endless Magazine：Reload直後3秒間は弾薬を消費しない
//------------------------------------------------------------
void MythicRuntime::OnReloadFinished()
{
    if (Has(MythicEffectType::EndlessMagazine))
    {
        m_endlessTimer = ENDLESS_TIME;
    }
}

bool MythicRuntime::IsAmmoFree() const
{
    return m_endlessTimer > 0.0f;
}

//------------------------------------------------------------
// Overclock Core：20回命中で5秒間 Fire Rate +35%
//------------------------------------------------------------
void MythicRuntime::OnWeaponHit()
{
    if (!Has(MythicEffectType::OverclockCore))
    {
        return;
    }

    ++m_hitCount;

    if (m_hitCount >= OVERCLOCK_HITS)
    {
        m_hitCount = 0;
        m_overclockTimer = OVERCLOCK_TIME;
    }
}

float MythicRuntime::GetFireRateBonus() const
{
    return (m_overclockTimer > 0.0f) ? 0.35f : 0.0f;
}

//------------------------------------------------------------
// Deadeye Protocol：HeadShot時2秒間 Crit Rate +30%
//------------------------------------------------------------
void MythicRuntime::OnHeadShot()
{
    if (Has(MythicEffectType::DeadeyeProtocol))
    {
        m_deadeyeTimer = DEADEYE_TIME;
    }
}

float MythicRuntime::GetCritRateBonus() const
{
    return (m_deadeyeTimer > 0.0f) ? 0.30f : 0.0f;
}

//------------------------------------------------------------
// Last Stand：HP30%以下で Defense +35%
//------------------------------------------------------------
float MythicRuntime::GetDefenseMultiplier(float hpRatio) const
{
    if (!Has(MythicEffectType::LastStand))
    {
        return 1.0f;
    }

    return (hpRatio <= 0.30f) ? 1.35f : 1.0f;
}

//------------------------------------------------------------
// Emergency Shield：最大HPの15%
//------------------------------------------------------------
float MythicRuntime::ConsumeShieldGrant(float maxHP)
{
    if (m_pendingShield <= 0.0f)
    {
        return 0.0f;
    }

    const float grants = m_pendingShield;

    m_pendingShield = 0.0f;

    return maxHP * 0.15f * grants;
}

//------------------------------------------------------------
// Combat Flow：回避成功でDash Cooldownを30%回復
//------------------------------------------------------------
float MythicRuntime::GetDashCooldownScaleOnDodge() const
{
    return Has(MythicEffectType::CombatFlow) ? 0.70f : 1.0f;
}

//------------------------------------------------------------
// 撃破時の抽選
//------------------------------------------------------------
bool MythicRuntime::RollBloodBombardment()
{
    return Has(MythicEffectType::BloodBombardment) && RollPercent(20);
}

bool MythicRuntime::RollAmmoFabricator()
{
    return Has(MythicEffectType::AmmoFabricator) && RollPercent(10);
}

bool MythicRuntime::RollFortuneArmor()
{
    return Has(MythicEffectType::FortuneArmor) && RollPercent(15);
}

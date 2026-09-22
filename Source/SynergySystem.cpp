/*--------------------------------------------------------------------

ファイル名：SynergySystem.cpp
お仕事　　：シナジー10種類の効果

数値はすべて設計書 12. の記載どおり。

----------------------------------------------------------------------*/

#include "SynergySystem.h"

#include <algorithm>
#include <random>

namespace
{
    // 内部乱数（弾薬節約・回避判定など）
    std::mt19937& Rng()
    {
        static std::mt19937 rng{ std::random_device{}() };
        return rng;
    }

    int RollPercent()
    {
        std::uniform_int_distribution<int> dist(1, 100);
        return dist(Rng());
    }

    // Assault 6 の Overdrive 継続時間
    constexpr float OVERDRIVE_TIME = 3.0f;

    // Velocity 6 の回避後バフ継続時間
    constexpr float DODGE_BUFF_TIME = 2.0f;

    // Guardian 6 の Shield 付与間隔
    constexpr float SHIELD_INTERVAL = 10.0f;

    // Vampire の撃破回復 内部CD
    constexpr float VAMPIRE_HEAL_CD = 0.5f;
}

//============================================================
// 集計
//============================================================

void SynergySystem::Clear()
{
    m_counts.fill(0);
}

void SynergySystem::CountSynergies(
    const std::vector<SynergyType>& equipped
)
{
    Clear();

    for (SynergyType s : equipped)
    {
        const int index = static_cast<int>(s);

        if (index >= 0 && index < SYNERGY_COUNT)
        {
            ++m_counts[index];
        }
    }
}

int SynergySystem::GetCount(SynergyType type) const
{
    const int index = static_cast<int>(type);

    if (index < 0 || index >= SYNERGY_COUNT)
    {
        return 0;
    }

    return m_counts[index];
}

int SynergySystem::GetTier(SynergyType type) const
{
    const int n = GetCount(type);

    if (n >= 6) return 6;
    if (n >= 4) return 4;
    if (n >= 2) return 2;

    return 0;
}

//============================================================
// パッシブ効果
//============================================================

void SynergySystem::ApplyPassiveEffects(PlayerStats& s) const
{
    int tier = 0;

    //--------------------------------------------------------
    // ① Assault  連続攻撃／攻撃速度
    //--------------------------------------------------------
    tier = GetTier(SynergyType::Assault);

    if (tier == 2)      s.fireRatePercent += 0.08f;
    else if (tier == 4) s.fireRatePercent += 0.18f;
    else if (tier == 6) s.fireRatePercent += 0.30f;

    //--------------------------------------------------------
    // ② Hunter  Crit
    //--------------------------------------------------------
    tier = GetTier(SynergyType::Hunter);

    if (tier == 2)
    {
        s.critRate += 0.06f;
    }
    else if (tier == 4)
    {
        s.critRate += 0.12f;
        s.critDamage += 0.20f;
    }
    else if (tier == 6)
    {
        s.critRate += 0.18f;
        s.critDamage += 0.40f;
    }

    //--------------------------------------------------------
    // ③ Demolition は命中カウンタ方式なのでパッシブ無し
    //--------------------------------------------------------

    //--------------------------------------------------------
    // ④ Guardian  防御／Shield
    //--------------------------------------------------------
    tier = GetTier(SynergyType::Guardian);

    if (tier == 2)
    {
        s.defense *= 1.08f;
    }
    else if (tier == 4)
    {
        s.defense *= 1.15f;
        s.maxHP *= 1.10f;
    }
    else if (tier == 6)
    {
        s.defense *= 1.25f;
        s.maxHP *= 1.10f;
    }

    //--------------------------------------------------------
    // ⑤ Velocity  移動／回避
    //--------------------------------------------------------
    tier = GetTier(SynergyType::Velocity);

    if (tier == 2)
    {
        s.moveSpeedPercent += 0.06f;
    }
    else if (tier == 4)
    {
        s.moveSpeedPercent += 0.12f;
        s.dodge += 0.05f;
    }
    else if (tier == 6)
    {
        s.moveSpeedPercent += 0.20f;
        s.dodge += 0.10f;
    }

    //--------------------------------------------------------
    // ⑥ Gunner  弾薬／Reload
    //--------------------------------------------------------
    tier = GetTier(SynergyType::Gunner);

    if (tier == 2)
    {
        s.magazinePercent += 0.15f;
        s.reloadSpeedPercent += 0.08f;
    }
    else if (tier == 4)
    {
        s.magazinePercent += 0.30f;
        s.reloadSpeedPercent += 0.15f;
    }
    else if (tier == 6)
    {
        s.magazinePercent += 0.50f;
        s.reloadSpeedPercent += 0.25f;
    }

    //--------------------------------------------------------
    // ⑦ Executioner / ⑧ Vampire はイベント方式
    //--------------------------------------------------------

    //--------------------------------------------------------
    // ⑨ Redeye  弱点／HeadShot
    //--------------------------------------------------------
    tier = GetTier(SynergyType::Redeye);

    if (tier == 2)      s.headShotDamagePercent += 0.15f;
    else if (tier == 4) s.headShotDamagePercent += 0.30f;
    else if (tier == 6) s.headShotDamagePercent += 0.30f;   // 6は必中Crit側で強化

    //--------------------------------------------------------
    // ⑩ Fortune  Drop／高レア装備
    //--------------------------------------------------------
    tier = GetTier(SynergyType::Fortune);

    if (tier == 2)
    {
        s.dropRateBonus += 0.08f;
    }
    else if (tier == 4)
    {
        s.dropRateBonus += 0.12f;
        s.shopRerollDiscount = 0.15f;
    }
    else if (tier == 6)
    {
        s.dropRateBonus += 0.18f;
        s.shopRerollDiscount = 0.15f;
    }
}

//============================================================
// ランタイム
//============================================================

void SynergySystem::ResetRuntime()
{
    m_assaultHitCount = 0;
    m_assaultNextShotBonus = false;
    m_overdriveTimer = 0.0f;

    m_demolitionHitCount = 0;

    m_shieldTimer = 0.0f;
    m_pendingShield = 0.0f;

    m_vampireHealCooldown = 0.0f;
    m_lifestealBudget = 0.0f;
    m_lifestealWindow = 0.0f;

    m_dodgeBuffTimer = 0.0f;
}

void SynergySystem::Update(float deltaTime, float maxHP)
{
    //--------------------------------------------------------
    // 各種タイマー
    //--------------------------------------------------------
    if (m_overdriveTimer > 0.0f)
    {
        m_overdriveTimer -= deltaTime;

        if (m_overdriveTimer < 0.0f)
        {
            m_overdriveTimer = 0.0f;
        }
    }

    if (m_dodgeBuffTimer > 0.0f)
    {
        m_dodgeBuffTimer -= deltaTime;

        if (m_dodgeBuffTimer < 0.0f)
        {
            m_dodgeBuffTimer = 0.0f;
        }
    }

    if (m_vampireHealCooldown > 0.0f)
    {
        m_vampireHealCooldown -= deltaTime;

        if (m_vampireHealCooldown < 0.0f)
        {
            m_vampireHealCooldown = 0.0f;
        }
    }

    //--------------------------------------------------------
    // Vampire 6：吸収上限は「1秒につき最大HPの2%」
    //--------------------------------------------------------
    m_lifestealWindow += deltaTime;

    if (m_lifestealWindow >= 1.0f)
    {
        m_lifestealWindow = 0.0f;
        m_lifestealBudget = maxHP * 0.02f;
    }

    //--------------------------------------------------------
    // Guardian 6：10秒ごとに最大HP12%分のShield
    //--------------------------------------------------------
    if (GetTier(SynergyType::Guardian) >= 6)
    {
        m_shieldTimer += deltaTime;

        if (m_shieldTimer >= SHIELD_INTERVAL)
        {
            m_shieldTimer -= SHIELD_INTERVAL;
            m_pendingShield += maxHP * 0.12f;
        }
    }
    else
    {
        m_shieldTimer = 0.0f;
    }
}

//------------------------------------------------------------
// 1発命中
//------------------------------------------------------------
SynergyHitResult SynergySystem::OnWeaponHit()
{
    SynergyHitResult result;

    //--------------------------------------------------------
    // ① Assault
    //   4：連続命中8回ごとに、次の攻撃 Damage +15%
    //   6：連続命中8回で Overdrive（3秒間 Attack +15%）
    //--------------------------------------------------------
    const int assaultTier = GetTier(SynergyType::Assault);

    if (assaultTier >= 4)
    {
        // 前回の命中で予約された +15% をここで消費する
        if (m_assaultNextShotBonus)
        {
            result.damageMultiplier *= 1.15f;
            m_assaultNextShotBonus = false;
        }

        ++m_assaultHitCount;

        if (m_assaultHitCount >= 8)
        {
            m_assaultHitCount = 0;

            if (assaultTier >= 6)
            {
                m_overdriveTimer = OVERDRIVE_TIME;
            }
            else
            {
                m_assaultNextShotBonus = true;
            }
        }
    }

    //--------------------------------------------------------
    // ③ Demolition
    //   2：12回ごと 60% / 範囲 ×1.00
    //   4： 9回ごと 90% / 範囲 ×1.20
    //   6： 6回ごと120% / 範囲 ×1.35
    //--------------------------------------------------------
    const int demoTier = GetTier(SynergyType::Demolition);

    if (demoTier >= 2)
    {
        ++m_demolitionHitCount;

        const int required = GetDemolitionRequiredHits();

        if (m_demolitionHitCount >= required)
        {
            m_demolitionHitCount = 0;
            result.triggerExplosion = true;

            if (demoTier >= 6)
            {
                result.explosionDamageRatio = 1.20f;
                result.explosionRadiusScale = 1.35f;
            }
            else if (demoTier >= 4)
            {
                result.explosionDamageRatio = 0.90f;
                result.explosionRadiusScale = 1.20f;
            }
            else
            {
                result.explosionDamageRatio = 0.60f;
                result.explosionRadiusScale = 1.00f;
            }
        }
    }

    return result;
}

int SynergySystem::GetDemolitionRequiredHits() const
{
    const int tier = GetTier(SynergyType::Demolition);

    if (tier >= 6) return 6;
    if (tier >= 4) return 9;
    if (tier >= 2) return 12;

    return 0;
}

//------------------------------------------------------------
// ⑧ Vampire：撃破時回復
//   2：最大HPの1.5%（内部CD 0.5秒）
//   4：2.5% / Elite 5%
//   6：3.0% / Elite 5%
//------------------------------------------------------------
float SynergySystem::OnKill(bool elite, float maxHP)
{
    const int tier = GetTier(SynergyType::Vampire);

    if (tier < 2)
    {
        return 0.0f;
    }

    if (m_vampireHealCooldown > 0.0f)
    {
        return 0.0f;
    }

    float percent = 0.0f;

    if (tier == 2)
    {
        percent = 0.015f;
    }
    else if (tier == 4)
    {
        percent = elite ? 0.05f : 0.025f;
    }
    else
    {
        percent = elite ? 0.05f : 0.03f;
    }

    m_vampireHealCooldown = VAMPIRE_HEAL_CD;

    return maxHP * percent;
}

//------------------------------------------------------------
// ⑧ Vampire 6：与Damageの2%をHP吸収
//   吸収上限は1秒につき最大HPの2%
//------------------------------------------------------------
float SynergySystem::OnDamageDealt(float damage, float maxHP)
{
    if (GetTier(SynergyType::Vampire) < 6)
    {
        return 0.0f;
    }

    if (m_lifestealBudget <= 0.0f)
    {
        return 0.0f;
    }

    float heal = damage * 0.02f;

    heal = std::min(heal, m_lifestealBudget);

    m_lifestealBudget -= heal;

    if (maxHP <= 0.0f)
    {
        return 0.0f;
    }

    return heal;
}

void SynergySystem::OnDodgeSuccess()
{
    if (GetTier(SynergyType::Velocity) >= 6)
    {
        m_dodgeBuffTimer = DODGE_BUFF_TIME;
    }
}

//============================================================
// 問い合わせ
//============================================================

//------------------------------------------------------------
// ⑥ Gunner 6：射撃時20%の確率で弾薬を消費しない
//------------------------------------------------------------
bool SynergySystem::ShouldConsumeAmmo()
{
    if (GetTier(SynergyType::Gunner) < 6)
    {
        return true;
    }

    return RollPercent() > 20;
}

bool SynergySystem::HeadShotForcesCrit() const
{
    return GetTier(SynergyType::Redeye) >= 6;
}

bool SynergySystem::ShouldHighlightWeakPoint() const
{
    return GetTier(SynergyType::Redeye) >= 4;
}

//------------------------------------------------------------
// ⑦ Executioner
//   2：HP35%以下へ +12%
//   4：HP35%以下へ +25%
//   6：Boss/Elite はHP20%以下で +35%
//------------------------------------------------------------
float SynergySystem::GetExecutionerBonus(
    float hpRatio,
    bool bossOrElite
) const
{
    const int tier = GetTier(SynergyType::Executioner);

    if (tier == 0)
    {
        return 0.0f;
    }

    if (tier >= 6)
    {
        if (bossOrElite)
        {
            return (hpRatio <= 0.20f) ? 0.35f : 0.0f;
        }

        // 通常敵は ShouldExecute 側で処理するが、
        // 即死ラインに届く前も 25% は乗せておく
        return (hpRatio <= 0.35f) ? 0.25f : 0.0f;
    }

    if (tier == 4)
    {
        return (hpRatio <= 0.35f) ? 0.25f : 0.0f;
    }

    return (hpRatio <= 0.35f) ? 0.12f : 0.0f;
}

//------------------------------------------------------------
// ⑦ Executioner 6：通常敵はHP15%以下で即Execution
//------------------------------------------------------------
bool SynergySystem::ShouldExecute(
    float hpRatio,
    bool bossOrElite
) const
{
    if (bossOrElite)
    {
        return false;
    }

    if (GetTier(SynergyType::Executioner) < 6)
    {
        return false;
    }

    return hpRatio <= 0.15f;
}

float SynergySystem::GetOverdriveAttackBonus() const
{
    return (m_overdriveTimer > 0.0f) ? 0.15f : 0.0f;
}

float SynergySystem::GetOverdriveTimer() const
{
    return m_overdriveTimer;
}

float SynergySystem::GetDodgeFireRateBonus() const
{
    return (m_dodgeBuffTimer > 0.0f) ? 0.20f : 0.0f;
}

float SynergySystem::ConsumeShieldGrant(float /*maxHP*/)
{
    const float value = m_pendingShield;

    m_pendingShield = 0.0f;

    return value;
}

//------------------------------------------------------------
// ② Hunter 6：Elite / BossへのCrit Damage +15%
//------------------------------------------------------------
float SynergySystem::GetEliteCritDamageBonus() const
{
    return (GetTier(SynergyType::Hunter) >= 6) ? 0.15f : 0.0f;
}

int SynergySystem::GetFortuneLevel() const
{
    return GetTier(SynergyType::Fortune);
}

int SynergySystem::GetAssaultHitCount() const
{
    return m_assaultHitCount;
}

int SynergySystem::GetDemolitionHitCount() const
{
    return m_demolitionHitCount;
}

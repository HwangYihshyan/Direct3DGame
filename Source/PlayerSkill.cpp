/*--------------------------------------------------------------------

ファイル名：PlayerSkill.cpp
お仕事　　：キャラクター固有スキル

Synergy の数でスキルが伸びるが、
特定の Synergy が無くても最低限は機能する（設計書 19-7.1）。

----------------------------------------------------------------------*/

#include "PlayerSkill.h"

#include <algorithm>

//============================================================
// 共通処理
//============================================================

void PlayerSkill::Initialize(
    const PlayerCharacterData& data,
    SkillSlot slot
)
{
    m_slot = slot;

    if (slot == SkillSlot::First)
    {
        m_name = data.skillName;
        m_cooldownMax = data.skillCooldown;
        m_durationBase = data.skillDuration;
    }
    else
    {
        m_name = data.skillName2;
        m_cooldownMax = data.skillCooldown2;
        m_durationBase = data.skillDuration2;
    }

    //--------------------------------------------------------
    // skill_icons.png の並び
    //   0 Shock Wave / 1 Bullet Screen
    //   2 Cripple Shot / 3 Blink
    //--------------------------------------------------------
    const int base = (data.type == PlayerType::Player1) ? 0 : 2;

    m_iconIndex = base + ((slot == SkillSlot::First) ? 0 : 1);

    ResetCooldown();
}

void PlayerSkill::ResetCooldown()
{
    m_cooldown = 0.0f;
    m_activeTimer = 0.0f;
}

void PlayerSkill::Update(float deltaTime)
{
    if (m_cooldown > 0.0f)
    {
        m_cooldown = std::max(0.0f, m_cooldown - deltaTime);
    }

    if (m_activeTimer > 0.0f)
    {
        m_activeTimer = std::max(0.0f, m_activeTimer - deltaTime);
    }
}

bool PlayerSkill::CanUse() const
{
    return m_cooldown <= 0.0f;
}

float PlayerSkill::CalcDuration(const SynergySystem& /*synergy*/) const
{
    return m_durationBase;
}

void PlayerSkill::OnActivate(const SynergySystem& /*synergy*/)
{
    // 既定では何もしない
}

SkillActivation PlayerSkill::CalcActivation(
    const SynergySystem& /*synergy*/,
    float /*maxHP*/
) const
{
    SkillActivation result;

    result.activated = true;

    return result;
}

SkillActivation PlayerSkill::Execute(
    const SynergySystem& synergy,
    float maxHP
)
{
    if (!CanUse())
    {
        return SkillActivation();
    }

    m_activeTimer = CalcDuration(synergy);
    m_cooldown = m_cooldownMax;

    OnActivate(synergy);

    SkillActivation result = CalcActivation(synergy, maxHP);

    result.activated = true;

    return result;
}

//============================================================
// Vanguard その1：Shock Wave
//   周囲の敵をノックバックさせ、Damageを与える。
//   Guardian の数だけ範囲と威力が伸びる。
//============================================================

SkillBuff Vanguard_ShockWave::GetBuff() const
{
    SkillBuff buff;

    // 発動の瞬間だけ踏ん張るので少し遅くなる
    if (IsActive())
    {
        buff.moveSpeedBonus = -0.25f;
    }

    return buff;
}

SkillActivation Vanguard_ShockWave::CalcActivation(
    const SynergySystem& synergy,
    float maxHP
) const
{
    SkillActivation result;

    result.activated = true;
    result.shockWave = true;

    const int guardian = synergy.GetCount(SynergyType::Guardian);

    // 半径 260 を基準に、Guardian 1個ごとに +20
    result.shockRadius = 260.0f + guardian * 20.0f;

    // 最大HPを基準にした固定Damage。装備に依存しすぎないようにする
    result.shockDamage = maxHP * (0.55f + guardian * 0.05f);

    result.knockBack = 320.0f + guardian * 20.0f;

    return result;
}

//============================================================
// Vanguard その2：Bullet Screen
//   一定時間、あらゆるDamageを受けなくなる。
//   強力なぶんクールダウンは長い。
//============================================================

SkillBuff Vanguard_BulletScreen::GetBuff() const
{
    SkillBuff buff;

    if (IsActive())
    {
        buff.invincible = true;
    }

    return buff;
}

float Vanguard_BulletScreen::CalcDuration(const SynergySystem& synergy) const
{
    const int guardian = synergy.GetCount(SynergyType::Guardian);

    // Guardian 1個につき +0.5秒、最大 +3秒
    return m_durationBase + std::min(guardian * 0.5f, 3.0f);
}

//============================================================
// 突撃兵 その1：制圧射撃
//
//   10秒間、攻撃力と連射速度が上がり、
//   そのあいだ弾薬を消費しなくなる。
//
//   リロードも弾切れも気にせず撃ち続けられるので、
//   「攻めきる」ことが強さになる突撃兵に合わせている。
//
//   上昇量は発動した瞬間の 強襲(Assault) の数で伸びる。
//============================================================

SkillBuff Marksman_SuppressiveFire::GetBuff() const
{
    SkillBuff buff;

    if (IsActive())
    {
        buff.attackBonus = m_attackBonus;
        buff.fireRateBonus = m_fireRateBonus;
        buff.infiniteAmmo = true;
    }

    return buff;
}

float Marksman_SuppressiveFire::CalcDuration(
    const SynergySystem& /*synergy*/
) const
{
    // 効果時間は10秒で固定。伸びるのは上昇量のほう。
    return m_durationBase;
}

SkillActivation Marksman_SuppressiveFire::CalcActivation(
    const SynergySystem& /*synergy*/,
    float /*maxHP*/
) const
{
    SkillActivation result;

    result.activated = true;

    //--------------------------------------------------------
    // 弾切れのまま発動しても撃てるように、弾倉を満タンにする。
    // 予備弾は減らさない。
    //--------------------------------------------------------
    result.refillMagazine = true;

    return result;
}

void Marksman_SuppressiveFire::OnActivate(const SynergySystem& synergy)
{
    const int assault = synergy.GetCount(SynergyType::Assault);

    m_attackBonus =
        ATTACK_BONUS + assault * ATTACK_PER_ASSAULT;

    m_fireRateBonus =
        FIRE_RATE_BONUS + assault * FIRE_RATE_PER_ASSAULT;
}

//============================================================
// Marksman その2：Blink
//   方向キーの向きへ瞬間移動する。
//============================================================

SkillBuff Marksman_Blink::GetBuff() const
{
    return SkillBuff();
}

SkillActivation Marksman_Blink::CalcActivation(
    const SynergySystem& synergy,
    float /*maxHP*/
) const
{
    SkillActivation result;

    result.activated = true;

    const int velocity = synergy.GetCount(SynergyType::Velocity);

    // 基本 340px、Velocity 1個ごとに +30px
    result.blinkDistance = 340.0f + velocity * 30.0f;

    return result;
}

//============================================================
// Player3：Aegis Field（将来枠）
//============================================================

SkillBuff Player3_Skill::GetBuff() const
{
    SkillBuff buff;

    if (IsActive())
    {
        buff.moveSpeedBonus = -0.15f;   // 踏ん張るぶん遅くなる
    }

    return buff;
}

SkillActivation Player3_Skill::CalcActivation(
    const SynergySystem& synergy,
    float maxHP
) const
{
    SkillActivation result;

    result.activated = true;

    const int guardian = synergy.GetCount(SynergyType::Guardian);

    result.shield = maxHP * (0.25f + 0.02f * guardian);

    return result;
}

//============================================================
// Player4：Blood Pact（将来枠）
//============================================================

SkillBuff Player4_Skill::GetBuff() const
{
    SkillBuff buff;

    if (IsActive())
    {
        buff.lifesteal = 0.08f;
        buff.attackBonus = 0.10f;
    }

    return buff;
}

//============================================================

std::unique_ptr<PlayerSkill> CreatePlayerSkill(
    PlayerType type,
    SkillSlot slot
)
{
    std::unique_ptr<PlayerSkill> skill;

    const bool first = (slot == SkillSlot::First);

    switch (type)
    {
    case PlayerType::Player1:
        if (first) skill = std::make_unique<Vanguard_ShockWave>();
        else       skill = std::make_unique<Vanguard_BulletScreen>();
        break;

    case PlayerType::Player2:
        if (first) skill = std::make_unique<Marksman_SuppressiveFire>();
        else       skill = std::make_unique<Marksman_Blink>();
        break;

    case PlayerType::Player3:
        skill = std::make_unique<Player3_Skill>();
        break;

    case PlayerType::Player4:
        skill = std::make_unique<Player4_Skill>();
        break;
    }

    if (skill)
    {
        skill->Initialize(GetPlayerCharacter(type), slot);
    }

    return skill;
}

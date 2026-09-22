/*--------------------------------------------------------------------

ファイル名：MythicEffect.h
お仕事　　：SSS（Mythic）装備の固有能力

設計書 5-2.「SSS のみ固有能力を持つ」
設計書 15. 完成装備例「Blood Bombardment」
設計書 19. 第5段階「SSS Mythic固有能力」

固有能力は文字列ではなく列挙型で持ち、
発動条件（命中回数・タイマー・確率）を MythicRuntime が管理する。

----------------------------------------------------------------------*/

#ifndef MYTHICEFFECT_H
#define MYTHICEFFECT_H

#include <array>
#include <vector>

enum class MythicEffectType
{
    None,

    //--------------------------------------------------------
    // 武器のSSS固有能力
    //--------------------------------------------------------
    BloodBombardment,   // 敵撃破時20%で武器Damage150%の爆発
    PerfectChamber,     // 8発目のDamage +100%
    EndlessMagazine,    // Reload直後3秒間、弾薬を消費しない
    DeadeyeProtocol,    // HeadShot時2秒間 Crit Rate +30%
    OverclockCore,      // 20回命中で5秒間 Fire Rate +35%

    //--------------------------------------------------------
    // 防具のSSS固有能力
    //--------------------------------------------------------
    LastStand,          // HP30%以下で Defense +35%
    EmergencyShield,    // 15秒ごとに最大HP15%のShield
    CombatFlow,         // 回避成功でDash Cooldownを30%回復
    AmmoFabricator,     // 敵撃破時10%で弾薬を補充
    FortuneArmor        // Elite撃破時15%で追加装備
};

constexpr int MYTHIC_WEAPON_COUNT = 5;
constexpr int MYTHIC_ARMOR_COUNT = 5;

const wchar_t* GetMythicName(MythicEffectType type);
const wchar_t* GetMythicDescription(MythicEffectType type);

bool IsWeaponMythic(MythicEffectType type);

const std::array<MythicEffectType, MYTHIC_WEAPON_COUNT>& GetWeaponMythicList();
const std::array<MythicEffectType, MYTHIC_ARMOR_COUNT>& GetArmorMythicList();

//------------------------------------------------------------
// 固有能力の発動状態
//
// 装備しているSSSの能力だけが有効になる。
// 同じ能力を2個装備しても重複はしない（フラグ管理）。
//------------------------------------------------------------
class MythicRuntime
{
public:
    // 装備中のSSS能力を設定する（発動中のタイマーは維持）
    void SetEquipped(const std::vector<MythicEffectType>& equipped);

    bool Has(MythicEffectType type) const;

    // 1つでも所持しているか（HUD表示用）
    bool HasAny() const;

    const std::vector<MythicEffectType>& GetEquipped() const
    {
        return m_equipped;
    }

    void Reset();
    void Update(float deltaTime);

    //--------------------------------------------------------
    // 射撃まわり
    //--------------------------------------------------------

    // PerfectChamber：8発目なら 2.0 を返す
    float OnShotFired();

    // EndlessMagazine：Reload直後3秒間は弾薬を消費しない
    void OnReloadFinished();
    bool IsAmmoFree() const;

    // OverclockCore：命中20回で発動
    void OnWeaponHit();
    float GetFireRateBonus() const;

    // DeadeyeProtocol：HeadShot後2秒間のCrit Rate上昇
    void OnHeadShot();
    float GetCritRateBonus() const;

    //--------------------------------------------------------
    // 生存まわり
    //--------------------------------------------------------

    // LastStand：HP30%以下で Defense +35%
    float GetDefenseMultiplier(float hpRatio) const;

    // EmergencyShield：15秒ごとに最大HPの15%
    float ConsumeShieldGrant(float maxHP);

    // CombatFlow：回避成功でDash Cooldownを30%回復
    // 残りクールダウンに掛ける係数を返す
    float GetDashCooldownScaleOnDodge() const;

    //--------------------------------------------------------
    // 撃破時
    //--------------------------------------------------------

    // BloodBombardment：爆発するか
    bool RollBloodBombardment();

    // AmmoFabricator：弾薬を補充するか
    bool RollAmmoFabricator();

    // FortuneArmor：Elite撃破時に追加装備が出るか
    bool RollFortuneArmor();

    //--------------------------------------------------------
    // 表示用
    //--------------------------------------------------------
    float GetOverclockTimer() const { return m_overclockTimer; }
    float GetDeadeyeTimer() const { return m_deadeyeTimer; }
    float GetEndlessMagazineTimer() const { return m_endlessTimer; }
    int GetPerfectChamberCount() const { return m_shotCount; }

private:
    std::vector<MythicEffectType> m_equipped;

    // PerfectChamber
    int m_shotCount = 0;

    // EndlessMagazine
    float m_endlessTimer = 0.0f;

    // OverclockCore
    int m_hitCount = 0;
    float m_overclockTimer = 0.0f;

    // DeadeyeProtocol
    float m_deadeyeTimer = 0.0f;

    // EmergencyShield
    float m_shieldTimer = 0.0f;
    float m_pendingShield = 0.0f;
};

#endif

/*--------------------------------------------------------------------

ファイル名：PlayerSkill.h
お仕事　　：キャラクター固有スキル

設計書 Ver.3 19-7.1 ～ 19-7.4
    Player1_Skill / Player2_Skill / Player3_Skill / Player4_Skill

    共通Interface：
        Initialize() / Update() / CanUse() / Execute() / ResetCooldown()

    要件：
        Synergy Count により性能が変化する構造にする。
        ただし特定のSynergyを必須にはしない。

4つとも中身が「一定時間バフを掛ける」形なので、
1つの基底クラスと4つの派生クラスにまとめている。

----------------------------------------------------------------------*/

#ifndef PLAYERSKILL_H
#define PLAYERSKILL_H

#include <memory>

#include "PlayerCharacter.h"
#include "SynergySystem.h"

//------------------------------------------------------------
// スキル発動中に戦闘へ渡す補正
//------------------------------------------------------------
struct SkillBuff
{
    float fireRateBonus = 0.0f;
    float moveSpeedBonus = 0.0f;
    float critRateBonus = 0.0f;
    float headShotBonus = 0.0f;
    float attackBonus = 0.0f;
    float lifesteal = 0.0f;        // 与Damageに対する吸収率

    // 一定時間 無敵になる（重装兵の 弾幕障壁）
    bool invincible = false;

    // 発動中は弾薬を消費しない（突撃兵の 制圧射撃）
    bool infiniteAmmo = false;
};

//------------------------------------------------------------
// スキルの枠
//------------------------------------------------------------
enum class SkillSlot
{
    First,   // K キー
    Second   // L キー
};

//------------------------------------------------------------
// 発動した瞬間に1回だけ起きること
//------------------------------------------------------------
struct SkillActivation
{
    bool activated = false;

    // 付与するShield（実HP量）
    float shield = 0.0f;

    // 回復量（実HP量）
    float heal = 0.0f;

    //--------------------------------------------------------
    // 衝撃波（Vanguard の Shock Wave）
    //--------------------------------------------------------
    bool shockWave = false;
    float shockRadius = 0.0f;
    float shockDamage = 0.0f;
    float knockBack = 0.0f;

    //--------------------------------------------------------
    // 瞬間移動（Marksman の Blink）
    //--------------------------------------------------------
    float blinkDistance = 0.0f;

    //--------------------------------------------------------
    // 弾倉を満タンにする（突撃兵の 制圧射撃）
    //
    // 弾切れのまま発動しても撃ち出せるようにするためのもの。
    // 予備弾は減らさない。効果中は弾薬を消費しないので、
    // 効果が切れたあとも満タンのまま引き継がれる。
    //--------------------------------------------------------
    bool refillMagazine = false;
};

class PlayerSkill
{
public:
    virtual ~PlayerSkill() = default;

    void Initialize(const PlayerCharacterData& data, SkillSlot slot);

    SkillSlot GetSlot() const { return m_slot; }

    // skill_icons.png の何番目を使うか
    int GetIconIndex() const { return m_iconIndex; }

    void Update(float deltaTime);

    bool CanUse() const;

    // 使えたら発動する。使えなければ activated=false が返る
    SkillActivation Execute(const SynergySystem& synergy, float maxHP);

    void ResetCooldown();

    bool IsActive() const { return m_activeTimer > 0.0f; }

    float GetCooldown() const { return m_cooldown; }
    float GetCooldownMax() const { return m_cooldownMax; }
    float GetActiveTimer() const { return m_activeTimer; }

    const wchar_t* GetName() const { return m_name; }

    // 発動中の補正。止まっていれば何も乗らない
    virtual SkillBuff GetBuff() const = 0;

protected:
    // 派生クラスが効果時間や付与量を決める
    virtual float CalcDuration(const SynergySystem& synergy) const;

    //--------------------------------------------------------
    // 発動した瞬間に1回だけ呼ばれる。
    //
    // CalcActivation は const なので、
    // 「発動時のシナジー数で上昇量を決めて持っておく」
    // といったことができない。そのための入口。
    //--------------------------------------------------------
    virtual void OnActivate(const SynergySystem& synergy);

    virtual SkillActivation CalcActivation(
        const SynergySystem& synergy,
        float maxHP
    ) const;

protected:
    const wchar_t* m_name = L"";
    SkillSlot m_slot = SkillSlot::First;
    int m_iconIndex = 0;

    float m_cooldownMax = 14.0f;
    float m_cooldown = 0.0f;

    float m_durationBase = 5.0f;
    float m_activeTimer = 0.0f;
};

//------------------------------------------------------------
// Player1 その1：Shock Wave
//   周囲へ衝撃波。ノックバックとDamage。
//------------------------------------------------------------
class Vanguard_ShockWave : public PlayerSkill
{
public:
    SkillBuff GetBuff() const override;

protected:
    SkillActivation CalcActivation(
        const SynergySystem& synergy,
        float maxHP
    ) const override;
};

//------------------------------------------------------------
// Player1 その2：Bullet Screen
//   一定時間、敵の弾を無効化する。
//------------------------------------------------------------
class Vanguard_BulletScreen : public PlayerSkill
{
public:
    SkillBuff GetBuff() const override;

protected:
    float CalcDuration(const SynergySystem& synergy) const override;
};

//------------------------------------------------------------
// Player2 その1：制圧射撃
//   10秒間、攻撃力と連射速度が上がり、弾薬を消費しなくなる。
//   発動した瞬間に弾倉を満タンにするので、
//   弾切れのまま撃てなくなっている状況からでも使える。
//   撃ち続けることが強さになる突撃兵向けのスキル。
//------------------------------------------------------------
class Marksman_SuppressiveFire : public PlayerSkill
{
public:
    //--------------------------------------------------------
    // 上昇量。Assault の数だけ伸びる。
    //--------------------------------------------------------
    static constexpr float ATTACK_BONUS = 0.30f;      // 攻撃力 +30%
    static constexpr float FIRE_RATE_BONUS = 0.45f;   // 連射速度 +45%

    // Assault 1個ごとの上乗せ
    static constexpr float ATTACK_PER_ASSAULT = 0.02f;
    static constexpr float FIRE_RATE_PER_ASSAULT = 0.03f;

    SkillBuff GetBuff() const override;

protected:
    float CalcDuration(const SynergySystem& synergy) const override;

    // 発動した瞬間に弾倉を満タンにする
    SkillActivation CalcActivation(
        const SynergySystem& synergy,
        float maxHP
    ) const override;

    // 上昇量は発動した瞬間の Assault の数で決める
    void OnActivate(const SynergySystem& synergy) override;

private:
    float m_attackBonus = ATTACK_BONUS;
    float m_fireRateBonus = FIRE_RATE_BONUS;
};

//------------------------------------------------------------
// Player2 その2：Blink
//   方向キーの向きへ瞬間移動する。
//------------------------------------------------------------
class Marksman_Blink : public PlayerSkill
{
public:
    SkillBuff GetBuff() const override;

protected:
    SkillActivation CalcActivation(
        const SynergySystem& synergy,
        float maxHP
    ) const override;
};

//------------------------------------------------------------
// Player3：Aegis Field（19-7.3 将来枠）
//------------------------------------------------------------
class Player3_Skill : public PlayerSkill
{
public:
    SkillBuff GetBuff() const override;

protected:
    SkillActivation CalcActivation(
        const SynergySystem& synergy,
        float maxHP
    ) const override;
};

//------------------------------------------------------------
// Player4：Blood Pact（19-7.4 将来枠）
//------------------------------------------------------------
class Player4_Skill : public PlayerSkill
{
public:
    SkillBuff GetBuff() const override;
};

// キャラクターと枠に応じたスキルを作る
std::unique_ptr<PlayerSkill> CreatePlayerSkill(PlayerType type, SkillSlot slot);

#endif

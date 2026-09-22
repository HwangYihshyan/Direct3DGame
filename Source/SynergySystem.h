/*--------------------------------------------------------------------

ファイル名：SynergySystem.h
お仕事　　：シナジー10種類の集計と効果適用

設計書 12. シナジー10種類 再調整版 を全実装。

・パッシブ効果（Fire Rate、Crit など）は ApplyPassiveEffects で加算
・命中カウンタや発動タイマーなど「動く効果」は
  このクラス内にランタイム状態として保持する

----------------------------------------------------------------------*/

#ifndef SYNERGYSYSTEM_H
#define SYNERGYSYSTEM_H

#include <array>
#include <vector>

#include "EquipmentTypes.h"
#include "PlayerStats.h"

//------------------------------------------------------------
// 1発命中したときにシナジーが返す結果
//------------------------------------------------------------
struct SynergyHitResult
{
    // このダメージへ掛ける追加倍率（Assault 4 の「次の攻撃 +15%」など）
    float damageMultiplier = 1.0f;

    // Demolition：爆発を発生させるか
    bool triggerExplosion = false;

    // 爆発ダメージ＝武器Damageの何倍か
    float explosionDamageRatio = 0.0f;

    // 爆発範囲の倍率
    float explosionRadiusScale = 1.0f;
};

class SynergySystem
{
public:
    //--------------------------------------------------------
    // 集計
    //--------------------------------------------------------

    // 装備中の全シナジーを数え直す（ランタイム状態は維持）
    void CountSynergies(const std::vector<SynergyType>& equipped);

    void Clear();

    int GetCount(SynergyType type) const;

    // 発動段階 0 / 2 / 4 / 6
    int GetTier(SynergyType type) const;

    //--------------------------------------------------------
    // パッシブ効果
    //--------------------------------------------------------
    void ApplyPassiveEffects(PlayerStats& stats) const;

    //--------------------------------------------------------
    // ランタイム
    //--------------------------------------------------------
    void ResetRuntime();
    void Update(float deltaTime, float maxHP);

    // 1発命中したとき
    SynergyHitResult OnWeaponHit();

    // 敵を倒したとき：回復量（実HP）を返す
    float OnKill(bool elite, float maxHP);

    // ダメージを与えたとき：Vampire 6 の吸収量（実HP）を返す
    float OnDamageDealt(float damage, float maxHP);

    // 回避に成功したとき（Velocity 6）
    void OnDodgeSuccess();

    //--------------------------------------------------------
    // 各シナジーの問い合わせ
    //--------------------------------------------------------

    // Gunner 6：20%で弾を消費しない
    bool ShouldConsumeAmmo();

    // Redeye 6：HeadShotは必ずCrit
    bool HeadShotForcesCrit() const;

    // Redeye 4：弱点表示を強調するか
    bool ShouldHighlightWeakPoint() const;

    // Executioner：瀕死の敵への追加Damage倍率（0.25 で +25%）
    float GetExecutionerBonus(float hpRatio, bool bossOrElite) const;

    // Executioner 6：通常敵をHP15%以下で即Execution
    bool ShouldExecute(float hpRatio, bool bossOrElite) const;

    // Assault 6：Overdrive中のAttackボーナス
    float GetOverdriveAttackBonus() const;
    float GetOverdriveTimer() const;

    // Velocity 6：回避後2秒間のFire Rateボーナス
    float GetDodgeFireRateBonus() const;

    // Guardian 6：10秒ごとのShield付与。付与する実HP量を返す（無ければ0）
    float ConsumeShieldGrant(float maxHP);

    // Hunter 6：Elite / BossへのCrit Damage追加
    float GetEliteCritDamageBonus() const;

    // Fortune段階（0 / 2 / 4 / 6）
    int GetFortuneLevel() const;

    //--------------------------------------------------------
    // デバッグ／HUD表示用
    //--------------------------------------------------------
    int GetAssaultHitCount() const;
    int GetDemolitionHitCount() const;
    int GetDemolitionRequiredHits() const;

private:
    std::array<int, SYNERGY_COUNT> m_counts{};

    //--------------------------------------------------------
    // ランタイム状態
    //--------------------------------------------------------

    // Assault：連続命中カウンタ
    int m_assaultHitCount = 0;

    // Assault 4：次の1発だけ +15%
    bool m_assaultNextShotBonus = false;

    // Assault 6：Overdrive 残り時間
    float m_overdriveTimer = 0.0f;

    // Demolition：命中カウンタ
    int m_demolitionHitCount = 0;

    // Guardian 6：Shield付与タイマー
    float m_shieldTimer = 0.0f;
    float m_pendingShield = 0.0f;

    // Vampire：撃破回復の内部CD
    float m_vampireHealCooldown = 0.0f;

    // Vampire 6：1秒あたりの吸収上限管理
    float m_lifestealBudget = 0.0f;
    float m_lifestealWindow = 0.0f;

    // Velocity 6：回避後バフ
    float m_dodgeBuffTimer = 0.0f;
};

#endif

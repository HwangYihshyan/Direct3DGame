/*--------------------------------------------------------------------

ファイル名：PlayerStats.h
お仕事　　：装備とシナジーを合算した最終ステータス

「基礎値 → 装備メインステータス → ランダムOP → シナジー」
の順に積み上げた結果がここに入る。

----------------------------------------------------------------------*/

#ifndef PLAYERSTATS_H
#define PLAYERSTATS_H

#include <string>
#include <vector>

//------------------------------------------------------------
// HUD へ並べるための1行
//------------------------------------------------------------
struct StatLine
{
    const wchar_t* label;
    float value;
    bool isPercent;

    // stat_icons.png の何番目か
    int iconIndex;
};

// アイコン番号から名前と説明を引く（PlayerStats.cpp）
const wchar_t* GetStatName(int iconIndex);
const wchar_t* GetStatDescription(int iconIndex);

struct PlayerStats
{
    //--------------------------------------------------------
    // 生存
    //--------------------------------------------------------
    float maxHP = 1000.0f;
    float defense = 100.0f;
    float damageReduction = 0.0f;   // 0.0～ （0.08 で 8%カット）
    float shieldPercent = 0.0f;     // 最大HPに対する初期シールド%

    //--------------------------------------------------------
    // 攻撃
    //--------------------------------------------------------
    float attackPercent = 0.0f;
    float critRate = 0.05f;         // 基礎5%
    float critDamage = 0.50f;       // 基礎 ×1.5
    float headShotDamagePercent = 0.0f;
    float armorPenetration = 0.0f;

    //--------------------------------------------------------
    // 射撃
    //--------------------------------------------------------
    float fireRatePercent = 0.0f;
    float reloadSpeedPercent = 0.0f;
    float magazinePercent = 0.0f;
    float reserveAmmoPercent = 0.0f;
    float ammoDropPercent = 0.0f;
    float recoilControl = 0.0f;
    float weaponHandling = 0.0f;

    //--------------------------------------------------------
    // 機動
    //--------------------------------------------------------
    float moveSpeedPercent = 0.0f;
    float dodge = 0.0f;
    float dashCooldownReduction = 0.0f;
    float dashDistancePercent = 0.0f;

    //--------------------------------------------------------
    // その他
    //--------------------------------------------------------
    float detectionPercent = 0.0f;
    float dropRateBonus = 0.0f;
    float shopRerollDiscount = 0.0f;

    // 基礎値へ戻す
    void Reset()
    {
        *this = PlayerStats();
    }

    // 上限クランプ（PlayerStats.cpp）
    void Clamp();

    // 表示用の一覧を作る（PlayerStats.cpp）
    std::vector<StatLine> BuildDisplayList() const;
};

#endif

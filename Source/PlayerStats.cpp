/*--------------------------------------------------------------------

ファイル名：PlayerStats.cpp
お仕事　　：最終ステータスの補助処理

設計書 Ver.3 19-7.6
    計算順序：
      Base Character → Weapon Base → Rarity → Random OP
      → Armor → Synergy Passive → Temporary Buff

実際の積み上げは PlayerLoadout が行う。
ここでは上限クランプと、表示用の一覧化だけを担当する。

----------------------------------------------------------------------*/

#include "PlayerStats.h"

#include <algorithm>

void PlayerStats::Clamp()
{
    //--------------------------------------------------------
    // 上限を設けないと、装備が揃ったときに
    // 回避100%や被ダメ0%が成立してしまう。
    //--------------------------------------------------------
    dodge = std::clamp(dodge, 0.0f, 0.60f);
    damageReduction = std::clamp(damageReduction, 0.0f, 0.60f);
    critRate = std::clamp(critRate, 0.0f, 1.00f);
    armorPenetration = std::clamp(armorPenetration, 0.0f, 0.90f);

    dashCooldownReduction =
        std::clamp(dashCooldownReduction, 0.0f, 0.65f);

    // 連射速度が上がりすぎると弾が消費できなくなる
    fireRatePercent = std::min(fireRatePercent, 2.0f);

    maxHP = std::max(maxHP, 1.0f);
    defense = std::max(defense, 0.0f);
}

//------------------------------------------------------------
// アイコンの名前
// 並びは stat_icons.png と同じ
//------------------------------------------------------------
const wchar_t* GetStatName(int iconIndex)
{
    static const wchar_t* NAMES[20] =
    {
        L"攻撃力",           L"連射速度",        L"装弾数",
        L"リロード",           L"クリティカル率",        L"クリティカルダメージ",
        L"ヘッドショットダメージ",  L"装甲貫通",L"最大HP",
        L"防御力",          L"ダメージ軽減", L"シールド",
        L"予備弾薬",     L"弾薬ドロップ",        L"反動制御",
        L"武器取り回し",  L"移動速度",       L"回避率",
        L"ダッシュ再使用時間",    L"ダッシュ距離"
    };

    if (iconIndex < 0 || iconIndex >= 20)
    {
        return L"";
    }

    return NAMES[iconIndex];
}

//------------------------------------------------------------
// アイコンの説明
//------------------------------------------------------------
const wchar_t* GetStatDescription(int iconIndex)
{
    static const wchar_t* TEXTS[20] =
    {
        L"武器のダメージが増える",
        L"連射速度が上がる。弾の消費も早くなる",
        L"1マガジンに込められる弾数",
        L"リロードにかかる時間。短いほど良い",
        L"クリティカルが出る確率",
        L"クリティカル時の追加ダメージ",
        L"弱点に当てたときの追加ダメージ",
        L"敵の装甲を無視する割合",
        L"体力の上限",
        L"受けるダメージを減らす。高いほど硬い",
        L"防御力とは別に、受けるダメージを直接減らす",
        L"HPの前に削られる保護。ステージ開始時に張る",
        L"持ち歩ける予備弾の量",
        L"敵が弾薬を落とす確率",
        L"射撃時の拡散を抑える",
        L"武器の取り回し。拡散が小さくなる",
        L"移動の速さ",
        L"攻撃を完全に避ける確率",
        L"ダッシュの再使用までの時間。短いほど良い",
        L"ダッシュで進む距離"
    };

    if (iconIndex < 0 || iconIndex >= 20)
    {
        return L"";
    }

    return TEXTS[iconIndex];
}

std::vector<StatLine> PlayerStats::BuildDisplayList() const
{
    std::vector<StatLine> lines;

    //--------------------------------------------------------
    // 表示順は「生存 → 攻撃 → 射撃 → 機動」
    //--------------------------------------------------------
    lines.push_back({ L"最大HP",            maxHP,                          false,  8 });
    lines.push_back({ L"防御力",           defense,                        false,  9 });
    lines.push_back({ L"ダメージ軽減",  damageReduction * 100.0f,       true,  10 });
    lines.push_back({ L"シールド",            shieldPercent * 100.0f,         true,  11 });
    lines.push_back({ L"攻撃力",            attackPercent * 100.0f,         true,   0 });
    lines.push_back({ L"クリティカル率",         critRate * 100.0f,              true,   4 });
    lines.push_back({ L"クリティカルダメージ",       critDamage * 100.0f,            true,   5 });
    lines.push_back({ L"ヘッドショットダメージ",   headShotDamagePercent * 100.0f, true,   6 });
    lines.push_back({ L"装甲貫通", armorPenetration * 100.0f,      true,   7 });
    lines.push_back({ L"連射速度",         fireRatePercent * 100.0f,       true,   1 });
    lines.push_back({ L"リロード速度",      reloadSpeedPercent * 100.0f,    true,   3 });
    lines.push_back({ L"装弾数",     magazinePercent * 100.0f,       true,   2 });
    lines.push_back({ L"予備弾薬",      reserveAmmoPercent * 100.0f,    true,  12 });
    lines.push_back({ L"移動速度",        moveSpeedPercent * 100.0f,      true,  16 });
    lines.push_back({ L"回避率",             dodge * 100.0f,                 true,  17 });

    return lines;
}

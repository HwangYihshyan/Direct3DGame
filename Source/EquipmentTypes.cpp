/*--------------------------------------------------------------------

ファイル名：EquipmentTypes.cpp
お仕事　　：列挙型の表示用文字列と色

----------------------------------------------------------------------*/

#include "EquipmentTypes.h"

const wchar_t* ToString(Rarity v)
{
    switch (v)
    {
    case Rarity::Common:    return L"D / コモン";
    case Rarity::Uncommon:  return L"C / アンコモン";
    case Rarity::Rare:      return L"B / レア";
    case Rarity::Special:   return L"A / スペシャル";
    case Rarity::Epic:      return L"S / エピック";
    case Rarity::Legendary: return L"SS / レジェンダリー";
    case Rarity::Mythic:    return L"SSS / ミシック";
    }
    return L"不明";
}

const wchar_t* ToShortString(Rarity v)
{
    switch (v)
    {
    case Rarity::Common:    return L"D";
    case Rarity::Uncommon:  return L"C";
    case Rarity::Rare:      return L"B";
    case Rarity::Special:   return L"A";
    case Rarity::Epic:      return L"S";
    case Rarity::Legendary: return L"SS";
    case Rarity::Mythic:    return L"SSS";
    }
    return L"?";
}

const wchar_t* ToString(SynergyType v)
{
    switch (v)
    {
    case SynergyType::Assault:     return L"強襲";
    case SynergyType::Hunter:      return L"狩人";
    case SynergyType::Demolition:  return L"爆破";
    case SynergyType::Guardian:    return L"守護";
    case SynergyType::Velocity:    return L"俊足";
    case SynergyType::Gunner:      return L"銃手";
    case SynergyType::Executioner: return L"処刑";
    case SynergyType::Vampire:     return L"吸血";
    case SynergyType::Redeye:      return L"紅眼";
    case SynergyType::Fortune:     return L"幸運";
    }
    return L"不明";
}

const wchar_t* GetSynergyThemeText(SynergyType v)
{
    switch (v)
    {
    case SynergyType::Assault:     return L"連射速度 / 連続命中";
    case SynergyType::Hunter:      return L"クリティカル率 / クリティカルダメージ";
    case SynergyType::Demolition:  return L"命中ごとに追加爆発";
    case SynergyType::Guardian:    return L"防御力 / 最大HP / シールド";
    case SynergyType::Velocity:    return L"移動速度 / 回避率";
    case SynergyType::Gunner:      return L"装弾数 / リロード";
    case SynergyType::Executioner: return L"瀕死の敵へのダメージ";
    case SynergyType::Vampire:     return L"撃破時HP回復 / 吸収";
    case SynergyType::Redeye:      return L"ヘッドショットダメージ";
    case SynergyType::Fortune:     return L"ドロップ率 / 高レア率";
    }
    return L"";
}

//------------------------------------------------------------
// 発動段階ごとの効果（設計書 12. の数値そのまま）
//------------------------------------------------------------
const wchar_t* GetSynergyTierText(SynergyType v, int tier)
{
    // まだ発動していない場合は 2 発動時の内容を見せる
    if (tier < 2)
    {
        tier = 2;
    }

    switch (v)
    {
    case SynergyType::Assault:
        if (tier >= 6) return L"6: 連射速度 +30% / 連続8命中でオーバードライブ";
        if (tier >= 4) return L"4: 連射速度 +18% / 連続8命中で次の攻撃 +15%";
        return L"2: 連射速度 +8%";

    case SynergyType::Hunter:
        if (tier >= 6) return L"6: クリティカル率 +18% / クリティカルダメージ +40% / エリート特効";
        if (tier >= 4) return L"4: クリティカル率 +12% / クリティカルダメージ +20%";
        return L"2: クリティカル率 +6%";

    case SynergyType::Demolition:
        if (tier >= 6) return L"6: 6命中ごとに武器ダメージ120%の爆発";
        if (tier >= 4) return L"4: 9命中ごとに武器ダメージ90%の爆発";
        return L"2: 12命中ごとに武器ダメージ60%の爆発";

    case SynergyType::Guardian:
        if (tier >= 6) return L"6: 防御力 +25% / 最大HP +10% / 10秒ごとシールド";
        if (tier >= 4) return L"4: 防御力 +15% / 最大HP +10%";
        return L"2: 防御力 +8%";

    case SynergyType::Velocity:
        if (tier >= 6) return L"6: 移動速度 +20% / 回避率 +10%";
        if (tier >= 4) return L"4: 移動速度 +12% / 回避率 +5%";
        return L"2: 移動速度 +6%";

    case SynergyType::Gunner:
        if (tier >= 6) return L"6: 装弾数 +50% / リロード +25% / 20%で弾薬不消費";
        if (tier >= 4) return L"4: 装弾数 +30% / リロード +15%";
        return L"2: 装弾数 +15% / リロード +8%";

    case SynergyType::Executioner:
        if (tier >= 6) return L"6: 通常敵はHP15%以下で即撃破";
        if (tier >= 4) return L"4: HP35%以下の敵へ ダメージ +25%";
        return L"2: HP35%以下の敵へ ダメージ +12%";

    case SynergyType::Vampire:
        if (tier >= 6) return L"6: 撃破で3%回復 / 与ダメージの2%を吸収";
        if (tier >= 4) return L"4: 撃破で2.5%回復（エリートは5%）";
        return L"2: 撃破で最大HPの1.5%回復";

    case SynergyType::Redeye:
        if (tier >= 6) return L"6: ヘッドショットダメージ +30% / ヘッドショットは必ずクリティカル";
        if (tier >= 4) return L"4: ヘッドショットダメージ +30% / 弱点を強調表示";
        return L"2: ヘッドショットダメージ +15%";

    case SynergyType::Fortune:
        if (tier >= 6) return L"6: ドロップ率 +18% / エリートは追加ドロップ / 宝箱B以上確定";
        if (tier >= 4) return L"4: ドロップ率 +12% / 高レア率上昇";
        return L"2: ドロップ率 +8%";
    }

    return L"";
}

//------------------------------------------------------------
// 説明文の組み立て
//------------------------------------------------------------
std::wstring BuildSynergyDescription(
    SynergyType value,
    int count,
    int tier
)
{
    std::wstring text = ToString(value);

    text += L"  ";
    text += std::to_wstring(count);

    if (tier > 0)
    {
        text += L" 個（";
        text += std::to_wstring(tier);
        text += L" 発動中）";
    }
    else
    {
        //----------------------------------------------------
        // まだ発動していないときは、あと何個必要かを出す
        //----------------------------------------------------
        int need = 2 - count;

        if (need < 1)
        {
            need = 1;
        }

        text += L" 個（あと ";
        text += std::to_wstring(need);
        text += L" 個で発動）";
    }

    text += L"\n";
    text += GetSynergyThemeText(value);
    text += L"\n";

    //--------------------------------------------------------
    // 2 → 4 → 6 の順に並べる。
    // 今の段階には印を付けて分かるようにする。
    //--------------------------------------------------------
    const int steps[3] = { 2, 4, 6 };

    for (int i = 0; i < 3; ++i)
    {
        const int step = steps[i];

        text += (tier == step) ? L"? " : L"　 ";
        text += GetSynergyTierText(value, step);

        if (i < 2)
        {
            text += L"\n";
        }
    }

    return text;
}

const wchar_t* ToString(RandomOptionType v)
{
    switch (v)
    {
    case RandomOptionType::AttackPercent:    return L"攻撃力";
    case RandomOptionType::CritRate:         return L"クリティカル率";
    case RandomOptionType::CritDamage:       return L"クリティカルダメージ";
    case RandomOptionType::FireRate:         return L"連射速度";
    case RandomOptionType::ReloadSpeed:      return L"リロード速度";
    case RandomOptionType::MagazineSize:     return L"装弾数";
    case RandomOptionType::HeadShotDamage:   return L"ヘッドショットダメージ";
    case RandomOptionType::ArmorPenetration: return L"装甲貫通";
    case RandomOptionType::MaxHP:            return L"最大HP";
    case RandomOptionType::Defense:          return L"防御力";
    case RandomOptionType::DamageReduction:  return L"ダメージ軽減";
    case RandomOptionType::Shield:           return L"シールド";
    case RandomOptionType::ReserveAmmo:      return L"予備弾薬";
    case RandomOptionType::AmmoDrop:         return L"弾薬ドロップ";
    case RandomOptionType::RecoilControl:    return L"反動制御";
    case RandomOptionType::WeaponHandling:   return L"武器取り回し";
    case RandomOptionType::MoveSpeed:        return L"移動速度";
    case RandomOptionType::Dodge:            return L"回避率";
    case RandomOptionType::DashCooldown:     return L"ダッシュ再使用時間";
    case RandomOptionType::DashDistance:     return L"ダッシュ距離";
    case RandomOptionType::DetectionRange:   return L"索敵範囲";
    }
    return L"不明";
}

const wchar_t* ToString(WeaponType v)
{
    switch (v)
    {
    case WeaponType::AssaultRifle:      return L"アサルトライフル";
    case WeaponType::BurstRifle:        return L"バーストライフル";
    case WeaponType::BattleRifle:       return L"バトルライフル";
    case WeaponType::PumpShotgun:       return L"ポンプ式ショットガン";
    case WeaponType::AutoShotgun:       return L"オートショットガン";
    case WeaponType::ScoutRifle:        return L"スカウトライフル";
    case WeaponType::BoltActionRifle:   return L"ボルトアクションライフル";
    case WeaponType::AntiMaterialRifle: return L"対物ライフル";
    case WeaponType::LightMachineGun:   return L"軽機関銃";
    case WeaponType::HeavyMachineGun:   return L"重機関銃";
    case WeaponType::CombatPistol:      return L"コンバットピストル";
    case WeaponType::AutoPistol:        return L"オートピストル";
    case WeaponType::HeavyRevolver:     return L"リボルバー";
    case WeaponType::MagnumRevolver:    return L"マグナム";
    case WeaponType::CompactSMG:        return L"コンパクト短機関銃";
    case WeaponType::TacticalSMG:       return L"タクティカル短機関銃";
    case WeaponType::VampireSMG:        return L"ヴァンパイア短機関銃";
    }
    return L"不明";
}

const wchar_t* ToString(ArmorSlot v)
{
    switch (v)
    {
    case ArmorSlot::Head:  return L"頭部";
    case ArmorSlot::Body:  return L"胴体";
    case ArmorSlot::Leg:   return L"脚部";
    case ArmorSlot::Arm:   return L"腕部";
    case ArmorSlot::Boots: return L"ブーツ";
    }
    return L"不明";
}

const wchar_t* ToString(StageType v)
{
    switch (v)
    {
    case StageType::Forest: return L"森林";
    case StageType::Suburb: return L"平原";
    case StageType::Town:   return L"郊外";
    case StageType::Boss:   return L"ボス";
    }
    return L"不明";
}

const wchar_t* ToString(DropSource v)
{
    switch (v)
    {
    case DropSource::NormalEnemy:   return L"兵士";
    case DropSource::EliteEnemy:    return L"隊長";
    case DropSource::TreasureChest: return L"宝箱";
    case DropSource::Boss:          return L"ボス";
    }
    return L"不明";
}

const wchar_t* GetStageName(StageType stage)
{
    switch (stage)
    {
    case StageType::Forest: return L"ステージ1 - 森林";
    case StageType::Suburb: return L"ステージ2 - 平原";
    case StageType::Town:   return L"ステージ3 - 郊外";
    case StageType::Boss:   return L"最終 - ボス";
    }

    return L"ステージ";
}

RarityColor GetRarityColor(Rarity v)
{
    switch (v)
    {
    case Rarity::Common:    return { 0.62f, 0.66f, 0.70f };  // グレー
    case Rarity::Uncommon:  return { 0.45f, 0.85f, 0.45f };  // グリーン
    case Rarity::Rare:      return { 0.30f, 0.65f, 1.00f };  // ブルー
    case Rarity::Special:   return { 0.72f, 0.45f, 1.00f };  // パープル
    case Rarity::Epic:      return { 1.00f, 0.45f, 0.85f };  // ピンク
    case Rarity::Legendary: return { 1.00f, 0.72f, 0.15f };  // ゴールド
    case Rarity::Mythic:    return { 1.00f, 0.30f, 0.20f };  // レッド
    }
    return { 1.0f, 1.0f, 1.0f };
}

bool IsReductionOption(RandomOptionType v)
{
    // 設計書 10. BOOTS：Dash Cooldown だけ「マイナス表記」
    return v == RandomOptionType::DashCooldown;
}

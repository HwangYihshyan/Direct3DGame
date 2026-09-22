/*--------------------------------------------------------------------

ファイル名：ItemInfoUI.cpp
お仕事　　：装備の情報表示

設計書 Ver.3 19-6.2

----------------------------------------------------------------------*/

#include "ItemInfoUI.h"
#include "Rarity.h"

#include <cmath>
#include <cstdio>
#include <vector>

namespace
{
    //--------------------------------------------------------
    // 同じ種類のランダムOPを合計する
    //--------------------------------------------------------
    float SumOption(
        const std::vector<RandomOption>& options,
        RandomOptionType type
    )
    {
        float total = 0.0f;

        for (const RandomOption& option : options)
        {
            if (option.type == type)
            {
                total += option.value;
            }
        }

        return total;
    }

    bool HasOption(
        const std::vector<RandomOption>& options,
        RandomOptionType type
    )
    {
        for (const RandomOption& option : options)
        {
            if (option.type == type)
            {
                return true;
            }
        }

        return false;
    }

    //--------------------------------------------------------
    // ステータス1行を作る
    //
    //   label   … 項目名
    //   value   … ランダムOP込みの最終値
    //   bonus   … そのうちランダムOPによる上乗せ（%）
    //
    // ランダムOPを別欄にせず、
    // 最終値のうしろへ括弧書きで内訳を出す。
    //--------------------------------------------------------
    std::wstring StatRow(
        const wchar_t* label,
        const std::wstring& value,
        float bonus,
        bool bonusIsMinus = false
    )
    {
        std::wstring line = label;

        //----------------------------------------------------
        // 項目名の幅をそろえる。
        //
        // 項目名を日本語にしたので、文字数で数えると
        // 全角ぶんだけ右へずれてしまう。
        // 全角を2、半角を1として数えて桁をそろえる。
        //----------------------------------------------------
        auto visualWidth = [](const std::wstring& text)
        {
            size_t width = 0;

            for (wchar_t c : text)
            {
                width += (c > 0x7F) ? 2 : 1;
            }

            return width;
        };

        while (visualWidth(line) < 24)
        {
            line += L' ';
        }

        line += value;

        if (bonus > 0.001f)
        {
            wchar_t buffer[48];

            swprintf_s(
                buffer,
                bonusIsMinus ? L"   (追加 -%.1f%%)" : L"   (追加 +%.1f%%)",
                bonus
            );

            line += buffer;
        }

        line += L'\n';

        return line;
    }

    std::wstring FormatNumber(float value, int decimals)
    {
        wchar_t buffer[64];

        if (decimals <= 0)
        {
            swprintf_s(buffer, L"%.0f", value);
        }
        else if (decimals == 1)
        {
            swprintf_s(buffer, L"%.1f", value);
        }
        else
        {
            swprintf_s(buffer, L"%.2f", value);
        }

        return buffer;
    }

    //--------------------------------------------------------
    // 武器の説明文
    //
    // 設計書 Ver.3 19-6.2 の「Base Stat / Random OP」は
    // 別欄に分けず、1つの STATUS 欄へまとめて表示する。
    //--------------------------------------------------------
    std::wstring FormatWeapon(const GeneratedWeapon& weapon)
    {
        const WeaponData& data = weapon.data;
        const std::vector<RandomOption>& ops = data.randomOptions;

        std::wstring text;

        text += data.name;
        text += L"\n";

        text +=
            (data.category == WeaponCategory::Main)
            ? L"メイン武器\n"
            : L"サブ武器\n";

        text += L"\n― ステータス ―\n";

        //----------------------------------------------------
        // 攻撃力（レアリティ倍率 ＋ Attack%）
        //----------------------------------------------------
        const float attackOp = SumOption(ops, RandomOptionType::AttackPercent);

        const float attack =
            weapon.finalAttack * (1.0f + attackOp * 0.01f);

        text += StatRow(L"攻撃", FormatNumber(attack, 0), attackOp);

        //----------------------------------------------------
        // 連射速度
        //----------------------------------------------------
        const float fireOp = SumOption(ops, RandomOptionType::FireRate);

        const float rpm =
            data.roundsPerMinute * (1.0f + fireOp * 0.01f);

        text += StatRow(
            L"連射速度",
            FormatNumber(rpm, 0) + L" 発/分",
            fireOp
        );

        //----------------------------------------------------
        // マガジン
        //----------------------------------------------------
        const float magOp = SumOption(ops, RandomOptionType::MagazineSize);

        const float magazine =
            data.magazineSize * (1.0f + magOp * 0.01f);

        text += StatRow(L"弾数", FormatNumber(magazine, 0), magOp);

        //----------------------------------------------------
        // リロード
        //----------------------------------------------------
        const float reloadOp = SumOption(ops, RandomOptionType::ReloadSpeed);

        const float reload =
            data.reloadTime / (1.0f + reloadOp * 0.01f);

        text += StatRow(
            L"リロード時間",
            FormatNumber(reload, 2) + L" 秒",
            reloadOp
        );

        //----------------------------------------------------
        // Crit（基礎5% / +50%）
        //----------------------------------------------------
        const float critRateOp = SumOption(ops, RandomOptionType::CritRate);
        const float critDmgOp = SumOption(ops, RandomOptionType::CritDamage);

        text += StatRow(
            L"クリティカル率",
            FormatNumber(5.0f + critRateOp, 1) + L"%",
            critRateOp
        );

        text += StatRow(
            L"クリティカルダメージ",
            L"+" + FormatNumber(50.0f + critDmgOp, 0) + L"%",
            critDmgOp
        );

        //----------------------------------------------------
        // HeadShot
        //----------------------------------------------------
        const float headOp = SumOption(ops, RandomOptionType::HeadShotDamage);

        text += StatRow(
            L"ヘッドショット",
            L"×" + FormatNumber(data.headShotMultiplier, 1),
            headOp
        );

        //----------------------------------------------------
        // 貫通・散弾・移動速度は持っている武器だけ出す
        //----------------------------------------------------
        const float penOp = SumOption(ops, RandomOptionType::ArmorPenetration);

        if (penOp > 0.001f)
        {
            text += StatRow(
                L"攻撃貫通",
                FormatNumber(penOp, 1) + L"%",
                penOp
            );
        }

        if (data.pelletCount > 1)
        {
            text += StatRow(
                L"散弾数",
                std::to_wstring(data.pelletCount),
                0.0f
            );
        }

        if (data.pierceCount > 0)
        {
            text += StatRow(
                L"貫通",
                (data.pierceCount >= 99)
                    ? std::wstring(L"全弾")
                    : std::to_wstring(data.pierceCount),
                0.0f
            );
        }

        if (data.moveSpeedScale < 0.999f)
        {
            text += StatRow(
                L"移動速度",
                L"×" + FormatNumber(data.moveSpeedScale, 2),
                0.0f
            );
        }

        // シナジーはアイコンで別に表示するので、ここには出さない

        if (weapon.mythicEffect != MythicEffectType::None)
        {
            text += L"\n― ミシック ―\n";
            text += GetMythicName(weapon.mythicEffect);
            text += L"\n";
            text += GetMythicDescription(weapon.mythicEffect);
            text += L"\n";
        }

        return text;
    }

    //--------------------------------------------------------
    // 防具の説明文
    //--------------------------------------------------------
    std::wstring FormatArmor(const GeneratedArmor& armor)
    {
        const ArmorData& data = armor.data;
        const std::vector<RandomOption>& ops = data.randomOptions;

        std::wstring text;

        text += data.name;
        text += L"\n";

        text += L"装備\n";

        text += L"\n― ステータス ―\n";

        //----------------------------------------------------
        // その部位のメインステータス
        // 同じ種類のランダムOPがあれば合算して1行にする
        //----------------------------------------------------
        const float mainOp = SumOption(ops, data.mainStatType);

        if (data.mainStatType == RandomOptionType::MaxHP)
        {
            // BODY だけ実HP加算なので、OPは%として掛ける
            const float hp = data.mainStatValue * (1.0f + mainOp * 0.01f);

            text += StatRow(
                ToString(data.mainStatType),
                L"+" + FormatNumber(hp, 0),
                mainOp
            );
        }
        else
        {
            text += StatRow(
                ToString(data.mainStatType),
                L"+" + FormatNumber(data.mainStatValue + mainOp, 1) + L"%",
                mainOp
            );
        }

        //----------------------------------------------------
        // 防御力
        //----------------------------------------------------
        const float defenseOp =
            (data.mainStatType == RandomOptionType::Defense)
            ? 0.0f
            : SumOption(ops, RandomOptionType::Defense);

        const float defense =
            data.baseDefense * (1.0f + defenseOp * 0.01f);

        text += StatRow(
            L"防御力",
            L"+" + FormatNumber(defense, 0),
            defenseOp
        );

        //----------------------------------------------------
        // 残りのランダムOPも同じ欄に続けて並べる
        //----------------------------------------------------
        for (const RandomOption& option : ops)
        {
            if (option.type == data.mainStatType)
            {
                continue;
            }

            if (option.type == RandomOptionType::Defense)
            {
                continue;
            }

            const bool minus = IsReductionOption(option.type);

            text += StatRow(
                ToString(option.type),
                (minus ? L"-" : L"+") + FormatNumber(option.value, 1) + L"%",
                option.value,
                minus
            );
        }

        (void)HasOption;

        if (armor.mythicEffect != MythicEffectType::None)
        {
            text += L"\n― ミシック ―\n";
            text += GetMythicName(armor.mythicEffect);
            text += L"\n";
            text += GetMythicDescription(armor.mythicEffect);
            text += L"\n";
        }

        return text;
    }

}

//============================================================
// 説明文
//============================================================

std::wstring ItemInfoUI::BuildText(const EquipmentDrop& item)
{
    if (!item.dropped)
    {
        return L"";
    }

    return item.isWeapon
        ? FormatWeapon(item.weapon)
        : FormatArmor(item.armor);
}

//------------------------------------------------------------
// 能力値をアイコン付きの行へ
//------------------------------------------------------------
std::vector<ItemInfoUI::StatRowView> ItemInfoUI::BuildStatRows(
    const EquipmentDrop& item
)
{
    std::vector<StatRowView> rows;

    if (!item.dropped)
    {
        return rows;
    }

    const auto add = [&rows](int icon, const std::wstring& value,
                             float raw, bool lower = false)
        {
            rows.push_back({ icon, value, raw, lower });
        };

    if (item.isWeapon)
    {
        const WeaponData& data = item.weapon.data;
        const std::vector<RandomOption>& ops = data.randomOptions;

        const float atkOp = SumOption(ops, RandomOptionType::AttackPercent);
        const float atk = item.weapon.finalAttack * (1.0f + atkOp * 0.01f);
        add(0, FormatNumber(atk, 0), atk);

        const float fireOp = SumOption(ops, RandomOptionType::FireRate);
        const float rpm = data.roundsPerMinute * (1.0f + fireOp * 0.01f);
        add(1, FormatNumber(rpm, 0) + L" 発/分", rpm);

        const float magOp = SumOption(ops, RandomOptionType::MagazineSize);
        const float mag = data.magazineSize * (1.0f + magOp * 0.01f);
        add(2, FormatNumber(mag, 0), mag);

        const float relOp = SumOption(ops, RandomOptionType::ReloadSpeed);
        const float rel = data.reloadTime / (1.0f + relOp * 0.01f);
        add(3, FormatNumber(rel, 2) + L" 秒", rel, true);

        const float crOp = SumOption(ops, RandomOptionType::CritRate);
        add(4, FormatNumber(5.0f + crOp, 1) + L"%", 5.0f + crOp);

        const float cdOp = SumOption(ops, RandomOptionType::CritDamage);
        add(5, L"+" + FormatNumber(50.0f + cdOp, 0) + L"%", 50.0f + cdOp);

        const float hsOp = SumOption(ops, RandomOptionType::HeadShotDamage);
        const float hs = data.headShotMultiplier * (1.0f + hsOp * 0.01f);
        add(6, L"×" + FormatNumber(hs, 2), hs);

        const float penOp = SumOption(ops, RandomOptionType::ArmorPenetration);

        if (penOp > 0.001f)
        {
            add(7, FormatNumber(penOp, 1) + L"%", penOp);
        }
    }
    else
    {
        const ArmorData& data = item.armor.data;
        const std::vector<RandomOption>& ops = data.randomOptions;

        //----------------------------------------------------
        // OPの種類からアイコン番号を引く
        //----------------------------------------------------
        const auto iconOf = [](RandomOptionType type)
            {
                switch (type)
                {
                case RandomOptionType::AttackPercent:    return 0;
                case RandomOptionType::FireRate:         return 1;
                case RandomOptionType::MagazineSize:     return 2;
                case RandomOptionType::ReloadSpeed:      return 3;
                case RandomOptionType::CritRate:         return 4;
                case RandomOptionType::CritDamage:       return 5;
                case RandomOptionType::HeadShotDamage:   return 6;
                case RandomOptionType::ArmorPenetration: return 7;
                case RandomOptionType::MaxHP:            return 8;
                case RandomOptionType::Defense:          return 9;
                case RandomOptionType::DamageReduction:  return 10;
                case RandomOptionType::Shield:           return 11;
                case RandomOptionType::ReserveAmmo:      return 12;
                case RandomOptionType::AmmoDrop:         return 13;
                case RandomOptionType::RecoilControl:    return 14;
                case RandomOptionType::WeaponHandling:   return 15;
                case RandomOptionType::MoveSpeed:        return 16;
                case RandomOptionType::Dodge:            return 17;
                case RandomOptionType::DashCooldown:     return 18;
                case RandomOptionType::DashDistance:     return 19;
                case RandomOptionType::DetectionRange:   return 4;
                }

                return 9;
            };

        const float mainOp = SumOption(ops, data.mainStatType);

        if (data.mainStatType == RandomOptionType::MaxHP)
        {
            const float hp = data.mainStatValue * (1.0f + mainOp * 0.01f);
            add(iconOf(data.mainStatType), L"+" + FormatNumber(hp, 0), hp);
        }
        else
        {
            const float v = data.mainStatValue + mainOp;
            add(iconOf(data.mainStatType), L"+" + FormatNumber(v, 1) + L"%", v);
        }

        add(9, L"+" + FormatNumber(data.baseDefense, 0), data.baseDefense);

        for (const RandomOption& option : ops)
        {
            if (option.type == data.mainStatType)
            {
                continue;
            }

            const bool minus = IsReductionOption(option.type);

            add(iconOf(option.type),
                (minus ? L"-" : L"+") + FormatNumber(option.value, 1) + L"%",
                option.value);
        }
    }

    return rows;
}

std::vector<SynergyType> ItemInfoUI::GetSynergies(const EquipmentDrop& item)
{
    if (!item.dropped)
    {
        return {};
    }

    return item.isWeapon
        ? item.weapon.data.synergies
        : item.armor.data.synergies;
}

std::wstring ItemInfoUI::BuildEquippedText(
    const EquipmentDrop& incoming,
    const PlayerLoadout& loadout
)
{
    if (!incoming.dropped)
    {
        return L"";
    }

    if (incoming.isWeapon)
    {
        const bool isMain =
            (incoming.weapon.data.category == WeaponCategory::Main);

        if (isMain)
        {
            if (!loadout.HasMainWeapon())
            {
                return L"（メイン武器なし）";
            }

            return FormatWeapon(loadout.GetMainWeapon());
        }

        if (!loadout.HasSubWeapon())
        {
            return L"（サブ武器なし）";
        }

        return FormatWeapon(loadout.GetSubWeapon());
    }

    const ArmorSlot slot = incoming.armor.data.slot;

    if (!loadout.HasArmor(slot))
    {
        return std::wstring(L"（") + ToString(slot) + L" 未装備）";
    }

    return FormatArmor(loadout.GetArmor(slot));
}

//============================================================
// 比較パネル
//============================================================

void ItemInfoUI::DrawCompare(
    const UIRenderer& ui,
    const EquipmentDrop& item,
    const PlayerLoadout& loadout,
    bool isNew
) const
{
    if (!ui.IsReady() || !item.dropped)
    {
        return;
    }

    constexpr float LEFT = 690.0f;
    constexpr float TOP = 76.0f;
    constexpr float RIGHT = 1262.0f;
    constexpr float BOTTOM = 512.0f;
    constexpr float SPLIT = 974.0f;

    ui.FillPanel(LEFT, TOP, RIGHT, BOTTOM, 10.0f);

    ui.SetRarityColor(item.GetRarity(), 0.9f);
    ui.DrawRound(LEFT, TOP, RIGHT, BOTTOM, 10.0f, 2.0f);

    // レアリティは色の帯で示す（英字は使わない）
    ui.SetRarityColor(item.GetRarity(), 1.0f);
    ui.FillRound(LEFT + 8.0f, TOP + 4.0f, SPLIT - 8.0f, TOP + 10.0f, 3.0f);

    //--------------------------------------------------------
    // 見出し
    //--------------------------------------------------------
    //--------------------------------------------------------
    // NEW は初めて手にする種類のときだけ出す
    //--------------------------------------------------------
    if (isNew)
    {
        ui.SetColor(0.60f, 0.90f, 0.65f, 1.0f);

        ui.DrawText(L"◆ NEW", LEFT + 16.0f, TOP + 10.0f,
                    SPLIT, TOP + 34.0f, ui.Normal());
    }


    ui.SetColor(0.60f, 0.66f, 0.72f, 1.0f);
    ui.DrawText(L"装備中", SPLIT + 12.0f, TOP + 10.0f, RIGHT - 12.0f, TOP + 34.0f, ui.Normal());

    ui.SetColor(0.30f, 0.38f, 0.46f, 0.8f);
    ui.DrawLine(SPLIT, TOP + 8.0f, SPLIT, BOTTOM - 8.0f, 1.0f);

    //--------------------------------------------------------
    // シナジーはアイコンで並べる
    //--------------------------------------------------------
    constexpr float ICON = 38.0f;
    constexpr float ICON_TOP = TOP + 40.0f;

    const std::vector<SynergyType> newSynergies = GetSynergies(item);

    for (size_t i = 0; i < newSynergies.size(); ++i)
    {
        ui.DrawSynergyIcon(
            newSynergies[i],
            LEFT + 16.0f + i * (ICON + 6.0f),
            ICON_TOP,
            ICON
        );
    }

    EquipmentDrop equipped;

    equipped.dropped = false;

    if (item.isWeapon)
    {
        const bool isMain =
            (item.weapon.data.category == WeaponCategory::Main);

        if (isMain && loadout.HasMainWeapon())
        {
            equipped.dropped = true;
            equipped.isWeapon = true;
            equipped.weapon = loadout.GetMainWeapon();
        }
        else if (!isMain && loadout.HasSubWeapon())
        {
            equipped.dropped = true;
            equipped.isWeapon = true;
            equipped.weapon = loadout.GetSubWeapon();
        }
    }
    else if (loadout.HasArmor(item.armor.data.slot))
    {
        equipped.dropped = true;
        equipped.isWeapon = false;
        equipped.armor = loadout.GetArmor(item.armor.data.slot);
    }

    const std::vector<SynergyType> oldSynergies = GetSynergies(equipped);

    for (size_t i = 0; i < oldSynergies.size(); ++i)
    {
        ui.DrawSynergyIcon(
            oldSynergies[i],
            SPLIT + 12.0f + i * (ICON + 6.0f),
            ICON_TOP,
            ICON,
            0.75f
        );
    }

    //--------------------------------------------------------
    // 能力値はアイコンと数値で並べ、
    // 装備中より強ければ赤い上向き矢印、
    // 弱ければ緑の下向き矢印を後ろへ出す。
    //--------------------------------------------------------
    const float bodyTop = ICON_TOP + ICON + 12.0f;

    const std::vector<StatRowView> newRows = BuildStatRows(item);
    const std::vector<StatRowView> oldRows = BuildStatRows(equipped);

    constexpr float STAT = 30.0f;
    constexpr float ROW = 36.0f;

    for (size_t i = 0; i < newRows.size(); ++i)
    {
        const StatRowView& row = newRows[i];
        const float y = bodyTop + i * ROW;

        if (y + STAT > BOTTOM - 8.0f)
        {
            break;
        }

        ui.DrawStatIcon(row.iconIndex, LEFT + 16.0f, y, STAT);

        //----------------------------------------------------
        // 拾うかどうかを決める場面なので、
        // ここでは説明を出さない。
        // 能力の説明は LOADOUT 画面で見られる。
        //----------------------------------------------------

        ui.SetColor(0.94f, 0.97f, 1.0f, 1.0f);

        ui.DrawText(
            row.value,
            LEFT + 54.0f, y + 4.0f, SPLIT - 40.0f, y + STAT,
            ui.Small()
        );

        //----------------------------------------------------
        // 装備中の同じ項目と比べる
        //----------------------------------------------------
        for (const StatRowView& old : oldRows)
        {
            if (old.iconIndex != row.iconIndex)
            {
                continue;
            }

            if (std::fabs(old.raw - row.raw) < 0.001f)
            {
                break;
            }

            const bool stronger = row.lowerIsBetter
                ? (row.raw < old.raw)
                : (row.raw > old.raw);

            ui.DrawCompareArrow(SPLIT - 24.0f, y + STAT * 0.5f, stronger);
            break;
        }
    }

    //--------------------------------------------------------
    // 装備中も同じ形で並べる
    //--------------------------------------------------------
    for (size_t i = 0; i < oldRows.size(); ++i)
    {
        const float y = bodyTop + i * ROW;

        if (y + STAT > BOTTOM - 8.0f)
        {
            break;
        }

        ui.DrawStatIcon(oldRows[i].iconIndex, SPLIT + 12.0f, y, STAT, 0.75f);

        ui.SetColor(0.72f, 0.78f, 0.84f, 1.0f);

        ui.DrawText(
            oldRows[i].value,
            SPLIT + 50.0f, y + 4.0f, RIGHT - 10.0f, y + STAT,
            ui.Small()
        );
    }

    if (!equipped.dropped)
    {
        ui.SetColor(0.60f, 0.66f, 0.72f, 1.0f);

        ui.DrawText(
            L"（未装備）",
            SPLIT + 12.0f, bodyTop, RIGHT - 10.0f, bodyTop + 30.0f,
            ui.Small()
        );
    }
}

//============================================================
// 装備一覧
//============================================================

void ItemInfoUI::DrawLoadout(
    const UIRenderer& ui,
    const PlayerLoadout& loadout,
    float screenWidth,
    float screenHeight,
    float mouseX,
    float mouseY,
    std::wstring* outTooltip
) const
{
    if (!ui.IsReady())
    {
        return;
    }

    ui.SetColor(0.0f, 0.0f, 0.0f, 0.78f);
    ui.FillRect(0.0f, 0.0f, screenWidth, screenHeight);

    ui.SetColor(0.95f, 0.97f, 1.0f, 1.0f);
    ui.DrawText(L"ポーズ中...", 36.0f, 10.0f, 300.0f, 44.0f, ui.Header());

    //--------------------------------------------------------
    // シナジー10種を横一列に。
    // 持っていないものは暗く出し、
    // マウスを乗せると下に説明が出る。
    //--------------------------------------------------------
    {
        const SynergySystem& synergy = loadout.GetSynergy();

        constexpr float SYN_ICON = 34.0f;
        constexpr float SYN_STEP = 62.0f;
        constexpr float SYN_LEFT = 320.0f;
        constexpr float SYN_TOP = 8.0f;

        for (int i = 0; i < SYNERGY_COUNT; ++i)
        {
            const SynergyType type = static_cast<SynergyType>(i);

            const int count = synergy.GetCount(type);
            const int tier = synergy.GetTier(type);

            const float x = SYN_LEFT + i * SYN_STEP;

            float alpha = 0.22f;

            if (tier > 0)       alpha = 1.0f;
            else if (count > 0) alpha = 0.55f;

            if (mouseX >= x && mouseX <= x + SYN_ICON &&
                mouseY >= SYN_TOP && mouseY <= SYN_TOP + SYN_ICON)
            {
                ui.SetColor(1.0f, 0.95f, 0.55f, 0.9f);

                ui.DrawRound(x - 3.0f, SYN_TOP - 3.0f,
                             x + SYN_ICON + 3.0f, SYN_TOP + SYN_ICON + 3.0f,
                             7.0f, 2.0f);

                if (outTooltip)
                {
                    *outTooltip =
                        BuildSynergyDescription(type, count, tier);
                }
            }

            ui.DrawSynergyIcon(type, x, SYN_TOP, SYN_ICON, alpha);

            //------------------------------------------------
            // アイコンの下に個数
            //------------------------------------------------
            if (tier > 0)
            {
                ui.SetColor(1.00f, 0.86f, 0.35f, 1.0f);
            }
            else if (count > 0)
            {
                ui.SetColor(0.62f, 0.66f, 0.72f, 1.0f);
            }
            else
            {
                ui.SetColor(0.34f, 0.37f, 0.42f, 1.0f);
            }

            std::wstring label = std::to_wstring(count);

            if (tier > 0)
            {
                label += L"/" + std::to_wstring(tier);
            }

            ui.DrawText(label, x - 6.0f, SYN_TOP + SYN_ICON,
                        x + SYN_ICON + 6.0f, SYN_TOP + SYN_ICON + 22.0f,
                        ui.Small());
        }
    }

    //--------------------------------------------------------
    // 装備カード（武器2 ＋ 防具5）
    //--------------------------------------------------------
    struct SlotView
    {
        std::wstring label;
        Rarity rarity;
        bool equipped;
        std::vector<SynergyType> synergies;

        // 能力値はアイコンで並べる
        std::vector<StatRowView> stats;
        std::wstring name;
        std::wstring extra;      // MYTHIC など、文字で出すもの
    };

    //--------------------------------------------------------
    // 装備1個ぶんの表示内容を作る
    //--------------------------------------------------------
    const auto makeWeaponView =
        [](const wchar_t* label, const GeneratedWeapon& weapon)
        {
            EquipmentDrop drop;

            drop.dropped = true;
            drop.isWeapon = true;
            drop.weapon = weapon;

            SlotView view;

            view.label = label;
            view.rarity = weapon.data.rarity;
            view.equipped = true;
            view.synergies = weapon.data.synergies;
            view.stats = BuildStatRows(drop);
            view.name = weapon.data.name;

            if (weapon.mythicEffect != MythicEffectType::None)
            {
                view.extra = GetMythicName(weapon.mythicEffect);
            }

            return view;
        };

    const auto makeArmorView =
        [](const wchar_t* label, const GeneratedArmor& armor)
        {
            EquipmentDrop drop;

            drop.dropped = true;
            drop.isWeapon = false;
            drop.armor = armor;

            SlotView view;

            view.label = label;
            view.rarity = armor.data.rarity;
            view.equipped = true;
            view.synergies = armor.data.synergies;
            view.stats = BuildStatRows(drop);
            view.name = armor.data.name;

            if (armor.mythicEffect != MythicEffectType::None)
            {
                view.extra = GetMythicName(armor.mythicEffect);
            }

            return view;
        };

    const auto makeEmptyView =
        [](const wchar_t* label)
        {
            SlotView view;

            view.label = label;
            view.rarity = Rarity::Common;
            view.equipped = false;
            view.name = L"未装備";

            return view;
        };

    std::vector<SlotView> slots;

    slots.push_back(
        loadout.HasMainWeapon()
        ? makeWeaponView(L"メイン", loadout.GetMainWeapon())
        : makeEmptyView(L"メイン")
    );

    slots.push_back(
        loadout.HasSubWeapon()
        ? makeWeaponView(L"サブ", loadout.GetSubWeapon())
        : makeEmptyView(L"サブ")
    );

    for (int i = 0; i < ARMOR_SLOT_COUNT; ++i)
    {
        const ArmorSlot slot = static_cast<ArmorSlot>(i);

        slots.push_back(
            loadout.HasArmor(slot)
            ? makeArmorView(ToString(slot), loadout.GetArmor(slot))
            : makeEmptyView(ToString(slot))
        );
    }

    constexpr float CARD_TOP = 70.0f;
    constexpr float CARD_BOTTOM = 536.0f;
    constexpr float CARD_WIDTH = 168.0f;
    constexpr float CARD_GAP = 8.0f;
    constexpr float CARD_ICON = 34.0f;

    float x = 36.0f;

    for (const SlotView& view : slots)
    {
        const float right = x + CARD_WIDTH;

        ui.SetColor(0.03f, 0.05f, 0.07f, 0.94f);
        ui.FillRound(x, CARD_TOP, right, CARD_BOTTOM, 6.0f);

        if (view.equipped)
        {
            ui.SetRarityColor(view.rarity, 0.9f);
        }
        else
        {
            ui.SetColor(0.25f, 0.28f, 0.32f, 0.9f);
        }

        ui.DrawRound(x, CARD_TOP, right, CARD_BOTTOM, 6.0f, 1.5f);

        //----------------------------------------------------
        // レアリティは色の帯で示す
        //----------------------------------------------------
        if (view.equipped)
        {
            ui.SetRarityColor(view.rarity, 1.0f);
            ui.FillRound(x + 8.0f, CARD_TOP + 5.0f,
                         right - 8.0f, CARD_TOP + 11.0f, 3.0f);
        }

        ui.SetColor(0.55f, 0.72f, 0.90f, 1.0f);
        ui.DrawText(view.label, x + 10.0f, CARD_TOP + 14.0f,
                    right - 8.0f, CARD_TOP + 40.0f, ui.Normal());

        //----------------------------------------------------
        // その装備が持つシナジー
        //----------------------------------------------------
        for (size_t i = 0; i < view.synergies.size(); ++i)
        {
            ui.DrawSynergyIcon(
                view.synergies[i],
                x + 10.0f + i * (CARD_ICON + 6.0f),
                CARD_TOP + 44.0f,
                CARD_ICON
            );
        }

        //----------------------------------------------------
        // 名前
        //----------------------------------------------------
        ui.SetColor(0.90f, 0.94f, 1.0f, 1.0f);
        ui.DrawText(view.name, x + 10.0f, CARD_TOP + 84.0f,
                    right - 8.0f, CARD_TOP + 128.0f, ui.Small());

        //----------------------------------------------------
        // 能力値は画面下と同じアイコンで並べる
        //----------------------------------------------------
        constexpr float STAT_ICON = 26.0f;
        constexpr float STAT_ROW = 32.0f;

        float statY = CARD_TOP + 132.0f;

        for (const StatRowView& row : view.stats)
        {
            if (statY + STAT_ICON > CARD_BOTTOM - 26.0f)
            {
                break;
            }

            ui.DrawStatIcon(row.iconIndex, x + 10.0f, statY, STAT_ICON);

            //------------------------------------------------
            // アイコンにマウスを乗せたら説明を出す
            //------------------------------------------------
            if (mouseX >= x + 10.0f && mouseX <= x + 10.0f + STAT_ICON &&
                mouseY >= statY && mouseY <= statY + STAT_ICON)
            {
                ui.SetColor(1.0f, 0.95f, 0.55f, 0.9f);

                ui.DrawRound(x + 7.0f, statY - 3.0f,
                             x + 13.0f + STAT_ICON, statY + STAT_ICON + 3.0f,
                             7.0f, 2.0f);

                if (outTooltip)
                {
                    std::wstring tip = GetStatName(row.iconIndex);
                    tip += L"\n";
                    tip += GetStatDescription(row.iconIndex);

                    *outTooltip = tip;
                }
            }

            ui.SetColor(0.88f, 0.93f, 1.0f, 1.0f);

            ui.DrawText(row.value, x + 14.0f + STAT_ICON, statY + 3.0f,
                        right - 8.0f, statY + STAT_ICON, ui.Small());

            statY += STAT_ROW;
        }

        //----------------------------------------------------
        // MYTHIC は文字で添える
        //----------------------------------------------------
        if (!view.extra.empty())
        {
            ui.SetRarityColor(Rarity::Mythic, 1.0f);

            ui.DrawText(view.extra, x + 10.0f, CARD_BOTTOM - 26.0f,
                        right - 8.0f, CARD_BOTTOM - 4.0f, ui.Small());
        }

        x += CARD_WIDTH + CARD_GAP;
    }

    //--------------------------------------------------------
    // 合計ステータス
    //   アイコンと数値を格子に並べる。
    //   マウスを乗せると下に説明が出る。
    //--------------------------------------------------------
    constexpr float STAT_TOP = 546.0f;
    constexpr float STAT_BOTTOM = 688.0f;
    constexpr float STAT_LEFT = 36.0f;
    constexpr float STAT_RIGHT = 1244.0f;

    ui.SetColor(0.03f, 0.05f, 0.08f, 0.92f);
    ui.FillRound(STAT_LEFT, STAT_TOP, STAT_RIGHT, STAT_BOTTOM, 8.0f);

    ui.SetColor(0.28f, 0.55f, 0.82f, 0.7f);
    ui.DrawRound(STAT_LEFT, STAT_TOP, STAT_RIGHT, STAT_BOTTOM, 8.0f, 1.2f);

    const std::vector<StatLine> lines = loadout.GetStats().BuildDisplayList();

    constexpr int COLUMNS = 5;
    constexpr float ICON = 30.0f;
    constexpr float COL_WIDTH = (STAT_RIGHT - STAT_LEFT - 24.0f) / COLUMNS;
    constexpr float ROW_HEIGHT = 44.0f;

    for (size_t i = 0; i < lines.size(); ++i)
    {
        const StatLine& line = lines[i];

        const int col = static_cast<int>(i) % COLUMNS;
        const int row = static_cast<int>(i) / COLUMNS;

        const float cellX = STAT_LEFT + 12.0f + col * COL_WIDTH;
        const float cellY = STAT_TOP + 6.0f + row * ROW_HEIGHT;

        ui.DrawStatIcon(line.iconIndex, cellX, cellY, ICON);

        if (mouseX >= cellX && mouseX <= cellX + ICON &&
            mouseY >= cellY && mouseY <= cellY + ICON)
        {
            ui.SetColor(1.0f, 0.95f, 0.55f, 0.9f);

            ui.DrawRound(cellX - 3.0f, cellY - 3.0f,
                         cellX + ICON + 3.0f, cellY + ICON + 3.0f, 7.0f, 2.0f);

            if (outTooltip)
            {
                std::wstring tip = GetStatName(line.iconIndex);
                tip += L"　／　";
                tip += GetStatDescription(line.iconIndex);

                *outTooltip = tip;
            }
        }

        ui.SetColor(0.86f, 0.92f, 1.0f, 1.0f);

        std::wstring value = line.isPercent
            ? (L"+" + FormatNumber(line.value, 1) + L"%")
            : FormatNumber(line.value, 0);

        ui.DrawText(
            value,
            cellX + ICON + 8.0f, cellY + 4.0f,
            cellX + COL_WIDTH - 10.0f, cellY + ICON,
            ui.Small()
        );
    }

    ui.SetColor(0.55f, 0.62f, 0.70f, 1.0f);
    ui.DrawText(L"TAB : 閉じる", 1080.0f, 12.0f, 1244.0f, 40.0f, ui.Small());
}

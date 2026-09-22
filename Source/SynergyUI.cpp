/*--------------------------------------------------------------------

ファイル名：SynergyUI.cpp
お仕事　　：シナジーとSSS固有能力の表示

----------------------------------------------------------------------*/
#define NOMINMAX
#include "SynergyUI.h"

#include <algorithm>
#include <cstdio>
#include <vector>

namespace
{
    constexpr float PANEL_WIDTH = 232.0f;

    // 発動段階ごとの色
    void SetTierColor(const UIRenderer& ui, int tier)
    {
        if (tier >= 6)      ui.SetColor(1.00f, 0.72f, 0.15f, 1.0f);
        else if (tier >= 4) ui.SetColor(0.72f, 0.45f, 1.00f, 1.0f);
        else if (tier >= 2) ui.SetColor(0.45f, 0.85f, 0.55f, 1.0f);
        else                ui.SetColor(0.48f, 0.53f, 0.58f, 1.0f);
    }

    // 次の発動段階まであと何個か
    int 
        TierNeed(int count)
    {
        if (count < 2) return 2 - count;
        if (count < 4) return 4 - count;
        if (count < 6) return 6 - count;

        return 0;
    }
}

void SynergyUI::Draw(
    const UIRenderer& ui,
    const SynergySystem& synergy,
    const MythicRuntime& mythic,
    float left,
    float top,
    float mouseX,
    float mouseY,
    std::wstring* outTooltip
) const
{
    if (!ui.IsReady())
    {
        return;
    }

    const float bottom = DrawSynergyPanel(
        ui, synergy, left, top, mouseX, mouseY, outTooltip
    );

    DrawMythicPanel(ui, mythic, left, bottom + 10.0f);
}

//------------------------------------------------------------
// シナジー一覧
// 戻り値はパネルの下端
//------------------------------------------------------------
float SynergyUI::DrawSynergyPanel(
    const UIRenderer& ui,
    const SynergySystem& synergy,
    float left,
    float top,
    float mouseX,
    float mouseY,
    std::wstring* outTooltip
) const
{
    //--------------------------------------------------------
    // 持っているシナジーだけを、発動段階の高い順に並べる。
    // 全種類を並べると画面を占めてしまうため。
    //--------------------------------------------------------
    struct Entry
    {
        SynergyType type;
        int count;
        int tier;
    };

    std::vector<Entry> entries;

    for (int i = 0; i < SYNERGY_COUNT; ++i)
    {
        const SynergyType type = static_cast<SynergyType>(i);
        const int count = synergy.GetCount(type);

        if (count <= 0)
        {
            continue;
        }

        entries.push_back({ type, count, synergy.GetTier(type) });
    }

    if (entries.empty())
    {
        return top;
    }

    std::sort(
        entries.begin(), entries.end(),
        [](const Entry& a, const Entry& b)
        {
            if (a.tier != b.tier) return a.tier > b.tier;
            return a.count > b.count;
        }
    );

    //--------------------------------------------------------
    // 数が多いときは行を詰めて、下の説明欄に届かないようにする。
    //   持っているシナジーが増えるほど1行が薄くなる。
    //--------------------------------------------------------
    constexpr float MAX_BOTTOM = 440.0f;
    constexpr float HEADER = 38.0f;

    const float count = static_cast<float>(entries.size());

    float row = (MAX_BOTTOM - top - HEADER) / std::max(count, 1.0f);

    row = std::clamp(row, 26.0f, 40.0f);

    const float ICON = row - 6.0f;
    const float ROW = row;

    const float height = HEADER + count * ROW;
    const float right = left + PANEL_WIDTH;
    const float bottom = top + height;

    ui.FillPanel(left, top, right, bottom, 8.0f);

    ui.SetColor(0.20f, 0.55f, 0.85f, 0.7f);
    ui.DrawRound(left, top, right, bottom, 8.0f, 1.2f);

    ui.SetColor(0.65f, 0.80f, 0.95f, 1.0f);

    ui.DrawText(
        L"シナジー",
        left + 12.0f, top + 6.0f, right - 8.0f, top + 34.0f,
        ui.Normal()
    );

    float y = top + 38.0f;

    for (const Entry& entry : entries)
    {
        const float iconLeft = left + 10.0f;

        const float alpha = (entry.tier > 0) ? 1.0f : 0.45f;

        const bool hovered =
            (mouseX >= iconLeft && mouseX <= right - 6.0f &&
             mouseY >= y && mouseY <= y + ICON);

        if (hovered)
        {
            ui.SetColor(1.0f, 0.95f, 0.60f, 0.85f);

            ui.DrawRound(
                iconLeft - 3.0f, y - 3.0f,
                right - 6.0f, y + ICON + 3.0f,
                8.0f, 2.0f
            );

            if (outTooltip)
            {
                *outTooltip = BuildSynergyDescription(
                    entry.type, entry.count, entry.tier
                );
            }
        }

        ui.DrawSynergyIcon(entry.type, iconLeft, y, ICON, alpha);

        if (!ui.HasIcons())
        {
            SetTierColor(ui, entry.tier);

            ui.DrawText(
                ToString(entry.type),
                iconLeft, y + 6.0f, iconLeft + ICON + 60.0f, y + ICON,
                ui.Small()
            );
        }

        SetTierColor(ui, entry.tier);

        std::wstring value = L"×";
        value += std::to_wstring(entry.count);

        if (entry.tier > 0)
        {
            value += L"   ";
            value += std::to_wstring(entry.tier);
            value += L"発動";
        }

        ui.DrawText(
            value,
            iconLeft + ICON + 10.0f, y + 5.0f, right - 6.0f, y + ICON,
            ui.Small()
        );

        y += ROW;
    }

    return bottom;
}


//------------------------------------------------------------
// SSS固有能力
//------------------------------------------------------------
void SynergyUI::DrawMythicPanel(
    const UIRenderer& ui,
    const MythicRuntime& mythic,
    float left,
    float top
) const
{
    if (!mythic.HasAny())
    {
        return;
    }

    const std::vector<MythicEffectType>& list = mythic.GetEquipped();

    const float height = 32.0f + list.size() * 22.0f;
    const float right = left + PANEL_WIDTH;
    const float bottom = top + height;

    ui.FillPanel(left, top, right, bottom, 8.0f);

    ui.SetRarityColor(Rarity::Mythic, 0.9f);
    ui.DrawRound(left, top, right, bottom, 8.0f, 1.5f);

    ui.DrawText(
        L"ミシック",
        left + 12.0f, top + 6.0f, right - 8.0f, top + 28.0f,
        ui.Normal()
    );

    float y = top + 30.0f;

    for (MythicEffectType type : list)
    {
        //----------------------------------------------------
        // 効果時間があるものは残り秒数も出す
        //----------------------------------------------------
        float remaining = 0.0f;

        switch (type)
        {
        case MythicEffectType::OverclockCore:
            remaining = mythic.GetOverclockTimer();
            break;

        case MythicEffectType::DeadeyeProtocol:
            remaining = mythic.GetDeadeyeTimer();
            break;

        case MythicEffectType::EndlessMagazine:
            remaining = mythic.GetEndlessMagazineTimer();
            break;

        default:
            break;
        }

        std::wstring line = GetMythicName(type);

        if (remaining > 0.0f)
        {
            ui.SetColor(1.0f, 0.90f, 0.35f, 1.0f);

            wchar_t buffer[32];
            swprintf_s(buffer, L"  %.1f秒", remaining);

            line += buffer;
        }
        else
        {
            ui.SetColor(0.72f, 0.60f, 0.55f, 1.0f);
        }

        ui.DrawText(
            line,
            left + 12.0f, y, right - 4.0f, y + 20.0f,
            ui.Small()
        );

        y += 22.0f;
    }
}

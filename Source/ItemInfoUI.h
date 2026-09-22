/*--------------------------------------------------------------------

ファイル名：ItemInfoUI.h
お仕事　　：拾った装備の情報表示と、装備中との比較

設計書 Ver.3 19-6.2
    Rarity / Name / Weapon・Armor / Base Stat /
    Random OP / Synergy / Mythic Effect

Random OP は別欄にせず、Base Stat と同じ STATUS 欄へまとめる。

----------------------------------------------------------------------*/

#ifndef ITEMINFOUI_H
#define ITEMINFOUI_H

#include <string>

#include "DropSystem.h"
#include "PlayerLoadout.h"
#include "SynergySystem.h"
#include "UIRenderer.h"

#include <vector>

class ItemInfoUI
{
public:
    //--------------------------------------------------------
    // 説明文の組み立て
    //--------------------------------------------------------
    static std::wstring BuildText(const EquipmentDrop& item);

    // その装備が持つシナジー（アイコン表示に使う）
    static std::vector<SynergyType> GetSynergies(const EquipmentDrop& item);

    //--------------------------------------------------------
    // 能力値1行ぶん
    //   iconIndex … stat_icons.png の何番目か
    //   raw       … 比較に使う数値
    //   lowerIsBetter … 数値が小さいほど強い項目か
    //--------------------------------------------------------
    struct StatRowView
    {
        int iconIndex = 0;
        std::wstring value;
        float raw = 0.0f;
        bool lowerIsBetter = false;
    };

    static std::vector<StatRowView> BuildStatRows(const EquipmentDrop& item);

    // 同じ枠に装備しているものの説明文
    static std::wstring BuildEquippedText(
        const EquipmentDrop& incoming,
        const PlayerLoadout& loadout
    );

    //--------------------------------------------------------
    // 拾える装備と装備中を左右に並べて比べる
    //--------------------------------------------------------
    //--------------------------------------------------------
    // 拾える装備と装備中を左右に並べて比べる
    //   拾うかどうかを決める場面なので、説明は出さない
    //--------------------------------------------------------
    void DrawCompare(
        const UIRenderer& ui,
        const EquipmentDrop& item,
        const PlayerLoadout& loadout,
        bool isNew
    ) const;

    //--------------------------------------------------------
    // 装備一覧（TAB）
    //--------------------------------------------------------
    //--------------------------------------------------------
    // 装備一覧
    //   マウスが乗ったステータスの説明を outTooltip へ返す
    //--------------------------------------------------------
    void DrawLoadout(
        const UIRenderer& ui,
        const PlayerLoadout& loadout,
        float screenWidth,
        float screenHeight,
        float mouseX,
        float mouseY,
        std::wstring* outTooltip
    ) const;
};

#endif

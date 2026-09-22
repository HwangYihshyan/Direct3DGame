/*--------------------------------------------------------------------

ファイル名：SynergyUI.h
お仕事　　：シナジーとSSS固有能力の表示

設計書 Ver.3 19-6.3
    10 Synergy Icon / 現在Count / 次Tierまで必要数 / 発動中2・4・6

表示のみを行い、ゲームの内部データは書き換えない。

----------------------------------------------------------------------*/

#ifndef SYNERGYUI_H
#define SYNERGYUI_H

#include "MythicEffect.h"
#include "SynergySystem.h"
#include "UIRenderer.h"

#include <string>

class SynergyUI
{
public:
    // 画面左に「発動しているシナジー」と「MYTHIC」を縦に並べる
    //--------------------------------------------------------
    // マウスが乗ったアイコンの説明を outTooltip へ書き出す
    //--------------------------------------------------------
    void Draw(
        const UIRenderer& ui,
        const SynergySystem& synergy,
        const MythicRuntime& mythic,
        float left,
        float top,
        float mouseX,
        float mouseY,
        std::wstring* outTooltip
    ) const;

private:
    float DrawSynergyPanel(
        const UIRenderer& ui,
        const SynergySystem& synergy,
        float left,
        float top,
        float mouseX,
        float mouseY,
        std::wstring* outTooltip
    ) const;

    void DrawMythicPanel(
        const UIRenderer& ui,
        const MythicRuntime& mythic,
        float left,
        float top
    ) const;
};

#endif

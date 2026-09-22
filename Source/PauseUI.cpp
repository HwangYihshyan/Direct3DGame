/*--------------------------------------------------------------------

ファイル名：PauseUI.cpp
お仕事　　：ポーズ画面

----------------------------------------------------------------------*/

#include "PauseUI.h"
#include "AudioManager.h"

#include <cmath>

namespace
{
    const wchar_t* ITEMS[] =
    {
        L"再開",
        L"設定",
        L"再スタート",
        L"タイトル画面へ"
    };

    constexpr float ITEM_WIDTH = 300.0f;
    constexpr float ITEM_HEIGHT = 52.0f;
    constexpr float ITEM_GAP = 14.0f;
    // 項目が4つになったので、少し上から並べる
    constexpr float FIRST_TOP = 300.0f;

    float ItemTop(int index)
    {
        return FIRST_TOP + index * (ITEM_HEIGHT + ITEM_GAP);
    }
}

void PauseUI::Open()
{
    m_open = true;
    m_selected = 0;
    m_blink = 0.0f;
}

void PauseUI::Close()
{
    m_open = false;
}

PauseResult PauseUI::Update(
    const InputTracker& input,
    float mouseX,
    float mouseY,
    float deltaTime
)
{
    if (!m_open)
    {
        return PauseResult::None;
    }

    m_blink += deltaTime;

    //--------------------------------------------------------
    // Escでそのまま復帰できる
    //--------------------------------------------------------
    if (input.IsPressed(VK_ESCAPE))
    {
        Audio::PlaySe(SeId::MenuBack);

        Close();
        return PauseResult::Resume;
    }

    //--------------------------------------------------------
    // キーボードだけで操作する
    //--------------------------------------------------------
    (void)mouseX;
    (void)mouseY;

    //--------------------------------------------------------
    // キーボード
    //--------------------------------------------------------
    const int before = m_selected;

    if (input.IsPressed(VK_UP) || input.IsPressed('W'))
    {
        m_selected = (m_selected - 1 + ITEM_COUNT) % ITEM_COUNT;
    }

    if (input.IsPressed(VK_DOWN) || input.IsPressed('S'))
    {
        m_selected = (m_selected + 1) % ITEM_COUNT;
    }

    if (m_selected != before)
    {
        Audio::PlaySe(SeId::MenuMove);
    }

    const bool decided =
        input.IsPressed(VK_RETURN) || input.IsPressed('J');

    if (!decided)
    {
        return PauseResult::None;
    }

    // 「再開」は戻る音、それ以外は決定音
    Audio::PlaySe((m_selected == 0) ? SeId::MenuBack : SeId::MenuDecide);

    //--------------------------------------------------------
    // 「設定」だけはポーズを開いたままにする。
    // 設定画面から戻ってきたとき、いきなりゲームが
    // 動き出すのではなく、このポーズ画面へ帰ってくる。
    //--------------------------------------------------------
    if (m_selected == 1)
    {
        return PauseResult::Option;
    }

    Close();

    switch (m_selected)
    {
    case 0:  return PauseResult::Resume;
    case 2:  return PauseResult::Restart;
    default: return PauseResult::Title;
    }
}

void PauseUI::Draw(
    const UIRenderer& ui,
    float screenWidth,
    float screenHeight
) const
{
    if (!m_open || !ui.IsReady())
    {
        return;
    }

    //--------------------------------------------------------
    // 背景を暗くする
    //--------------------------------------------------------
    ui.SetColor(0.0f, 0.0f, 0.0f, 0.68f);
    ui.FillRect(0.0f, 0.0f, screenWidth, screenHeight);

    ui.SetColor(0.92f, 0.96f, 1.0f, 1.0f);

    ui.DrawText(
        L"ポーズ",
        0.0f, 200.0f, screenWidth, 280.0f,
        ui.Big()
    );

    const float left = screenWidth * 0.5f - ITEM_WIDTH * 0.5f;
    const float right = screenWidth * 0.5f + ITEM_WIDTH * 0.5f;

    for (int i = 0; i < ITEM_COUNT; ++i)
    {
        const float top = ItemTop(i);
        const float bottom = top + ITEM_HEIGHT;

        const bool selected = (i == m_selected);

        if (selected)
        {
            // ゆっくり明滅させる。文字の明るさは変えない
            const float pulse =
                0.5f + 0.5f * std::sin(m_blink * 3.4f);

            ui.SetColor(0.10f, 0.62f, 1.00f, 0.20f + pulse * 0.20f);
            ui.FillRound(left, top, right, bottom, 8.0f);

            ui.SetColor(0.40f, 0.80f, 1.00f, 0.55f + pulse * 0.40f);
            ui.DrawRound(left, top, right, bottom, 8.0f, 2.0f);

            ui.SetColor(0.96f, 0.98f, 1.00f, 1.0f);
        }
        else
        {
            ui.SetColor(0.58f, 0.63f, 0.70f, 1.0f);
        }

        ui.DrawText(
            ITEMS[i],
            left + 24.0f, top + 12.0f, right - 24.0f, bottom,
            ui.Header()
        );
    }

    ui.SetColor(0.45f, 0.50f, 0.58f, 1.0f);

    ui.DrawText(
        L"Esc で再開",
        590.0f, 592.0f, screenWidth, 622.0f,
        ui.Normal()
    );
}

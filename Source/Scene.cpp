/*--------------------------------------------------------------------

ファイル名：Scene.cpp
お仕事　　：SceneContext と MenuController の実装

----------------------------------------------------------------------*/
#define NOMINMAX
#include "Scene.h"
#include "AudioManager.h"

#include <algorithm>
#include <cmath>

//============================================================
// SceneContext
//============================================================

void SceneContext::SetColor(float r, float g, float b, float a) const
{
    if (brush)
    {
        brush->SetColor(D2D1::ColorF(r, g, b, a));
        brush->SetOpacity(1.0f);
    }
}

void SceneContext::DrawText(
    const std::wstring& text,
    float left, float top, float right, float bottom,
    IDWriteTextFormat* format
) const
{
    if (!renderTarget || !format || !brush || text.empty())
    {
        return;
    }

    renderTarget->DrawText(
        text.c_str(),
        static_cast<UINT32>(text.size()),
        format,
        D2D1::RectF(left, top, right, bottom),
        brush
    );
}

void SceneContext::FillRect(
    float left, float top, float right, float bottom
) const
{
    if (!renderTarget || !brush)
    {
        return;
    }

    renderTarget->FillRectangle(
        D2D1::RectF(left, top, right, bottom),
        brush
    );
}

void SceneContext::FillRoundRect(
    float left, float top, float right, float bottom,
    float radius
) const
{
    if (!renderTarget || !brush)
    {
        return;
    }

    renderTarget->FillRoundedRectangle(
        D2D1::RoundedRect(
            D2D1::RectF(left, top, right, bottom), radius, radius
        ),
        brush
    );
}

void SceneContext::DrawRoundRect(
    float left, float top, float right, float bottom,
    float radius, float thickness
) const
{
    if (!renderTarget || !brush)
    {
        return;
    }

    renderTarget->DrawRoundedRectangle(
        D2D1::RoundedRect(
            D2D1::RectF(left, top, right, bottom), radius, radius
        ),
        brush,
        thickness
    );
}

//============================================================
// MenuController
//============================================================

void MenuController::SetItems(const std::vector<MenuItem>& items)
{
    m_items = items;

    if (m_selected >= static_cast<int>(m_items.size()))
    {
        m_selected = 0;
    }
}

void MenuController::SetLayout(
    float centerX, float topY, float width, float height, float gap
)
{
    m_centerX = centerX;
    m_topY = topY;
    m_width = width;
    m_height = height;
    m_gap = gap;
}

void MenuController::Reset()
{
    m_selected = 0;
    m_glow = 1.0f;
    m_blink = 0.0f;
    m_lastSelected = -1;
}

D2D1_RECT_F MenuController::GetItemRect(int index) const
{
    const float top = m_topY + index * (m_height + m_gap);

    return D2D1::RectF(
        m_centerX - m_width * 0.5f,
        top,
        m_centerX + m_width * 0.5f,
        top + m_height
    );
}

const MenuItem* MenuController::GetSelectedItem() const
{
    if (m_selected < 0 || m_selected >= static_cast<int>(m_items.size()))
    {
        return nullptr;
    }

    return &m_items[m_selected];
}

bool MenuController::Update(
    const InputTracker& input,
    float mouseX,
    float mouseY,
    float deltaTime
)
{
    if (m_items.empty())
    {
        return false;
    }

    const int count = static_cast<int>(m_items.size());

    //--------------------------------------------------------
    // 点滅させず、選択が移った瞬間だけ光らせて消えていく
    //--------------------------------------------------------
    m_glow = std::max(0.0f, m_glow - deltaTime * 2.6f);

    // 読みにくくならない程度にゆっくり点滅させる
    m_blink += deltaTime;

    //--------------------------------------------------------
    // メニューはキーボードだけで操作する
    //--------------------------------------------------------
    (void)mouseX;
    (void)mouseY;

    //--------------------------------------------------------
    // キーボード
    // 押した瞬間だけ動かすので、2項目飛ぶことはない
    //--------------------------------------------------------
    const int before = m_selected;

    if (input.IsPressed(VK_UP) || input.IsPressed('W'))
    {
        m_selected = (m_selected - 1 + count) % count;
    }

    if (input.IsPressed(VK_DOWN) || input.IsPressed('S'))
    {
        m_selected = (m_selected + 1) % count;
    }

    // 選択が動いた瞬間だけ鳴らす
    if (m_selected != before)
    {
        Audio::PlaySe(SeId::MenuMove);
    }

    //--------------------------------------------------------
    // 決定
    //--------------------------------------------------------
    if (m_selected != m_lastSelected)
    {
        m_lastSelected = m_selected;
        m_glow = 1.0f;
    }

    const bool decided =
        input.IsPressed(VK_RETURN);
       

    if (!decided)
    {
        return false;
    }

    const MenuItem* item = GetSelectedItem();

    const bool accepted = (item != nullptr) && item->enabled;

    if (accepted)
    {
        //----------------------------------------------------
        // 「戻る」だけ音を分ける。
        // 決定音が続くより、戻ったことが分かるほうが良い。
        //----------------------------------------------------
        Audio::PlaySe(
            (item->next == SceneType::Start)
            ? SeId::MenuBack
            : SeId::MenuDecide
        );
    }

    return accepted;
}

void MenuController::Draw(const SceneContext& context) const
{
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i)
    {
        const MenuItem& item = m_items[i];
        const D2D1_RECT_F rect = GetItemRect(i);

        const bool selected = (i == m_selected);

        if (selected)
        {
            //------------------------------------------------
            // 内側から外側へ薄くなる帯を重ねて、
            // ふわっと光ってから落ち着くように見せる
            //------------------------------------------------
            // 0.55Hz ほどでゆっくり明滅させる
            const float pulse =
                0.5f + 0.5f * std::sin(m_blink * 3.4f);

            const float base = 0.16f + pulse * 0.16f;
            const float flash = m_glow * m_glow * 0.45f;

            constexpr int STEPS = 5;

            for (int step = STEPS; step >= 1; --step)
            {
                const float t = static_cast<float>(step) / STEPS;
                const float spread = 14.0f * t * (0.5f + m_glow);
                const float alpha = (base + flash) * (1.0f - t) * 0.9f;

                context.SetColor(0.16f, 0.66f, 1.00f, alpha);

                context.FillRoundRect(
                    rect.left - spread, rect.top - spread * 0.5f,
                    rect.right + spread, rect.bottom + spread * 0.5f,
                    10.0f
                );
            }

            context.SetColor(
                0.35f, 0.78f, 1.00f,
                0.45f + pulse * 0.25f + m_glow * 0.30f
            );

            context.DrawRoundRect(
                rect.left, rect.top, rect.right, rect.bottom, 8.0f, 2.0f
            );
        }

        if (!item.enabled)
        {
            context.SetColor(0.32f, 0.34f, 0.38f, 1.0f);
        }
        else if (selected)
        {
            context.SetColor(0.96f, 0.98f, 1.00f, 1.0f);
        }
        else
        {
            context.SetColor(0.58f, 0.63f, 0.70f, 1.0f);
        }

        context.DrawText(
            item.label,
            rect.left, rect.top, rect.right, rect.bottom,
            context.menuFormat
        );
    }
}

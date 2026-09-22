/*--------------------------------------------------------------------

ファイル名：UIRenderer.cpp
お仕事　　：UI共通の描画処理

----------------------------------------------------------------------*/

#include "UIRenderer.h"

#include <algorithm>

void UIRenderer::Initialize(
    ID2D1RenderTarget* renderTarget,
    ID2D1SolidColorBrush* brush,
    ID2D1SolidColorBrush* panelBrush,
    IDWriteTextFormat* smallFormat,
    IDWriteTextFormat* normalFormat,
    IDWriteTextFormat* headerFormat,
    IDWriteTextFormat* bigFormat
)
{
    m_renderTarget = renderTarget;
    m_brush = brush;
    m_panelBrush = panelBrush;
    m_smallFormat = smallFormat;
    m_normalFormat = normalFormat;
    m_headerFormat = headerFormat;
    m_bigFormat = bigFormat;
}

//------------------------------------------------------------
// シナジーアイコン
// synergy_icons.png は 1セル 64x64 を横に10個並べたもの。
// 並び順は SynergyType の定義順と同じ。
//------------------------------------------------------------
void UIRenderer::DrawSynergyIcon(
    SynergyType type,
    float left,
    float top,
    float size,
    float alpha
) const
{
    if (!m_renderTarget || !m_iconBitmap)
    {
        return;
    }

    constexpr float CELL = 64.0f;

    const int index = static_cast<int>(type);

    if (index < 0 || index >= SYNERGY_COUNT)
    {
        return;
    }

    const D2D1_RECT_F source = D2D1::RectF(
        index * CELL, 0.0f, (index + 1) * CELL, CELL
    );

    m_renderTarget->DrawBitmap(
        m_iconBitmap,
        D2D1::RectF(left, top, left + size, top + size),
        alpha,
        D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
        source
    );
}

void UIRenderer::DrawSkillIcon(
    int index, float left, float top, float size, float alpha
) const
{
    if (!m_renderTarget || !m_skillIconBitmap || index < 0 || index > 3)
    {
        return;
    }

    constexpr float CELL = 64.0f;

    m_renderTarget->DrawBitmap(
        m_skillIconBitmap,
        D2D1::RectF(left, top, left + size, top + size),
        alpha,
        D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
        D2D1::RectF(index * CELL, 0.0f, (index + 1) * CELL, CELL)
    );
}

void UIRenderer::DrawStatIcon(
    int index, float left, float top, float size, float alpha
) const
{
    if (!m_renderTarget || !m_statIconBitmap || index < 0 || index > 19)
    {
        return;
    }

    constexpr float CELL = 48.0f;

    m_renderTarget->DrawBitmap(
        m_statIconBitmap,
        D2D1::RectF(left, top, left + size, top + size),
        alpha,
        D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
        D2D1::RectF(index * CELL, 0.0f, (index + 1) * CELL, CELL)
    );
}

//------------------------------------------------------------
// 装備中より強ければ赤い上向き、弱ければ緑の下向き
//------------------------------------------------------------
void UIRenderer::DrawCompareArrow(
    float centerX, float centerY, bool stronger
) const
{
    if (!m_renderTarget || !m_brush)
    {
        return;
    }

    if (stronger)
    {
        SetColor(1.00f, 0.32f, 0.28f, 1.0f);

        FillRect(centerX - 2.5f, centerY - 2.0f, centerX + 2.5f, centerY + 7.0f);

        m_renderTarget->FillRectangle(
            D2D1::RectF(centerX - 6.0f, centerY - 2.0f, centerX + 6.0f, centerY), m_brush);
        m_renderTarget->FillRectangle(
            D2D1::RectF(centerX - 4.0f, centerY - 5.0f, centerX + 4.0f, centerY - 2.0f), m_brush);
        m_renderTarget->FillRectangle(
            D2D1::RectF(centerX - 2.0f, centerY - 8.0f, centerX + 2.0f, centerY - 5.0f), m_brush);
    }
    else
    {
        SetColor(0.35f, 0.90f, 0.45f, 1.0f);

        FillRect(centerX - 2.5f, centerY - 7.0f, centerX + 2.5f, centerY + 2.0f);

        m_renderTarget->FillRectangle(
            D2D1::RectF(centerX - 6.0f, centerY, centerX + 6.0f, centerY + 2.0f), m_brush);
        m_renderTarget->FillRectangle(
            D2D1::RectF(centerX - 4.0f, centerY + 2.0f, centerX + 4.0f, centerY + 5.0f), m_brush);
        m_renderTarget->FillRectangle(
            D2D1::RectF(centerX - 2.0f, centerY + 5.0f, centerX + 2.0f, centerY + 8.0f), m_brush);
    }
}

void UIRenderer::SetColor(float r, float g, float b, float a) const
{
    if (m_brush)
    {
        m_brush->SetColor(D2D1::ColorF(r, g, b, a));
        m_brush->SetOpacity(1.0f);
    }
}

void UIRenderer::SetRarityColor(Rarity rarity, float alpha) const
{
    const RarityColor color = GetRarityColor(rarity);

    SetColor(color.r, color.g, color.b, alpha);
}

void UIRenderer::FillRect(float l, float t, float r, float b) const
{
    if (!m_renderTarget || !m_brush)
    {
        return;
    }

    m_renderTarget->FillRectangle(D2D1::RectF(l, t, r, b), m_brush);
}

void UIRenderer::FillPanel(
    float l, float t, float r, float b, float radius
) const
{
    if (!m_renderTarget || !m_panelBrush)
    {
        return;
    }

    m_renderTarget->FillRoundedRectangle(
        D2D1::RoundedRect(D2D1::RectF(l, t, r, b), radius, radius),
        m_panelBrush
    );
}

void UIRenderer::FillRound(
    float l, float t, float r, float b, float radius
) const
{
    if (!m_renderTarget || !m_brush)
    {
        return;
    }

    m_renderTarget->FillRoundedRectangle(
        D2D1::RoundedRect(D2D1::RectF(l, t, r, b), radius, radius),
        m_brush
    );
}

void UIRenderer::DrawRound(
    float l, float t, float r, float b, float radius, float thickness
) const
{
    if (!m_renderTarget || !m_brush)
    {
        return;
    }

    m_renderTarget->DrawRoundedRectangle(
        D2D1::RoundedRect(D2D1::RectF(l, t, r, b), radius, radius),
        m_brush,
        thickness
    );
}

void UIRenderer::DrawLine(
    float x1, float y1, float x2, float y2, float thickness
) const
{
    if (!m_renderTarget || !m_brush)
    {
        return;
    }

    m_renderTarget->DrawLine(
        D2D1::Point2F(x1, y1),
        D2D1::Point2F(x2, y2),
        m_brush,
        thickness
    );
}

void UIRenderer::DrawBar(
    float x, float y, float width, float height,
    float ratio, float r, float g, float b
) const
{
    ratio = std::clamp(ratio, 0.0f, 1.0f);

    SetColor(0.05f, 0.07f, 0.09f, 0.85f);
    FillRect(x, y, x + width, y + height);

    SetColor(r, g, b, 1.0f);
    FillRect(x, y, x + width * ratio, y + height);

    SetColor(0.35f, 0.42f, 0.50f, 0.9f);

    if (m_renderTarget && m_brush)
    {
        m_renderTarget->DrawRectangle(
            D2D1::RectF(x, y, x + width, y + height),
            m_brush,
            1.0f
        );
    }
}

void UIRenderer::DrawText(
    const std::wstring& text,
    float l, float t, float r, float b,
    IDWriteTextFormat* format
) const
{
    if (!m_renderTarget || !m_brush || !format || text.empty())
    {
        return;
    }

    m_renderTarget->DrawText(
        text.c_str(),
        static_cast<UINT32>(text.size()),
        format,
        D2D1::RectF(l, t, r, b),
        m_brush
    );
}

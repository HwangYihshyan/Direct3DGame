/*--------------------------------------------------------------------

ファイル名：ScoreDisplay.cpp
お仕事　　：スコア表示

----------------------------------------------------------------------*/
#define NOMINMAX
#include "ScoreDisplay.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace
{
    // 1秒あたり何割詰めるか
    constexpr float FOLLOW_SPEED = 6.0f;
}

void ScoreDisplay::Reset()
{
    m_displayed = 0.0f;
    m_punch = 0.0f;
    m_lastScore = 0;
}

void ScoreDisplay::Update(float deltaTime, const ShowScore& score)
{
    const int current = score.GetScore();

    //--------------------------------------------------------
    // 加点があったら一瞬だけ強調する
    //--------------------------------------------------------
    if (current > m_lastScore)
    {
        m_punch = 1.0f;
        m_lastScore = current;
    }

    m_punch = std::max(0.0f, m_punch - deltaTime * 3.0f);

    //--------------------------------------------------------
    // 表示はいきなり飛ばさず、追いつくように動かす
    //--------------------------------------------------------
    const float t = 1.0f - std::exp(-FOLLOW_SPEED * deltaTime);

    m_displayed += (current - m_displayed) * t;

    if (std::fabs(current - m_displayed) < 1.0f)
    {
        m_displayed = static_cast<float>(current);
    }
}

int ScoreDisplay::GetDisplayScore() const
{
    return static_cast<int>(m_displayed + 0.5f);
}

void ScoreDisplay::Draw(
    const UIRenderer& ui,
    const ShowScore& score,
    float screenWidth
) const
{
    if (!ui.IsReady())
    {
        return;
    }

    const float right = screenWidth - 20.0f;
    const float left = right - 260.0f;

    ui.FillPanel(left, 16.0f, right, 92.0f, 10.0f);

    //--------------------------------------------------------
    // 加点直後は枠が明るくなる
    //--------------------------------------------------------
    ui.SetColor(
        1.0f,
        0.80f + m_punch * 0.20f,
        0.30f + m_punch * 0.45f,
        0.55f + m_punch * 0.45f
    );

    ui.DrawRound(left, 16.0f, right, 92.0f, 10.0f, 1.6f);

    ui.SetColor(0.72f, 0.78f, 0.86f, 1.0f);

    ui.DrawText(L"Score", left + 14.0f, 20.0f, right - 14.0f, 42.0f, ui.Small());

    //--------------------------------------------------------
    // 数値
    //--------------------------------------------------------
    wchar_t buffer[64];
    swprintf_s(buffer, L"%d", GetDisplayScore());

    ui.SetColor(1.0f, 0.94f + m_punch * 0.06f, 0.62f + m_punch * 0.30f, 1.0f);

    ui.DrawText(buffer, left + 14.0f, 40.0f, right - 14.0f, 74.0f, ui.Header());

    //--------------------------------------------------------
    // コンボ
    //--------------------------------------------------------
    if (score.GetCombo() > 1)
    {
        swprintf_s(buffer, L"%d コンボ", score.GetCombo());

        ui.SetColor(0.60f, 1.00f, 0.75f, 1.0f);

        ui.DrawText(buffer, left + 14.0f, 70.0f, right - 14.0f, 90.0f, ui.Small());
    }
}

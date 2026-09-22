/*--------------------------------------------------------------------

ファイル名：GameOverScene.cpp
お仕事　　：ゲームオーバー画面

----------------------------------------------------------------------*/
#define NOMINMAX
#include "GameOverScene.h"
#include "ResultScene.h"
#include "CommonLevel.h"

#include <algorithm>
#include <cstdio>

GameOverScene::GameOverScene()
{
    m_menu.SetItems({
        { L"リトライ", SceneType::Game,  true },
        { L"タイトル画面へ", SceneType::Start, true }
        });

    m_menu.SetLayout(640.0f, 516.0f, 300.0f, 50.0f, 10.0f);
}

void GameOverScene::SetResult(const ResultData& result)
{
    m_result = result;

    //--------------------------------------------------------
    // ライフが尽きたらランキングへ進む
    //--------------------------------------------------------
    if (m_result.livesLeft > 0)
    {
        m_menu.SetItems({
            { L"リトライ", SceneType::Game,  true },
            { L"タイトル画面へ", SceneType::Start, true }
            });
    }
    else
    {
        m_menu.SetItems({
            { L"ランキング表示", SceneType::Ranking, true },
            { L"タイトル画面",   SceneType::Start,   true }
            });
    }

    m_menu.Reset();
}

void GameOverScene::Reset()
{
    m_menu.Reset();
    m_fade = 0.0f;
    m_wantsRetry = false;
}

SceneType GameOverScene::Update(
    const InputTracker& input,
    float mouseX,
    float mouseY,
    float deltaTime
)
{
    m_fade = std::min(m_fade + deltaTime * 1.6f, 1.0f);

    // 暗転しきるまでは入力を受けない
    if (m_fade < 0.6f)
    {
        return SceneType::None;
    }

    if (!m_menu.Update(input, mouseX, mouseY, deltaTime))
    {
        return SceneType::None;
    }

    m_wantsRetry =
        (m_result.livesLeft > 0) && (m_menu.GetSelected() == 0);

    const MenuItem* item = m_menu.GetSelectedItem();

    return item ? item->next : SceneType::None;
}

void GameOverScene::Draw(const SceneContext& context)
{
    //--------------------------------------------------------
    // 背景
    //--------------------------------------------------------
    if (context.gameOverBackground && context.renderTarget)
    {
        context.renderTarget->DrawBitmap(
            context.gameOverBackground,
            D2D1::RectF(0.0f, 0.0f, context.screenWidth, context.screenHeight),
            1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
        );

        context.SetColor(0.04f, 0.01f, 0.02f, 0.45f);
        context.FillRect(0.0f, 0.0f, context.screenWidth, context.screenHeight);
    }
    else
    {
        context.SetColor(0.03f, 0.01f, 0.02f, 1.0f);
        context.FillRect(0.0f, 0.0f, context.screenWidth, context.screenHeight);
    }

    //--------------------------------------------------------
    // 見出し
    //--------------------------------------------------------
    context.SetColor(0.95f, 0.32f, 0.32f, m_fade);

    context.DrawText(
        L"GAME OVER",
        0.0f, 60.0f, context.screenWidth, 150.0f,
        context.titleFormat
    );

    context.SetColor(0.82f, 0.86f, 0.92f, m_fade);

    std::wstring reached;
    reached += GetLevelSetting(m_result.stageIndex).displayName;

    context.DrawText(
        reached,
        0.0f, 152.0f, context.screenWidth, 186.0f,
        context.menuFormat
    );

    //--------------------------------------------------------
    // 大きくスコアを見せる
    //--------------------------------------------------------
    constexpr float CARD_TOP = 208.0f;
    constexpr float CARD_BOTTOM = 320.0f;
    constexpr float CARD_LEFT = 340.0f;
    constexpr float CARD_RIGHT = 940.0f;

    context.SetColor(0.06f, 0.02f, 0.04f, 0.86f * m_fade);
    context.FillRoundRect(CARD_LEFT, CARD_TOP, CARD_RIGHT, CARD_BOTTOM, 10.0f);

    context.SetColor(0.90f, 0.36f, 0.32f, 0.85f * m_fade);
    context.DrawRoundRect(CARD_LEFT, CARD_TOP, CARD_RIGHT, CARD_BOTTOM, 10.0f, 1.6f);

    context.SetColor(0.72f, 0.76f, 0.84f, m_fade);
    context.DrawText(L"Score", CARD_LEFT + 24.0f, CARD_TOP + 10.0f,
                     CARD_RIGHT - 24.0f, CARD_TOP + 38.0f, context.bodyFormat);

    {
        wchar_t buffer[64];
        swprintf_s(buffer, L"%d", m_result.score);

        context.SetColor(1.0f, 0.92f, 0.60f, m_fade);
        context.DrawText(buffer, CARD_LEFT + 24.0f, CARD_TOP + 34.0f,
                         CARD_RIGHT - 24.0f, CARD_BOTTOM - 10.0f,
                         context.titleFormat);
    }

    //--------------------------------------------------------
    // 主な戦績を3つだけ並べる
    //--------------------------------------------------------
    struct Item
    {
        const wchar_t* label;
        std::wstring value;
    };

    wchar_t timeText[32];
    const int total = static_cast<int>(m_result.clearTime);
    swprintf_s(timeText, L"%d:%02d", total / 60, total % 60);

    wchar_t killText[32];
    swprintf_s(killText, L"%d", m_result.killCount);

    wchar_t accText[32];
    swprintf_s(accText, L"%.0f%%", m_result.GetAccuracy() * 100.0f);

    const Item items[3] =
    {
        { L"生存時間", timeText },
        { L"撃破数",   killText },
        { L"命中率",   accText  }
    };

    constexpr float ITEM_TOP = 340.0f;
    constexpr float ITEM_BOTTOM = 428.0f;
    constexpr float ITEM_WIDTH = 190.0f;
    constexpr float ITEM_GAP = 15.0f;

    float x = 340.0f;

    for (const Item& item : items)
    {
        context.SetColor(0.05f, 0.02f, 0.03f, 0.82f * m_fade);
        context.FillRoundRect(x, ITEM_TOP, x + ITEM_WIDTH, ITEM_BOTTOM, 8.0f);

        context.SetColor(0.55f, 0.30f, 0.32f, 0.8f * m_fade);
        context.DrawRoundRect(x, ITEM_TOP, x + ITEM_WIDTH, ITEM_BOTTOM, 8.0f, 1.2f);

        context.SetColor(0.66f, 0.70f, 0.78f, m_fade);
        context.DrawText(item.label, x + 14.0f, ITEM_TOP + 8.0f,
                         x + ITEM_WIDTH - 14.0f, ITEM_TOP + 34.0f,
                         context.bodyFormat);

        context.SetColor(0.94f, 0.96f, 1.0f, m_fade);
        context.DrawText(item.value, x + 14.0f, ITEM_TOP + 34.0f,
                         x + ITEM_WIDTH - 14.0f, ITEM_BOTTOM - 8.0f,
                         context.menuFormat);

        x += ITEM_WIDTH + ITEM_GAP;
    }

    //--------------------------------------------------------
    // 残りライフ
    //--------------------------------------------------------
    {
        constexpr float LIFE_TOP = 440.0f;

        context.SetColor(0.72f, 0.76f, 0.84f, m_fade);

        context.DrawText(
            L"ライフ：",
            470.0f, LIFE_TOP, 560.0f, LIFE_TOP + 30.0f,
            context.bodyFormat
        );

        for (int i = 0; i < 3; ++i)
        {
            const float x = 566.0f + i * 40.0f;

            const bool alive = (i < m_result.livesLeft);

            if (alive)
            {
                context.SetColor(0.95f, 0.35f, 0.38f, m_fade);
            }
            else
            {
                context.SetColor(0.26f, 0.22f, 0.26f, m_fade);
            }

            //--------------------------------------------
            // ハート型のかわりに、丸を並べて残機を示す
            //--------------------------------------------
            context.FillRoundRect(x, LIFE_TOP + 4.0f,
                                  x + 26.0f, LIFE_TOP + 26.0f, 11.0f);
        }

        //context.SetColor(0.66f, 0.70f, 0.78f, m_fade);

        //if (m_result.livesLeft > 0)
        //{
        //    wchar_t buffer[64];
        //    swprintf_s(buffer, L"あと %d 回  できます", m_result.livesLeft);

        //    context.DrawText(buffer, 700.0f, LIFE_TOP, 940.0f,
        //                     LIFE_TOP + 30.0f, context.bodyFormat);
        //}
        //else
        //{
        //    context.DrawText(L"リトライはできません",
        //                     700.0f, LIFE_TOP, 940.0f,
        //                     LIFE_TOP + 30.0f, context.bodyFormat);
        //}
    }


    m_menu.Draw(context);
}

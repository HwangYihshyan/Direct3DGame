/*--------------------------------------------------------------------

ファイル名：GameClearScene.cpp
お仕事　　：ゲームクリア画面

----------------------------------------------------------------------*/
#define NOMINMAX
#include "GameClearScene.h"
#include "ResultScene.h"

#include <algorithm>
#include <cstdio>
#include <cmath>

GameClearScene::GameClearScene()
{
    m_menu.SetItems({
        { L"ランキング", SceneType::Ranking, true },
        { L"タイトル画面へ",   SceneType::Start,   true }
        });

    m_menu.SetLayout(640.0f, 604.0f, 280.0f, 46.0f, 8.0f);
}

void GameClearScene::SetResult(const ResultData& result)
{
    m_result = result;
}

void GameClearScene::Reset()
{
    m_menu.Reset();
    m_fade = 0.0f;
}

SceneType GameClearScene::Update(
    const InputTracker& input,
    float mouseX,
    float mouseY,
    float deltaTime
)
{
    m_fade = std::min(m_fade + deltaTime * 1.2f, 1.0f);

    if (m_fade < 0.6f)
    {
        return SceneType::None;
    }

    if (!m_menu.Update(input, mouseX, mouseY, deltaTime))
    {
        return SceneType::None;
    }

    const MenuItem* item = m_menu.GetSelectedItem();

    return item ? item->next : SceneType::None;
}

void GameClearScene::Draw(const SceneContext& context)
{
    //--------------------------------------------------------
    // 背景
    //--------------------------------------------------------
    if (context.gameClearBackground && context.renderTarget)
    {
        context.renderTarget->DrawBitmap(
            context.gameClearBackground,
            D2D1::RectF(0.0f, 0.0f, context.screenWidth, context.screenHeight),
            1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
        );

        context.SetColor(0.03f, 0.02f, 0.01f, 0.48f);
        context.FillRect(0.0f, 0.0f, context.screenWidth, context.screenHeight);
    }
    else
    {
        context.SetColor(0.03f, 0.03f, 0.02f, 1.0f);
        context.FillRect(0.0f, 0.0f, context.screenWidth, context.screenHeight);
    }

    //--------------------------------------------------------
    // 見出し
    //--------------------------------------------------------
    context.SetColor(1.00f, 0.84f, 0.32f, m_fade);

    context.DrawText(
        L"GAME CLEAR",
        0.0f, 44.0f, context.screenWidth, 130.0f,
        context.titleFormat
    );

    context.SetColor(0.86f, 0.90f, 0.96f, m_fade);

    context.DrawText(
        L"すべてのステージをクリアしました",
        0.0f, 132.0f, context.screenWidth, 166.0f,
        context.menuFormat
    );

    //--------------------------------------------------------
    // スコアを大きく1つだけ
    //--------------------------------------------------------
    constexpr float CARD_TOP = 184.0f;
    constexpr float CARD_BOTTOM = 292.0f;
    constexpr float CARD_LEFT = 340.0f;
    constexpr float CARD_RIGHT = 940.0f;

    context.SetColor(0.06f, 0.05f, 0.02f, 0.86f * m_fade);
    context.FillRoundRect(CARD_LEFT, CARD_TOP, CARD_RIGHT, CARD_BOTTOM, 10.0f);

    context.SetColor(1.00f, 0.78f, 0.32f, 0.85f * m_fade);
    context.DrawRoundRect(CARD_LEFT, CARD_TOP, CARD_RIGHT, CARD_BOTTOM, 10.0f, 1.6f);

    context.SetColor(0.76f, 0.78f, 0.84f, m_fade);
    context.DrawText(L"最終Score", CARD_LEFT + 24.0f, CARD_TOP + 8.0f,
                     CARD_RIGHT - 24.0f, CARD_TOP + 36.0f, context.bodyFormat);

    {
        wchar_t buffer[64];
        swprintf_s(buffer, L"%d", m_result.score);

        context.SetColor(1.0f, 0.94f, 0.66f, m_fade);
        context.DrawText(buffer, CARD_LEFT + 24.0f, CARD_TOP + 30.0f,
                         CARD_RIGHT - 24.0f, CARD_BOTTOM - 8.0f,
                         context.titleFormat);
    }

    //--------------------------------------------------------
    // 主な戦績を3枚のカードで
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

    const Item topRow[3] =
    {
        { L"クリア時間", timeText },
        { L"撃破数",     killText },
        { L"命中率",     accText  }
    };

    wchar_t comboText[32];
    swprintf_s(comboText, L"%d", m_result.maxCombo);

    wchar_t headText[32];
    swprintf_s(headText, L"%.0f%%", m_result.GetHeadShotRate() * 100.0f);

    wchar_t pickText[32];
    swprintf_s(pickText, L"%d", m_result.pickupCount);

    const Item bottomRow[3] =
    {
        { L"最大コンボ", comboText },
        { L"ヘッドショット率", headText  },
        { L"取得装備",   pickText  }
    };

    constexpr float ITEM_WIDTH = 190.0f;
    constexpr float ITEM_GAP = 15.0f;
    constexpr float ITEM_HEIGHT = 84.0f;

    const auto drawRow =
        [&](const Item* items, float top)
        {
            float x = 340.0f;

            for (int i = 0; i < 3; ++i)
            {
                context.SetColor(0.05f, 0.04f, 0.02f, 0.82f * m_fade);
                context.FillRoundRect(x, top, x + ITEM_WIDTH,
                                      top + ITEM_HEIGHT, 8.0f);

                context.SetColor(0.55f, 0.42f, 0.22f, 0.8f * m_fade);
                context.DrawRoundRect(x, top, x + ITEM_WIDTH,
                                      top + ITEM_HEIGHT, 8.0f, 1.2f);

                context.SetColor(0.70f, 0.72f, 0.78f, m_fade);
                context.DrawText(items[i].label, x + 14.0f, top + 6.0f,
                                 x + ITEM_WIDTH - 14.0f, top + 32.0f,
                                 context.bodyFormat);

                context.SetColor(0.94f, 0.96f, 1.0f, m_fade);
                context.DrawText(items[i].value, x + 14.0f, top + 32.0f,
                                 x + ITEM_WIDTH - 14.0f, top + ITEM_HEIGHT - 6.0f,
                                 context.menuFormat);

                x += ITEM_WIDTH + ITEM_GAP;
            }
        };

    drawRow(topRow, 306.0f);
    drawRow(bottomRow, 398.0f);

    //--------------------------------------------------------
    // 最高レアリティは色の帯で示す
    //--------------------------------------------------------
    {
        constexpr float BAR_TOP = 492.0f;

        context.SetColor(0.70f, 0.72f, 0.78f, m_fade);

        context.DrawText(
            L"最高レアリティ",
            420.0f, BAR_TOP, 540.0f, BAR_TOP + 28.0f,
            context.bodyFormat
        );

        const RarityColor color = GetRarityColor(m_result.bestRarity);

        context.SetColor(color.r, color.g, color.b, m_fade);
        context.FillRoundRect(548.0f, BAR_TOP + 6.0f,
                              688.0f, BAR_TOP + 24.0f, 6.0f);

        context.SetColor(0.70f, 0.72f, 0.78f, m_fade);

        wchar_t buffer[64];
        swprintf_s(buffer, L"SSS 取得 %d 個", m_result.mythicCount);

        context.DrawText(buffer, 706.0f, BAR_TOP, 880.0f, BAR_TOP + 28.0f,
                         context.bodyFormat);
    }

    //--------------------------------------------------------
    // 最後まで使った装備
    //--------------------------------------------------------
    {
        std::wstring build;

        for (size_t i = 0; i < m_result.equipmentNames.size(); ++i)
        {
            build += m_result.equipmentNames[i];

            if (i + 1 < m_result.equipmentNames.size())
            {
                build += L"　／　";
            }
        }

        if (!build.empty())
        {
            context.SetColor(0.60f, 0.64f, 0.72f, m_fade);

            context.DrawText(
                L"ビルド",
                0.0f, 526.0f, context.screenWidth, 550.0f,
                context.bodyFormat
            );

            context.SetColor(0.84f, 0.90f, 0.96f, m_fade);

            context.DrawText(
                build,
                240.0f, 550.0f, 1040.0f, 594.0f,
                context.bodyFormat
            );
        }
    }

    m_menu.Draw(context);
}

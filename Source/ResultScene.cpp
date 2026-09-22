/*--------------------------------------------------------------------

ファイル名：ResultScene.cpp
お仕事　　：ステージ終了後の結果表示

----------------------------------------------------------------------*/

#include "ResultScene.h"
#include "CommonLevel.h"

#include <cstdio>


ResultScene::ResultScene()
{
    m_menu.SetItems({
        { L"次のステージへ",  SceneType::Game,  true },
        { L"タイトル画面へ", SceneType::Start, true }
        });

    m_menu.SetLayout(640.0f, 566.0f, 260.0f, 44.0f, 8.0f);
}

void ResultScene::SetResult(const ResultData& result)
{
    m_result = result;
}

void ResultScene::Reset()
{
    m_menu.Reset();
    m_wantsNext = false;
    m_wantsRetry = false;
}

SceneType ResultScene::Update(
    const InputTracker& input,
    float mouseX,
    float mouseY,
    float deltaTime
)
{
    if (!m_menu.Update(input, mouseX, mouseY, deltaTime))
    {
        return SceneType::None;
    }

    const int selected = m_menu.GetSelected();

    m_wantsNext = (selected == 0);
    m_wantsRetry = (selected == 1);

    const MenuItem* item = m_menu.GetSelectedItem();

    return item ? item->next : SceneType::None;
}

void ResultScene::Draw(const SceneContext& context)
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

        context.SetColor(0.02f, 0.04f, 0.05f, 0.58f);
        context.FillRect(0.0f, 0.0f, context.screenWidth, context.screenHeight);
    }
    else
    {
        context.SetColor(0.02f, 0.04f, 0.07f, 1.0f);
        context.FillRect(0.0f, 0.0f, context.screenWidth, context.screenHeight);
    }

    //--------------------------------------------------------
    // 見出し
    //--------------------------------------------------------
    context.SetColor(0.45f, 0.95f, 0.60f, 1.0f);

    context.DrawText(
        L"ステージ クリア",
        0.0f, 110.0f, context.screenWidth, 200.0f,
        context.titleFormat
    );

    context.SetColor(0.82f, 0.88f, 0.94f, 1.0f);

    context.DrawText(
        GetLevelSetting(m_result.stageIndex).displayName,
        0.0f, 204.0f, context.screenWidth, 238.0f,
        context.menuFormat
    );

    //--------------------------------------------------------
    // 経過時間 / スコア / 命中率
    //
    // Game Over 画面と同じカードで並べている。
    //--------------------------------------------------------
    struct Item
    {
        const wchar_t* label;
        std::wstring value;
    };

    wchar_t timeText[32];
    const int total = static_cast<int>(m_result.clearTime);
    swprintf_s(timeText, L"%d:%02d", total / 60, total % 60);

    wchar_t scoreText[32];
    swprintf_s(scoreText, L"%d", m_result.score);

    wchar_t accText[32];
    swprintf_s(accText, L"%.0f%%", m_result.GetAccuracy() * 100.0f);

    const Item items[3] =
    {
        { L"経過時間", timeText  },
        { L"Score",   scoreText },
        { L"命中率",   accText   }
    };

    constexpr float ITEM_TOP = 286.0f;
    constexpr float ITEM_BOTTOM = 386.0f;
    constexpr float ITEM_WIDTH = 190.0f;
    constexpr float ITEM_GAP = 15.0f;

    float x = 340.0f;

    for (const Item& item : items)
    {
        context.SetColor(0.03f, 0.06f, 0.05f, 0.86f);
        context.FillRoundRect(x, ITEM_TOP, x + ITEM_WIDTH, ITEM_BOTTOM, 8.0f);

        context.SetColor(0.32f, 0.66f, 0.44f, 0.85f);
        context.DrawRoundRect(x, ITEM_TOP, x + ITEM_WIDTH, ITEM_BOTTOM, 8.0f, 1.2f);

        context.SetColor(0.70f, 0.76f, 0.82f, 1.0f);

        context.DrawText(
            item.label,
            x + 14.0f, ITEM_TOP + 10.0f, x + ITEM_WIDTH - 14.0f, ITEM_TOP + 38.0f,
            context.bodyFormat
        );

        context.SetColor(0.94f, 0.98f, 1.0f, 1.0f);

        context.DrawText(
            item.value,
            x + 14.0f, ITEM_TOP + 40.0f, x + ITEM_WIDTH - 14.0f, ITEM_BOTTOM - 10.0f,
            context.menuFormat
        );

        x += ITEM_WIDTH + ITEM_GAP;
    }

    m_menu.Draw(context);
}

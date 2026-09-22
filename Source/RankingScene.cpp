/*--------------------------------------------------------------------

ファイル名：RankingScene.cpp
お仕事　　：名前の入力とランキングの表示

名前はキーボードから直接入れる。
A～Z と 0～9、スペースだけを受け付ける。

----------------------------------------------------------------------*/

#include "RankingScene.h"

#include <cmath>
#include <cstdio>

RankingScene::RankingScene()
{
    m_ranking.Load();
}

void RankingScene::Begin(const ResultData& result)
{
    m_result = result;

    m_ranking.Load();

    m_name.clear();
    m_blink = 0.0f;
    m_rank = -1;

    //--------------------------------------------------------
    // 載らないスコアなら、名前を聞かずに一覧だけ見せる
    //--------------------------------------------------------
    m_phase = m_ranking.WillEnter(result.score)
        ? Phase::NameInput
        : Phase::List;
}

void RankingScene::BeginViewOnly()
{
    m_ranking.Load();
    m_ranking.ClearHighlight();

    m_phase = Phase::List;
    m_blink = 0.0f;
    m_rank = -1;
}

void RankingScene::Reset()
{
    m_blink = 0.0f;
}

//------------------------------------------------------------
// 名前の入力
//------------------------------------------------------------
void RankingScene::UpdateNameInput(const InputTracker& input)
{
    if (input.IsPressed(VK_BACK) &&
        !m_name.empty())
    {
        m_name.pop_back();
        return;
    }

    if (static_cast<int>(m_name.size()) >= Ranking::NAME_MAX)
    {
        return;
    }

    //--------------------------------------------------------
    // A～Z
    //--------------------------------------------------------
    for (int key = 'A'; key <= 'Z'; ++key)
    {
        if (input.IsPressed(key))
        {
            m_name += static_cast<wchar_t>(key);
            return;
        }
    }

    //--------------------------------------------------------
    // 0～9
    //--------------------------------------------------------
    for (int key = '0'; key <= '9'; ++key)
    {
        if (input.IsPressed(key))
        {
            m_name += static_cast<wchar_t>(key);
            return;
        }
    }

    if (input.IsPressed(VK_SPACE) && !m_name.empty())
    {
        m_name += L' ';
    }
}

void RankingScene::CommitName()
{
    RankingEntry entry;

    entry.name = m_name.empty() ? L"プレイヤー" : m_name;
    entry.score = m_result.score;
    entry.stageIndex = m_result.stageIndex;
    entry.cleared = m_result.cleared;

    m_rank = m_ranking.Add(entry);

    m_ranking.Save();

    m_phase = Phase::List;
}

SceneType RankingScene::Update(
    const InputTracker& input,
    float mouseX,
    float mouseY,
    float deltaTime
)
{
    (void)mouseX;
    (void)mouseY;

    m_blink += deltaTime;

    if (m_phase == Phase::NameInput)
    {
        if (input.IsPressed(VK_RETURN))
        {
            CommitName();
            return SceneType::None;
        }

        UpdateNameInput(input);

        return SceneType::None;
    }

    //--------------------------------------------------------
    // 一覧を見ている間は、戻る操作だけ
    //--------------------------------------------------------
    if (input.IsPressed(VK_RETURN) ||
        input.IsPressed(VK_ESCAPE) ||
        input.IsPressed('J'))
    {
        return SceneType::Start;
    }

    return SceneType::None;
}

//------------------------------------------------------------
// 共通の背景
//------------------------------------------------------------
namespace
{
    void DrawBackdrop(const SceneContext& context)
    {
        if (context.gameClearBackground && context.renderTarget)
        {
            context.renderTarget->DrawBitmap(
                context.gameClearBackground,
                D2D1::RectF(0.0f, 0.0f,
                            context.screenWidth, context.screenHeight),
                1.0f,
                D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
            );
        }

        context.SetColor(0.02f, 0.03f, 0.06f, 0.80f);
        context.FillRect(0.0f, 0.0f,
                         context.screenWidth, context.screenHeight);
    }
}

//------------------------------------------------------------
// 名前の入力画面
//------------------------------------------------------------
void RankingScene::DrawNameInput(const SceneContext& context) const
{
    DrawBackdrop(context);

    context.SetColor(1.00f, 0.86f, 0.40f, 1.0f);

    context.DrawText(
        L"登録",
        0.0f, 90.0f, context.screenWidth, 160.0f,
        context.headerFormat
    );

    //--------------------------------------------------------
    // スコア
    //--------------------------------------------------------
    wchar_t buffer[64];
    swprintf_s(buffer, L"SCORE  %d", m_result.score);

    context.SetColor(0.92f, 0.96f, 1.0f, 1.0f);

    context.DrawText(
        buffer,
        0.0f, 168.0f, context.screenWidth, 210.0f,
        context.menuFormat
    );

    //--------------------------------------------------------
    // 入力枠
    //--------------------------------------------------------
    constexpr float LEFT = 340.0f;
    constexpr float RIGHT = 940.0f;
    constexpr float TOP = 250.0f;
    constexpr float BOTTOM = 330.0f;

    context.SetColor(0.04f, 0.06f, 0.10f, 0.94f);
    context.FillRoundRect(LEFT, TOP, RIGHT, BOTTOM, 10.0f);

    context.SetColor(0.40f, 0.74f, 1.00f, 0.9f);
    context.DrawRoundRect(LEFT, TOP, RIGHT, BOTTOM, 10.0f, 2.0f);

    //--------------------------------------------------------
    // 入力した文字と、点滅するカーソル
    //--------------------------------------------------------
    std::wstring shown = m_name;

    if (std::fmod(m_blink, 1.0f) < 0.5f &&
        static_cast<int>(m_name.size()) < Ranking::NAME_MAX)
    {
        shown += L"_";
    }

    context.SetColor(0.96f, 0.98f, 1.0f, 1.0f);

    context.DrawText(
        shown,
        LEFT + 24.0f, TOP + 18.0f, RIGHT - 24.0f, BOTTOM - 12.0f,
        context.menuFormat
    );

    //--------------------------------------------------------
    // 残り文字数と操作
    //--------------------------------------------------------
    swprintf_s(
        buffer, L"%d / %d",
        static_cast<int>(m_name.size()), Ranking::NAME_MAX
    );

    context.SetColor(0.60f, 0.66f, 0.74f, 1.0f);

    context.DrawText(
        buffer,
        RIGHT - 140.0f, BOTTOM + 6.0f, RIGHT, BOTTOM + 34.0f,
        context.bodyFormat
    );

    context.DrawText(
        L"A ～ Z ・ 0 ～ 9 ・ SPACE で入力　　BackSpace で1文字消す\n"
        L"Enter で登録する（空のままなら プレイヤー になります）",
        340.0f, 366.0f, context.screenWidth, 430.0f,
        context.bodyFormat
    );
}

//------------------------------------------------------------
// 一覧
//------------------------------------------------------------
void RankingScene::DrawList(const SceneContext& context) const
{
    DrawBackdrop(context);

    context.SetColor(1.00f, 0.86f, 0.40f, 1.0f);

    context.DrawText(
        L"RANKING",
        0.0f, 40.0f, context.screenWidth, 100.0f,
        context.headerFormat
    );

    constexpr float LEFT = 300.0f;
    constexpr float RIGHT = 980.0f;
    constexpr float TOP = 110.0f;
    constexpr float ROW = 46.0f;

    const std::vector<RankingEntry>& entries = m_ranking.GetEntries();

    if (entries.empty())
    {
        context.SetColor(0.66f, 0.72f, 0.80f, 1.0f);

        context.DrawText(
            L"まだ記録がありません",
            0.0f, 300.0f, context.screenWidth, 340.0f,
            context.menuFormat
        );
    }

    for (size_t i = 0; i < entries.size(); ++i)
    {
        const RankingEntry& entry = entries[i];

        const float y = TOP + i * ROW;

        //----------------------------------------------------
        // 直前に登録したものは枠で目立たせる
        //----------------------------------------------------
        if (entry.highlight)
        {
            const float pulse =
                0.5f + 0.5f * std::sin(m_blink * 3.4f);

            context.SetColor(1.0f, 0.86f, 0.35f, 0.16f + pulse * 0.16f);
            context.FillRoundRect(LEFT - 12.0f, y, RIGHT + 12.0f, y + ROW - 6.0f, 8.0f);

            context.SetColor(1.0f, 0.86f, 0.35f, 0.6f + pulse * 0.35f);
            context.DrawRoundRect(LEFT - 12.0f, y, RIGHT + 12.0f, y + ROW - 6.0f, 8.0f, 1.6f);
        }

        //----------------------------------------------------
        // 順位
        //----------------------------------------------------
        wchar_t buffer[32];
        swprintf_s(buffer, L"%d", static_cast<int>(i) + 1);

        if (i == 0)      context.SetColor(1.00f, 0.84f, 0.30f, 1.0f);
        else if (i == 1) context.SetColor(0.84f, 0.88f, 0.94f, 1.0f);
        else if (i == 2) context.SetColor(0.88f, 0.64f, 0.40f, 1.0f);
        else             context.SetColor(0.62f, 0.68f, 0.76f, 1.0f);

        context.DrawText(buffer, LEFT, y + 6.0f, LEFT + 50.0f, y + ROW,
                         context.bodyFormat);

        //----------------------------------------------------
        // 名前
        //----------------------------------------------------
        context.SetColor(0.92f, 0.96f, 1.0f, 1.0f);

        context.DrawText(entry.name, LEFT + 60.0f, y + 6.0f,
                         LEFT + 340.0f, y + ROW, context.bodyFormat);

        //----------------------------------------------------
        // スコアと到達
        //----------------------------------------------------
        swprintf_s(buffer, L"%d", entry.score);

        context.SetColor(1.0f, 0.92f, 0.62f, 1.0f);

        context.DrawText(buffer, LEFT + 350.0f, y + 6.0f,
                         LEFT + 520.0f, y + ROW, context.bodyFormat);

        context.SetColor(0.60f, 0.66f, 0.74f, 1.0f);

        if (entry.cleared)
        {
            swprintf_s(buffer, L"クリア");
        }
        else
        {
            swprintf_s(buffer, L"ステージ %d", entry.stageIndex + 1);
        }

        context.DrawText(buffer, LEFT + 530.0f, y + 6.0f,
                         RIGHT, y + ROW, context.bodyFormat);
    }

    const float pulse = 0.5f + 0.5f * std::sin(m_blink * 3.4f);

    context.SetColor(0.55f, 0.72f + pulse * 0.20f, 0.90f, 1.0f);

    context.DrawText(
        L"Enter でタイトルへ戻る",
        0.0f, 636.0f, context.screenWidth, 676.0f,
        context.menuFormat
    );
}

void RankingScene::Draw(const SceneContext& context)
{
    if (m_phase == Phase::NameInput)
    {
        DrawNameInput(context);
    }
    else
    {
        DrawList(context);
    }
}

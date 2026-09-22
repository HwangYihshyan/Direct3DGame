/*--------------------------------------------------------------------

ファイル名：ControlsScene.cpp
お仕事　　：操作方法の説明画面

すべてキーボードで操作するので、
キーと役割を左右2列に分けて並べている。

----------------------------------------------------------------------*/

#include "ControlsScene.h"

#include <cmath>

namespace
{
    //--------------------------------------------------------
    // 基本操作
    //--------------------------------------------------------
    const wchar_t* MOVE_KEYS[] =
    {
        L"A / D",
        L"SPACE",
        L"↓＋SPACE",
        L"SHIFT",
        L"↑ / ↓",
        L"J",
        L"R"
    };

    const wchar_t* MOVE_TEXTS[] =
    {
        L"左右へ移動する",
        L"ジャンプする。段差や障害物の上に乗れる",
        L"乗っている段差をすり抜けて下へ降りる",
        L"ダッシュする。動いている間は無敵",
        L"狙いを上下へ45度ずつ傾ける",
        L"撃つ。押しっぱなしで連射できる武器もある",
        L"リロードする。弾が切れると自動でも始まる"
    };

    //--------------------------------------------------------
    // 戦い方
    //--------------------------------------------------------
    const wchar_t* ACTION_KEYS[] =
    {
        L"K",
        L"L",
        L"Q / 1 / 2",
        L"F",
        L"G",
        L"TAB",
        L"Esc"
    };

    const wchar_t* ACTION_TEXTS[] =
    {
        L"1つ目のスキル。キャラクターごとに違う",
        L"2つ目のスキル。効果は強いが待ち時間が長い",
        L"メイン武器とサブ武器を持ち替える",
        L"足元の装備を拾う。宝箱もこれで開ける",
        L"足元の装備を捨てる",
        L"装備一覧を開く。合計ステータスも見られる",
        L"ポーズ。もう一度押すと戻る"
    };

    constexpr int MOVE_COUNT =
        static_cast<int>(sizeof(MOVE_KEYS) / sizeof(MOVE_KEYS[0]));

    constexpr int ACTION_COUNT =
        static_cast<int>(sizeof(ACTION_KEYS) / sizeof(ACTION_KEYS[0]));
}

void ControlsScene::Reset()
{
    m_time = 0.0f;
    m_blink = 0.0f;
}

SceneType ControlsScene::Update(
    const InputTracker& input,
    float mouseX,
    float mouseY,
    float deltaTime
)
{
    (void)mouseX;
    (void)mouseY;

    m_time += deltaTime;
    m_blink += deltaTime;

    //--------------------------------------------------------
    // 見るだけの画面なので、戻る操作だけ受け付ける
    //--------------------------------------------------------
    if (input.IsPressed(VK_ESCAPE) ||
        input.IsPressed(VK_RETURN) ||
        input.IsPressed('J'))
    {
        return SceneType::Start;
    }

    return SceneType::None;
}

//------------------------------------------------------------
// キーと役割を1列ぶん描く
//------------------------------------------------------------
void ControlsScene::DrawColumn(
    const SceneContext& context,
    const wchar_t* title,
    const wchar_t* const* keys,
    const wchar_t* const* texts,
    int count,
    float left,
    float right,
    float top
) const
{
    constexpr float ROW = 52.0f;
    constexpr float KEY_WIDTH = 122.0f;

    context.SetColor(0.04f, 0.06f, 0.10f, 0.92f);
    context.FillRoundRect(left, top, right, top + 40.0f + count * ROW, 10.0f);

    context.SetColor(0.28f, 0.58f, 0.88f, 0.75f);
    context.DrawRoundRect(left, top, right, top + 40.0f + count * ROW, 10.0f, 1.4f);

    context.SetColor(0.60f, 0.82f, 1.00f, 1.0f);

    context.DrawText(
        title,
        left + 16.0f, top + 6.0f, right - 16.0f, top + 36.0f,
        context.bodyFormat
    );

    for (int i = 0; i < count; ++i)
    {
        const float y = top + 40.0f + i * ROW;

        //----------------------------------------------------
        // キーは枠で囲んで、押すものだと分かるようにする
        //----------------------------------------------------
        context.SetColor(0.10f, 0.14f, 0.20f, 0.95f);
        context.FillRoundRect(left + 16.0f, y + 4.0f,
                              left + 16.0f + KEY_WIDTH, y + 40.0f, 6.0f);

        context.SetColor(0.45f, 0.70f, 0.95f, 0.85f);
        context.DrawRoundRect(left + 16.0f, y + 4.0f,
                              left + 16.0f + KEY_WIDTH, y + 40.0f, 6.0f, 1.2f);

        context.SetColor(0.94f, 0.97f, 1.00f, 1.0f);
        context.DrawText(keys[i], left + 24.0f, y + 10.0f,
                         left + 8.0f + KEY_WIDTH, y + 38.0f,
                         context.bodyFormat);

        context.SetColor(0.80f, 0.86f, 0.92f, 1.0f);
        context.DrawText(texts[i], left + 26.0f + KEY_WIDTH, y + 10.0f,
                         right - 12.0f, y + 44.0f,
                         context.bodyFormat);
    }
}

void ControlsScene::Draw(const SceneContext& context)
{
    //--------------------------------------------------------
    // 背景
    //   作戦室を描いた専用の絵を使う。
    //   ドット絵の粒がゲーム中とそろうよう、
    //   拡大は近傍で行う。
    //--------------------------------------------------------
    if (context.controlsBackground && context.renderTarget)
    {
        context.renderTarget->DrawBitmap(
            context.controlsBackground,
            D2D1::RectF(0.0f, 0.0f, context.screenWidth, context.screenHeight),
            1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR
        );

        // 文字が読めるよう、少しだけ暗く重ねる
        context.SetColor(0.02f, 0.03f, 0.05f, 0.46f);
        context.FillRect(0.0f, 0.0f, context.screenWidth, context.screenHeight);
    }
    else
    {
        context.SetColor(0.02f, 0.03f, 0.06f, 1.0f);
        context.FillRect(0.0f, 0.0f, context.screenWidth, context.screenHeight);
    }

    context.SetColor(0.94f, 0.97f, 1.0f, 1.0f);

    context.DrawText(
        L"操作方法",
        0.0f, 26.0f, context.screenWidth, 78.0f,
        context.headerFormat
    );

    //--------------------------------------------------------
    // 左：動きまわる操作　　右：戦う操作
    //--------------------------------------------------------
    DrawColumn(context, L"移動と射撃", MOVE_KEYS, MOVE_TEXTS,
               MOVE_COUNT, 40.0f, 620.0f, 92.0f);

    DrawColumn(context, L"スキルと装備", ACTION_KEYS, ACTION_TEXTS,
               ACTION_COUNT, 660.0f, 1240.0f, 92.0f);

    //--------------------------------------------------------
    // 覚えておくと楽になること
    //--------------------------------------------------------
    context.SetColor(0.04f, 0.06f, 0.10f, 0.92f);
    context.FillRoundRect(40.0f, 508.0f, 1240.0f, 700.0f, 10.0f);

    context.SetColor(0.85f, 0.72f, 0.30f, 0.7f);
    context.DrawRoundRect(40.0f, 508.0f, 1240.0f, 700.0f, 10.0f, 1.4f);

    context.SetColor(1.0f, 0.86f, 0.40f, 1.0f);

    context.DrawText(
        L"覚えておくと楽になること",
        56.0f, 514.0f, 1224.0f, 542.0f,
        context.bodyFormat
    );

    context.SetColor(0.82f, 0.88f, 0.94f, 1.0f);

    context.DrawText(
        L"・区画の敵を全滅させると右へ進めます。倒しきるまで先へは行けません。\n"
        L"・障害物は弾を止めます。物陰でリロードすると安全です。\n"
        L"・同じシナジーを2個・4個・6個と揃えるほど効果が強くなります。\n"
        L"・サブ武器の予備弾は無限です。メインが切れたら自動で持ち替えます。",
        56.0f, 542.0f, 1224.0f, 618.0f,
        context.bodyFormat
    );

    //--------------------------------------------------------
    // 戻る案内
    //--------------------------------------------------------
    const float pulse = 0.5f + 0.5f * std::sin(m_blink * 3.4f);

    context.SetColor(0.55f, 0.72f + pulse * 0.20f, 0.90f, 1.0f);

    context.DrawText(
        L"Esc または Enter で設定へ戻る",
        0.0f, 646.0f, context.screenWidth, 680.0f,
        context.menuFormat
    );
}

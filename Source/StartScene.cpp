/*--------------------------------------------------------------------

ファイル名：StartScene.cpp
お仕事　　：タイトル画面

----------------------------------------------------------------------*/

#include "StartScene.h"
#include "AudioManager.h"

#include <algorithm>
#include <cmath>

StartScene::StartScene()
{
    m_menu.SetItems({
        { L"ゲームスタート", SceneType::LevelSelect, true },
        { L"操作方法", SceneType::Controls, true },
        { L"設定",     SceneType::Option,      true },
        { L"終了",       SceneType::Exit,        true }
        });

    m_menu.SetLayout(640.0f, 340.0f, 360.0f, 56.0f, 20.0f);
}

void StartScene::Reset()
{
    m_menu.Reset();
    m_time = 0.0f;

    m_confirmExit = false;
    m_confirmChoice = 1;
    m_confirmBlink = 0.0f;
}

SceneType StartScene::Update(
    const InputTracker& input,
    float mouseX,
    float mouseY,
    float deltaTime
)
{
    m_time += deltaTime;

    //--------------------------------------------------------
    // 　終了の確認ウィンドウ
    //   誤って終了しないよう、既定は NO にしてある
    //--------------------------------------------------------
    if (m_confirmExit)
    {
        m_confirmBlink += deltaTime;

        const int beforeChoice = m_confirmChoice;

        if (input.IsPressed(VK_LEFT) || input.IsPressed('A'))
        {
            m_confirmChoice = 0;
        }

        if (input.IsPressed(VK_RIGHT) || input.IsPressed('D'))
        {
            m_confirmChoice = 1;
        }

        if (m_confirmChoice != beforeChoice)
        {
            Audio::PlaySe(SeId::MenuMove);
        }

        if (input.IsPressed(VK_ESCAPE))
        {
            Audio::PlaySe(SeId::MenuBack);

            m_confirmExit = false;
            return SceneType::None;
        }

        if (input.IsPressed(VK_RETURN) || input.IsPressed('J'))
        {
            if (m_confirmChoice == 0)
            {
                Audio::PlaySe(SeId::MenuDecide);
                return SceneType::Exit;
            }

            Audio::PlaySe(SeId::MenuBack);

            m_confirmExit = false;
        }

        return SceneType::None;
    }

    // ロゴが出そろうまでは決定を受け付けない
    if (m_time < 0.35f)
    {
        return SceneType::None;
    }

    if (!m_menu.Update(input, mouseX, mouseY, deltaTime))
    {
        return SceneType::None;
    }

    const MenuItem* item = m_menu.GetSelectedItem();

    if (!item)
    {
        return SceneType::None;
    }

    // 終了はすぐ終わらせず、確認をはさむ
    if (item->next == SceneType::Exit)
    {
        m_confirmExit = true;
        m_confirmChoice = 1;
        m_confirmBlink = 0.0f;

        return SceneType::None;
    }

    return item->next;
}

void StartScene::Draw(const SceneContext& context)
{
    //--------------------------------------------------------
    // 背景
    //   タイトル専用の3層を、速度を変えて流し続ける。
    //   横につないだ絵なので、端まで行くと折り返して続く。
    //--------------------------------------------------------
    const bool hasTitleLayers =
        context.titleLayer[0] && context.titleLayer[1] && context.titleLayer[2];

    if (hasTitleLayers && context.renderTarget)
    {
        const float speeds[3] = { 5.0f, 14.0f, 30.0f };

        for (int i = 0; i < 3; ++i)
        {
            ID2D1Bitmap* layer = context.titleLayer[i];

            const D2D1_SIZE_F size = layer->GetSize();

            if (size.height <= 0.0f)
            {
                continue;
            }

            const float height = context.screenHeight;
            const float width = size.width * (height / size.height);

            if (width <= 1.0f)
            {
                continue;
            }

            const float offset = -std::fmod(m_time * speeds[i], width);

            for (float x = offset; x < context.screenWidth; x += width)
            {
                context.renderTarget->DrawBitmap(
                    layer,
                    D2D1::RectF(x, 0.0f, x + width, height),
                    1.0f,
                    D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR
                );
            }
        }

        //----------------------------------------------------
        // 舞い上がる光の粒
        //   同じ式で位置を出しているので、
        //   毎フレーム値を持たなくても動いて見える
        //----------------------------------------------------
        for (int i = 0; i < 26; ++i)
        {
            const float seed = static_cast<float>(i) * 7.31f;

            const float speed = 14.0f + std::fmod(seed, 11.0f);

            const float y = 620.0f -
                std::fmod(m_time * speed + seed * 23.0f, 560.0f);

            const float x = std::fmod(seed * 97.0f, context.screenWidth)
                + std::sin(m_time * 0.7f + seed) * 18.0f;

            const float life = (620.0f - y) / 560.0f;
            const float alpha = (1.0f - life) * 0.55f;

            const float size = 2.0f + std::fmod(seed, 2.0f);

            context.SetColor(1.0f, 0.84f, 0.52f, alpha);
            context.FillRect(x, y, x + size, y + size);
        }

        //----------------------------------------------------
        // ゆっくり流れる光の帯
        //----------------------------------------------------
        const float sweep =
            std::fmod(m_time * 90.0f, context.screenWidth + 520.0f) - 260.0f;

        for (int i = 0; i < 10; ++i)
        {
            const float t = static_cast<float>(i) / 10.0f;

            context.SetColor(0.60f, 0.76f, 1.0f, 0.030f * (1.0f - t));

            context.FillRect(
                sweep + i * 22.0f, 0.0f,
                sweep + i * 22.0f + 22.0f, context.screenHeight
            );
        }

        //----------------------------------------------------
        // 題字を読みやすくするための影
        //
        // 一律に暗くすると、背景を明るくしたときに
        // 境目が横線として見えてしまう。
        // 上ほど濃く、下へ向けて消える帯にしている。
        //----------------------------------------------------
        constexpr int SHADE_STEPS = 24;
        constexpr float SHADE_BOTTOM = 340.0f;

        for (int i = 0; i < SHADE_STEPS; ++i)
        {
            const float t = static_cast<float>(i) / SHADE_STEPS;

            const float top = SHADE_BOTTOM * t;
            const float bottom = SHADE_BOTTOM * (t + 1.0f / SHADE_STEPS);

            context.SetColor(0.02f, 0.03f, 0.06f, 0.30f * (1.0f - t));

            context.FillRect(0.0f, top, context.screenWidth, bottom);
        }
    }
    else if (context.titleBackground && context.renderTarget)
    {
        context.renderTarget->DrawBitmap(
            context.titleBackground,
            D2D1::RectF(0.0f, 0.0f, context.screenWidth, context.screenHeight),
            1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
        );

        context.SetColor(0.02f, 0.03f, 0.06f, 0.45f);
        context.FillRect(0.0f, 0.0f, context.screenWidth, 300.0f);
    }
    else
    {
        context.SetColor(0.02f, 0.03f, 0.06f, 1.0f);
        context.FillRect(0.0f, 0.0f, context.screenWidth, context.screenHeight);
    }

    //--------------------------------------------------------
    // キャラクター
    //   ゲーム中と同じスプライトをそのまま使うので、
    //   色がずれることがない。
    //--------------------------------------------------------
    if (context.renderTarget)
    {
        constexpr float CELL_W = 61.0f;
        constexpr float CELL_H = 68.0f;

        struct Stand
        {
            int sheet;
            int column;
            int row;
            float x;
            float scale;
            bool mirror;
        };

        const Stand stands[2] =
        {
            { 0, 5, 1, 140.0f, 4.4f, false },
            { 1, 3, 1, 940.0f, 4.4f, true  }
        };

        for (const Stand& stand : stands)
        {
            ID2D1Bitmap* sheet = context.playerSheet[stand.sheet];

            if (!sheet)
            {
                continue;
            }

            const float w = CELL_W * stand.scale;
            const float h = CELL_H * stand.scale;

            const float top = 600.0f - h;

            const D2D1_RECT_F source = D2D1::RectF(
                stand.column * CELL_W, stand.row * CELL_H,
                (stand.column + 1) * CELL_W, (stand.row + 1) * CELL_H
            );

            const D2D1_RECT_F destination =
                D2D1::RectF(stand.x, top, stand.x + w, top + h);

            D2D1_MATRIX_3X2_F oldTransform;
            context.renderTarget->GetTransform(&oldTransform);

            if (stand.mirror)
            {
                context.renderTarget->SetTransform(
                    D2D1::Matrix3x2F::Scale(
                        D2D1::SizeF(-1.0f, 1.0f),
                        D2D1::Point2F(stand.x + w * 0.5f, top + h * 0.5f)
                    ) * oldTransform
                );
            }

            context.renderTarget->DrawBitmap(
                sheet,
                destination,
                1.0f,
                D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                source
            );

            context.renderTarget->SetTransform(oldTransform);
        }
    }

    //--------------------------------------------------------
    // タイトル
    //--------------------------------------------------------
    const float fade = std::clamp(m_time / 0.8f, 0.0f, 1.0f);

    context.SetColor(0.92f, 0.96f, 1.0f, fade);

    context.DrawText(
        L"BULLET DRAFT",
        0.0f, 90.0f, context.screenWidth, 250.0f,
        context.titleFormat
    );

    context.SetColor(0.42f, 0.50f, 0.60f, fade);

  

    //--------------------------------------------------------
    // メニュー
    //--------------------------------------------------------
    m_menu.Draw(context);

    context.SetColor(0.90f, 0.90f, 0.90f, 1.0f);

    context.DrawText(
        L"W / S または ↑ / ↓ ：選択　　Enter：決定",
       500.0f, 640.0f, context.screenWidth, 668.0f,
        context.bodyFormat
    );

    if (m_confirmExit)
    {
        DrawExitConfirm(context);
    }

    context.DrawText(
        L"Ver.1",
        context.screenWidth - 120.0f, 676.0f,
        context.screenWidth - 24.0f, 704.0f,
        context.bodyFormat
    );
}


//------------------------------------------------------------
// 終了の確認
//------------------------------------------------------------
void StartScene::DrawExitConfirm(const SceneContext& context) const
{
    context.SetColor(0.0f, 0.0f, 0.0f, 0.72f);
    context.FillRect(0.0f, 0.0f, context.screenWidth, context.screenHeight);

    constexpr float LEFT = 380.0f;
    constexpr float RIGHT = 900.0f;
    constexpr float TOP = 250.0f;
    constexpr float BOTTOM = 450.0f;

    context.SetColor(0.04f, 0.06f, 0.10f, 0.96f);
    context.FillRoundRect(LEFT, TOP, RIGHT, BOTTOM, 12.0f);

    context.SetColor(0.35f, 0.70f, 1.00f, 0.9f);
    context.DrawRoundRect(LEFT, TOP, RIGHT, BOTTOM, 12.0f, 2.0f);

    context.SetColor(0.94f, 0.97f, 1.0f, 1.0f);

    context.DrawText(
        L"ゲームを終了しますか？",
        LEFT, TOP + 34.0f, RIGHT, TOP + 78.0f,
        context.menuFormat
    );

    //--------------------------------------------------------
    // YES / NO
    //--------------------------------------------------------
    const wchar_t* labels[2] = { L"はい", L"いいえ" };

    const float pulse =
        0.5f + 0.5f * std::sin(m_confirmBlink * 3.4f);

    for (int i = 0; i < 2; ++i)
    {
        const float centerX = (i == 0) ? 520.0f : 760.0f;
        const float half = 90.0f;

        const bool selected = (i == m_confirmChoice);

        if (selected)
        {
            context.SetColor(0.10f, 0.62f, 1.00f, 0.20f + pulse * 0.20f);
            context.FillRoundRect(centerX - half, 340.0f,
                                  centerX + half, 400.0f, 8.0f);

            context.SetColor(0.40f, 0.80f, 1.00f, 0.55f + pulse * 0.40f);
            context.DrawRoundRect(centerX - half, 340.0f,
                                  centerX + half, 400.0f, 8.0f, 2.0f);

            context.SetColor(0.96f, 0.98f, 1.00f, 1.0f);
        }
        else
        {
            context.SetColor(0.55f, 0.60f, 0.68f, 1.0f);
        }

        context.DrawText(labels[i], centerX - half, 350.0f,
                         centerX + half, 396.0f, context.menuFormat);
    }

    context.SetColor(0.50f, 0.56f, 0.64f, 1.0f);

  
}

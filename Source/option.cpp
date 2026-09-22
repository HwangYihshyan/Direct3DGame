#include "option.h"
#include "AudioManager.h"

#include <cmath>

#include <cwchar>





//==========================================
// 設定ファイルの値を反映する
//==========================================

void OptionScene::SetVolume(int master, int bgm, int se)
{
    auto clamp = [](int value)
    {
        if (value < 0)
        {
            value = 0;
        }

        if (value > 100)
        {
            value = 100;
        }

        return value;
    };

    masterVolume = clamp(master);
    bgmVolume = clamp(bgm);
    seVolume = clamp(se);
}


//==========================================
// OPTION入力
//==========================================

void OptionScene::Update(HWND hwnd,
    IDXGISwapChain* swapChain
)
{
    static bool oldUp = false;
    static bool oldDown = false;

    static bool oldLeft = false;
    static bool oldRight = false;

    static bool oldEnter = false;
    static bool oldEscape = false;


    bool up =
        (GetAsyncKeyState(VK_UP) & 0x8000)
        ||
        (GetAsyncKeyState('W') & 0x8000);


    bool down =
        (GetAsyncKeyState(VK_DOWN) & 0x8000)
        ||
        (GetAsyncKeyState('S') & 0x8000);


    bool left =
        (GetAsyncKeyState(VK_LEFT) & 0x8000)
        ||
        (GetAsyncKeyState('A') & 0x8000);


    bool right =
        (GetAsyncKeyState(VK_RIGHT) & 0x8000)
        ||
        (GetAsyncKeyState('D') & 0x8000);


    bool enter =
        (GetAsyncKeyState(VK_RETURN) & 0x8000);


    bool escape =
        (GetAsyncKeyState(VK_ESCAPE) & 0x8000);


    //-------------------------------------
    // 上
    //-------------------------------------

    const int beforeSelected = selected;

    if (up && !oldUp)
    {
        selected--;

        // 項目は5つ（0～4）
        if (selected < 0)
        {
            selected = 4;
        }
    }


    //-------------------------------------
    // 下
    //-------------------------------------

    if (down && !oldDown)
    {
        selected++;

        if (selected > 4)
        {
            selected = 0;
        }
    }


    //-------------------------------------
    // カーソルが動いた合図
    //-------------------------------------

    if (selected != beforeSelected)
    {
        Audio::PlaySe(SeId::MenuMove);
    }


            








    //-------------------------------------
    // MASTER VOLUME
    //-------------------------------------

    if (selected == 0)
    {
        if (left && !oldLeft)
        {
            masterVolume -= 10;

            if (masterVolume < 0)
            {
                masterVolume = 0;
            }
        }


        if (right && !oldRight)
        {
            masterVolume += 10;

            if (masterVolume > 100)
            {
                masterVolume = 100;
            }
        }
    }


    //-------------------------------------
    // BGM VOLUME
    //-------------------------------------

    if (selected == 1)
    {
        if (left && !oldLeft)
        {
            bgmVolume -= 10;

            if (bgmVolume < 0)
            {
                bgmVolume = 0;
            }
        }


        if (right && !oldRight)
        {
            bgmVolume += 10;

            if (bgmVolume > 100)
            {
                bgmVolume = 100;
            }
        }
    }


    //-------------------------------------
    // SE VOLUME
    //-------------------------------------

    if (selected == 2)
    {
        if (left && !oldLeft)
        {
            seVolume -= 10;

            if (seVolume < 0)
            {
                seVolume = 0;
            }
        }


        if (right && !oldRight)
        {
            seVolume += 10;

            if (seVolume > 100)
            {
                seVolume = 100;
            }
        }
    }


    //-------------------------------------
    // 変更した音量をその場で反映する
    //
    // Master と SE は変えた瞬間に1度鳴らして、
    // 今の大きさが耳で分かるようにしている。
    //-------------------------------------

    Audio::SetVolume(masterVolume, bgmVolume, seVolume);

    if (
        (selected == 0 || selected == 2)
        &&
        ((left && !oldLeft) || (right && !oldRight))
        )
    {
        Audio::PlaySe(SeId::MenuDecide);
    }


    //-------------------------------------
    // FULLSCREEN
    //-------------------------------------

    if (selected == 3)
    {
        if (
            (left && !oldLeft)
            ||
            (right && !oldRight)
            ||
            (enter && !oldEnter)
            )
        {
            fullscreen = !fullscreen;


            if (swapChain != nullptr)
            {
                swapChain->SetFullscreenState(
                    fullscreen,
                    nullptr
                );
            }
        }
    }


  


    //-------------------------------------
    // BACK
    //-------------------------------------

    if (
        selected == 4
        &&
        enter
        &&
        !oldEnter
        )
    {
        back = true;

        Audio::PlaySe(SeId::MenuBack);
    }


    //-------------------------------------
    // ESC
    //-------------------------------------

    if (
        escape
        &&
        !oldEscape
        )
    {
        back = true;

        Audio::PlaySe(SeId::MenuBack);
    }


    //-------------------------------------
    // 前フレーム保存
    //-------------------------------------

    oldUp = up;
    oldDown = down;

    oldLeft = left;
    oldRight = right;

    oldEnter = enter;
    oldEscape = escape;
    //---------------------------------
    // 選択が移った瞬間に光らせ、
    // あとは時間で消えていく
    //---------------------------------

    if (selected != lastSelected)
    {
        lastSelected = selected;
        glow = 1.0f;
    }

    blink += 0.09f;

    glow -= 0.045f;

    if (glow < 0.0f)
    {
        glow = 0.0f;
    }

}


//==========================================
// OPTION描画
//==========================================

void OptionScene::Draw(
    ID2D1RenderTarget* renderTarget,
    IDWriteTextFormat* titleFormat,
    IDWriteTextFormat* menuFormat,
    ID2D1SolidColorBrush* whiteBrush,
    ID2D1SolidColorBrush* blueBrush,
    ID2D1SolidColorBrush* grayBrush
)
{
    //-------------------------------------
    // OPTIONタイトル
    //-------------------------------------

    D2D1_RECT_F titleRect =
        D2D1::RectF(
            0,
            50,
            1280,
            170
        );


    renderTarget->DrawText(

        L"設定",

        static_cast<UINT32>(
            wcslen(L"設定")
            ),

        titleFormat,

        titleRect,

        whiteBrush
    );


    //-------------------------------------
    // 項目
    //-------------------------------------

    const wchar_t* names[]
    {
        L"マスター音量",
        L"BGM音量",
        L"SE音量",
        L"フルスクリーン",
        L"戻る"
    };


    //-------------------------------------
    // 回数は配列から取る。
    // 項目を足し引きしても、ここがずれない。
    //-------------------------------------

    const int itemCount =
        static_cast<int>(
            sizeof(names) / sizeof(names[0])
            );


    for (int i = 0; i < itemCount; i++)
    {
        // 項目が6つになったので行間を詰める
        float y =
            178.0f
            +
            i * 72.0f;


        D2D1_RECT_F nameRect =
            D2D1::RectF(
                260,
                y,
                650,
                y + 55
            );


        //---------------------------------
        // 選択中
        //---------------------------------

        if (i == selected)
        {
            D2D1_RECT_F selectRect =
                D2D1::RectF(
                    240,
                    y - 5,
                    1040,
                    y + 60
                );


            //---------------------------------
            // 内側から外側へ薄くなる帯を重ねる。
            // 明るさは glow で時間とともに落ちる。
            //---------------------------------

            // ゆっくり明滅させる
            const float pulse =
                0.5f + 0.5f * sinf(blink);

            const float flash = glow * glow * 0.42f + pulse * 0.12f;

            for (int step = 5; step >= 1; --step)
            {
                const float t = step / 5.0f;

                const float spread =
                    16.0f * t * (0.5f + glow);

                blueBrush->SetOpacity(
                    (0.16f + flash) * (1.0f - t) * 0.9f
                );

                renderTarget
                    ->FillRoundedRectangle(

                        D2D1::RoundedRect(

                            D2D1::RectF(
                                selectRect.left - spread,
                                selectRect.top - spread * 0.4f,
                                selectRect.right + spread,
                                selectRect.bottom + spread * 0.4f
                            ),

                            10.0f,
                            10.0f
                        ),

                        blueBrush
                    );
            }


            blueBrush->SetOpacity(
                1.0f
            );


            renderTarget->DrawText(

                names[i],

                static_cast<UINT32>(
                    wcslen(names[i])
                    ),

                menuFormat,

                nameRect,

                blueBrush
            );
        }


        //---------------------------------
        // 非選択
        //---------------------------------

        else
        {
            renderTarget->DrawText(

                names[i],

                static_cast<UINT32>(
                    wcslen(names[i])
                    ),

                menuFormat,

                nameRect,

                grayBrush
            );
        }
    }


    //-------------------------------------
    // MASTER
    //-------------------------------------

    DrawVolume(
        renderTarget,
        menuFormat,
        whiteBrush,
        blueBrush,
        grayBrush,
        masterVolume,
        178.0f
    );


    //-------------------------------------
    // BGM
    //-------------------------------------

    DrawVolume(
        renderTarget,
        menuFormat,
        whiteBrush,
        blueBrush,
        grayBrush,
        bgmVolume,
        250.0f
    );


    //-------------------------------------
    // SE
    //-------------------------------------

    DrawVolume(
        renderTarget,
        menuFormat,
        whiteBrush,
        blueBrush,
        grayBrush,
        seVolume,
        322.0f
    );


    //-------------------------------------
    // FULLSCREEN
    //-------------------------------------

    const wchar_t* fullscreenText;

    if (fullscreen)
    {
        fullscreenText =
            L"<  オン  >";
    }
    else
    {
        fullscreenText =
            L"<  オフ  >";
    }


    D2D1_RECT_F fullscreenRect =
        D2D1::RectF(
            700,
            394,
            1000,
            449
        );


    renderTarget->DrawText(

        fullscreenText,

        static_cast<UINT32>(
            wcslen(fullscreenText)
            ),

        menuFormat,

        fullscreenRect,

        whiteBrush
    );


    //-------------------------------------
    // 操作説明
    //-------------------------------------

    D2D1_RECT_F helpRect =
        D2D1::RectF(
            0,
            650,
            1280,
            700
        );


    renderTarget->DrawText(

        L"W / S : 選択    A / D : 変更    Enter : 決定    Esc : 戻る",

        static_cast<UINT32>(
            wcslen(L"W / S : 選択    A / D : 変更    Enter : 決定    Esc : 戻る")
            ),

        menuFormat,

        helpRect,

        grayBrush
    );
}


//==========================================
// Volume表示
//==========================================

void OptionScene::DrawVolume(
    ID2D1RenderTarget* renderTarget,
    IDWriteTextFormat* menuFormat,
    ID2D1SolidColorBrush* whiteBrush,
    ID2D1SolidColorBrush* blueBrush,
    ID2D1SolidColorBrush* grayBrush,
    int volume,
    float y
)
{
    wchar_t text[32];


    swprintf_s(
        text,
        L"<  %d  >",
        volume
    );


    //-------------------------------------
    // 数字
    //-------------------------------------

    D2D1_RECT_F valueRect =
        D2D1::RectF(
            700,
            y,
            1000,
            y + 45
        );


    renderTarget->DrawText(

        text,

        static_cast<UINT32>(
            wcslen(text)
            ),

        menuFormat,

        valueRect,

        whiteBrush
    );


    //-------------------------------------
    // Volume Bar
    //-------------------------------------

    float barX = 720.0f;

    float barY =
        y + 50.0f;


    float maxWidth =
        240.0f;


    float currentWidth =
        maxWidth
        *
        (
            static_cast<float>(volume)
            /
            100.0f
            );


    //-------------------------------------
    // 背景
    //-------------------------------------

    D2D1_RECT_F backgroundBar =
        D2D1::RectF(
            barX,
            barY,
            barX + maxWidth,
            barY + 8
        );


    renderTarget->FillRectangle(
        backgroundBar,
        grayBrush
    );


    //-------------------------------------
    // 現在値
    //-------------------------------------

    D2D1_RECT_F volumeBar =
        D2D1::RectF(
            barX,
            barY,
            barX + currentWidth,
            barY + 8
        );


    renderTarget->FillRectangle(
        volumeBar,
        blueBrush
    );
}


//==========================================
// BACK判定
//==========================================

bool OptionScene::IsBack() const
{
    return back;
}


void OptionScene::ResetBack()
{
    back = false;

    //--------------------------------------------------------
    // カーソルを先頭へ戻す。
    //
    // 前回「戻る」を選んだまま閉じていると、
    // 決定キーを押したまま開いた瞬間に
    // もう一度「戻る」が決定されてしまうことがある。
    //--------------------------------------------------------
    selected = 0;
}

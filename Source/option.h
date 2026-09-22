#ifndef OPTION_H
#define OPTION_H

#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <dxgi.h>

class OptionScene
{
public:

    OptionScene() = default;

    // 入力処理
    void Update(HWND hwnd, IDXGISwapChain* swapChain);

    // 描画
    void Draw(
        ID2D1RenderTarget* renderTarget,
        IDWriteTextFormat* titleFormat,
        IDWriteTextFormat* menuFormat,
        ID2D1SolidColorBrush* whiteBrush,
        ID2D1SolidColorBrush* blueBrush,
        ID2D1SolidColorBrush* grayBrush
    );

    // タイトルへ戻るか
    bool IsBack() const;

    // 操作説明を開きたいか
    bool IsControlsRequested() const { return openControls; }
    void ResetControlsRequest() { openControls = false; }

    // 選択が移った瞬間に1.0になり、そこから減衰する
    float glow = 1.0f;
    float blink = 0.0f;
    int lastSelected = -1;

    // BACK状態解除
    void ResetBack();

    //--------------------------------------------------------
    // 設定ファイルから読んだ音量を反映する（0～100）
    //--------------------------------------------------------
    void SetVolume(int master, int bgm, int se);

    int GetMasterVolume() const { return masterVolume; }
    int GetBgmVolume() const { return bgmVolume; }
    int GetSeVolume() const { return seVolume; }

private:

    void DrawVolume(
        ID2D1RenderTarget* renderTarget,
        IDWriteTextFormat* menuFormat,
        ID2D1SolidColorBrush* whiteBrush,
        ID2D1SolidColorBrush* blueBrush,
        ID2D1SolidColorBrush* grayBrush,
        int volume,
        float y
    );

private:

    int selected = 0;

    int masterVolume = 100;
    int bgmVolume = 70;
    int seVolume = 80;

    bool fullscreen = false;

    // 設定 が選ばれた
    bool openControls = false;

    bool back = false;
};

#endif
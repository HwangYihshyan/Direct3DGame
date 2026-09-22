/*--------------------------------------------------------------------

ファイル名：PostEffect.h
お仕事　　：画面全体へ掛ける後処理

これまで Direct2D はバックバッファへ直接描いていた。
そこへ1枚はさむ。

    Direct2D → オフスクリーンのテクスチャ
                   ↓  ScreenVS.hlsl / ScreenPS.hlsl
               バックバッファ

シェーダは .hlsl をビルド時に fxc がコンパイルし、
実行ファイルの隣の Shaders フォルダへ .cso として置かれる。
起動時にその .cso を読み込んでシェーダを作る。

    Shaders\ScreenVS.cso   画面を覆う三角形を作る
    Shaders\ScreenPS.cso   ビネットと被弾時の赤み

用意に失敗した場合は IsReady() が false を返す。
呼び出し側はこれまでどおりバックバッファへ直接描けばよい
（設計書 19-13 共通エラー処理と同じ考え方）。

----------------------------------------------------------------------*/

#ifndef POSTEFFECT_H
#define POSTEFFECT_H

#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

class PostEffect
{
public:
    //--------------------------------------------------------
    // オフスクリーンとシェーダを用意する。
    // 失敗しても false を返すだけで、例外は投げない。
    //--------------------------------------------------------
    bool Initialize(ID3D11Device* device, UINT width, UINT height);

    void Shutdown();

    bool IsReady() const { return m_ready; }

    //--------------------------------------------------------
    // Direct2D の描画先にする面。
    // Initialize に失敗していれば nullptr。
    //--------------------------------------------------------
    IDXGISurface* GetSceneSurface() const { return m_sceneSurface.Get(); }

    // 場面を描く前に、ここを描画先にして消す
    void BeginScene(ID3D11DeviceContext* context, const float clearColor[4]) const;

    //--------------------------------------------------------
    // 描き上がった場面をバックバッファへ出す。
    //
    //   damageFlash   被弾直後 1.0 → 0.0
    //--------------------------------------------------------
    void Apply(
        ID3D11DeviceContext* context,
        ID3D11RenderTargetView* backBuffer,
        float damageFlash
    ) const;

private:
    // 定数バッファは16バイト単位で送る決まりがある
    struct ScreenParams
    {
        float damageFlash = 0.0f;
        float vignetteScale = 1.30f;
        float reserved0 = 0.0f;
        float reserved1 = 0.0f;
    };

    bool m_ready = false;

    UINT m_width = 0;
    UINT m_height = 0;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_sceneTexture;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_sceneTarget;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_sceneView;
    Microsoft::WRL::ComPtr<IDXGISurface> m_sceneSurface;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;

    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_sampler;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_constantBuffer;
};

#endif

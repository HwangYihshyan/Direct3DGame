/*--------------------------------------------------------------------

ファイル名：PostEffect.cpp
お仕事　　：PostEffect.h の中身

頂点バッファも入力レイアウトも作っていない。
ScreenVS.hlsl が SV_VertexID から座標を組み立てるので、
Draw(3, 0) を呼ぶだけで画面全体が覆える。

----------------------------------------------------------------------*/

#include "PostEffect.h"

#include <fstream>
#include <vector>

namespace
{
    //--------------------------------------------------------
    // .cso を丸ごと読む。
    // 見つからなければ空を返す。
    //--------------------------------------------------------
    std::vector<char> LoadCompiledShader(const char* path)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);

        if (!file)
        {
            return {};
        }

        const std::streamsize size = file.tellg();

        if (size <= 0)
        {
            return {};
        }

        file.seekg(0, std::ios::beg);

        std::vector<char> data(static_cast<size_t>(size));

        if (!file.read(data.data(), size))
        {
            return {};
        }

        return data;
    }
}

bool PostEffect::Initialize(ID3D11Device* device, UINT width, UINT height)
{
    Shutdown();

    if (!device || width == 0 || height == 0)
    {
        return false;
    }

    m_width = width;
    m_height = height;

    //--------------------------------------------------------
    // 場面を受け取るテクスチャ
    //
    //   Direct2D の描画先（RENDER_TARGET）であり、
    //   同時にシェーダから読む絵（SHADER_RESOURCE）でもある。
    //
    //   形式はバックバッファと同じ B8G8R8A8。
    //   Direct2D と相互運用するにはこれである必要がある。
    //--------------------------------------------------------
    D3D11_TEXTURE2D_DESC textureDesc{};

    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags =
        D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    HRESULT hr = device->CreateTexture2D(
        &textureDesc, nullptr, m_sceneTexture.GetAddressOf()
    );

    if (FAILED(hr))
    {
        Shutdown();
        return false;
    }

    hr = device->CreateRenderTargetView(
        m_sceneTexture.Get(), nullptr, m_sceneTarget.GetAddressOf()
    );

    if (FAILED(hr))
    {
        Shutdown();
        return false;
    }

    hr = device->CreateShaderResourceView(
        m_sceneTexture.Get(), nullptr, m_sceneView.GetAddressOf()
    );

    if (FAILED(hr))
    {
        Shutdown();
        return false;
    }

    hr = m_sceneTexture.As(&m_sceneSurface);

    if (FAILED(hr))
    {
        Shutdown();
        return false;
    }

    //--------------------------------------------------------
    // シェーダ
    //   ビルド時に fxc が作った .cso を読む。
    //--------------------------------------------------------
    const std::vector<char> vertexCode =
        LoadCompiledShader("Shaders\\ScreenVS.cso");

    const std::vector<char> pixelCode =
        LoadCompiledShader("Shaders\\ScreenPS.cso");

    if (vertexCode.empty() || pixelCode.empty())
    {
        Shutdown();
        return false;
    }

    hr = device->CreateVertexShader(
        vertexCode.data(), vertexCode.size(), nullptr,
        m_vertexShader.GetAddressOf()
    );

    if (FAILED(hr))
    {
        Shutdown();
        return false;
    }

    hr = device->CreatePixelShader(
        pixelCode.data(), pixelCode.size(), nullptr,
        m_pixelShader.GetAddressOf()
    );

    if (FAILED(hr))
    {
        Shutdown();
        return false;
    }

    //--------------------------------------------------------
    // 等倍で出すので、拡大方法は線形で十分。
    // 端をなめるとにじむので CLAMP にしておく。
    //--------------------------------------------------------
    D3D11_SAMPLER_DESC samplerDesc{};

    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = device->CreateSamplerState(&samplerDesc, m_sampler.GetAddressOf());

    if (FAILED(hr))
    {
        Shutdown();
        return false;
    }

    //--------------------------------------------------------
    // 画面より大きい三角形を出すので、裏面を捨てない設定にする
    //--------------------------------------------------------
    D3D11_RASTERIZER_DESC rasterDesc{};

    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.DepthClipEnable = TRUE;

    hr = device->CreateRasterizerState(
        &rasterDesc, m_rasterizer.GetAddressOf()
    );

    if (FAILED(hr))
    {
        Shutdown();
        return false;
    }

    //--------------------------------------------------------
    // 毎フレーム書き換える定数バッファ
    //--------------------------------------------------------
    D3D11_BUFFER_DESC bufferDesc{};

    bufferDesc.ByteWidth = sizeof(ScreenParams);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = device->CreateBuffer(
        &bufferDesc, nullptr, m_constantBuffer.GetAddressOf()
    );

    if (FAILED(hr))
    {
        Shutdown();
        return false;
    }

    m_ready = true;

    return true;
}

void PostEffect::Shutdown()
{
    m_ready = false;

    m_constantBuffer.Reset();
    m_rasterizer.Reset();
    m_sampler.Reset();

    m_pixelShader.Reset();
    m_vertexShader.Reset();

    m_sceneSurface.Reset();
    m_sceneView.Reset();
    m_sceneTarget.Reset();
    m_sceneTexture.Reset();
}

void PostEffect::BeginScene(
    ID3D11DeviceContext* context,
    const float clearColor[4]
) const
{
    if (!m_ready || !context)
    {
        return;
    }

    ID3D11RenderTargetView* target = m_sceneTarget.Get();

    context->OMSetRenderTargets(1, &target, nullptr);
    context->ClearRenderTargetView(target, clearColor);
}

void PostEffect::Apply(
    ID3D11DeviceContext* context,
    ID3D11RenderTargetView* backBuffer,
    float damageFlash
) const
{
    if (!m_ready || !context || !backBuffer)
    {
        return;
    }

    //--------------------------------------------------------
    // シェーダへ渡す値
    //--------------------------------------------------------
    D3D11_MAPPED_SUBRESOURCE mapped{};

    if (SUCCEEDED(context->Map(
        m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        ScreenParams params;

        params.damageFlash =
            (damageFlash < 0.0f) ? 0.0f
                                 : (damageFlash > 1.0f ? 1.0f : damageFlash);

        *static_cast<ScreenParams*>(mapped.pData) = params;

        context->Unmap(m_constantBuffer.Get(), 0);
    }

    //--------------------------------------------------------
    // バックバッファへ切り替える
    //--------------------------------------------------------
    context->OMSetRenderTargets(1, &backBuffer, nullptr);

    D3D11_VIEWPORT viewport{};

    viewport.Width = static_cast<float>(m_width);
    viewport.Height = static_cast<float>(m_height);
    viewport.MaxDepth = 1.0f;

    context->RSSetViewports(1, &viewport);
    context->RSSetState(m_rasterizer.Get());

    //--------------------------------------------------------
    // 頂点バッファも入力レイアウトも使わない。
    // 頂点シェーダが SV_VertexID から座標を作る。
    //--------------------------------------------------------
    context->IASetInputLayout(nullptr);
    context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);

    ID3D11ShaderResourceView* view = m_sceneView.Get();
    ID3D11SamplerState* sampler = m_sampler.Get();
    ID3D11Buffer* buffer = m_constantBuffer.Get();

    context->PSSetShaderResources(0, 1, &view);
    context->PSSetSamplers(0, 1, &sampler);
    context->PSSetConstantBuffers(0, 1, &buffer);

    context->Draw(3, 0);

    //--------------------------------------------------------
    // 次のフレームで同じテクスチャへ描くので、外しておく。
    // 描画先と読み取り元を同時に指したままにはできない。
    //--------------------------------------------------------
    ID3D11ShaderResourceView* empty = nullptr;

    context->PSSetShaderResources(0, 1, &empty);
}

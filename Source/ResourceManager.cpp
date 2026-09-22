/*--------------------------------------------------------------------

ファイル名：ResourceManager.cpp
お仕事　　：画像の読み込みとキャッシュ

----------------------------------------------------------------------*/

#include "ResourceManager.h"

bool ResourceManager::Initialize(ID2D1RenderTarget* renderTarget)
{
    if (!renderTarget)
    {
        return false;
    }

    m_renderTarget = renderTarget;
    m_bitmaps.clear();

    if (!m_wicFactory)
    {
        const HRESULT hr = CoCreateInstance(
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&m_wicFactory)
        );

        if (FAILED(hr))
        {
            return false;
        }
    }

    return true;
}

void ResourceManager::Clear()
{
    m_bitmaps.clear();
    m_renderTarget = nullptr;
}

ID2D1Bitmap* ResourceManager::GetBitmap(const std::wstring& path)
{
    if (!m_renderTarget || !m_wicFactory || path.empty())
    {
        return nullptr;
    }

    //--------------------------------------------------------
    // すでに読み込んでいればそれを返す。
    // 失敗したパスも nullptr のまま登録しておき、
    // 毎フレーム読み込みを試さないようにする。
    //--------------------------------------------------------
    const auto found = m_bitmaps.find(path);

    if (found != m_bitmaps.end())
    {
        return found->second.Get();
    }

    Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap;

    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;

    HRESULT hr = m_wicFactory->CreateDecoderFromFilename(
        path.c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        &decoder
    );

    if (SUCCEEDED(hr))
    {
        Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
        hr = decoder->GetFrame(0, &frame);

        Microsoft::WRL::ComPtr<IWICFormatConverter> converter;

        if (SUCCEEDED(hr))
        {
            hr = m_wicFactory->CreateFormatConverter(&converter);
        }

        if (SUCCEEDED(hr))
        {
            hr = converter->Initialize(
                frame.Get(),
                GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone,
                nullptr,
                0.0,
                WICBitmapPaletteTypeMedianCut
            );
        }

        if (SUCCEEDED(hr))
        {
            hr = m_renderTarget->CreateBitmapFromWicBitmap(
                converter.Get(),
                nullptr,
                &bitmap
            );
        }
    }

    if (FAILED(hr))
    {
        ++m_failedCount;
        bitmap = nullptr;
    }

    m_bitmaps[path] = bitmap;

    return bitmap.Get();
}

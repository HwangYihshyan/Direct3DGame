/*--------------------------------------------------------------------

ファイル名：ResourceManager.h
お仕事　　：画像の読み込みとキャッシュ

設計書 Ver.3 19-11.1
    同じ画像を何度も読み込まない。Pathをキーにしてキャッシュする。

読み込みに失敗した場合は nullptr を返す。
呼び出し側は nullptr でも動くように書くこと（19-13 共通エラー処理）。

----------------------------------------------------------------------*/

#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <d2d1.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <string>
#include <unordered_map>

class ResourceManager
{
public:
    // Direct2D の RenderTarget が作り直された場合は再度呼ぶ
    bool Initialize(ID2D1RenderTarget* renderTarget);

    // 読み込み済みならキャッシュを返す
    ID2D1Bitmap* GetBitmap(const std::wstring& path);

    // RenderTarget を作り直す前に呼ぶ
    void Clear();

    bool IsReady() const { return m_renderTarget.Get() != nullptr; }

    // 読み込みに失敗したパスの数（デバッグ用）
    int GetFailedCount() const { return m_failedCount; }

private:
    Microsoft::WRL::ComPtr<ID2D1RenderTarget> m_renderTarget;
    Microsoft::WRL::ComPtr<IWICImagingFactory> m_wicFactory;

    std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<ID2D1Bitmap>> m_bitmaps;

    int m_failedCount = 0;
};

#endif

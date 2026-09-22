/*--------------------------------------------------------------------

ファイル名：UIRenderer.h
お仕事　　：UIクラスが共有する描画の道具

設計書 Ver.3 19-6
    UIはゲームの内部データを書き換えず、表示のみ担当する。

各UIが個別に Brush や TextFormat を持たなくて済むよう、
ここへまとめて渡す。

----------------------------------------------------------------------*/

#ifndef UIRENDERER_H
#define UIRENDERER_H

#include <d2d1.h>
#include <dwrite.h>

#include <string>

#include "EquipmentTypes.h"

class UIRenderer
{
public:
    void Initialize(
        ID2D1RenderTarget* renderTarget,
        ID2D1SolidColorBrush* brush,
        ID2D1SolidColorBrush* panelBrush,
        IDWriteTextFormat* smallFormat,
        IDWriteTextFormat* normalFormat,
        IDWriteTextFormat* headerFormat,
        IDWriteTextFormat* bigFormat
    );

    // synergy_icons.png（1セル64x64・横10列）を渡す
    void SetIconBitmap(ID2D1Bitmap* icons) { m_iconBitmap = icons; }

    // skill_icons.png（1セル64・横4列）
    void SetSkillIconBitmap(ID2D1Bitmap* icons) { m_skillIconBitmap = icons; }

    void DrawSkillIcon(
        int index, float left, float top, float size, float alpha = 1.0f
    ) const;

    // stat_icons.png（1セル48・横20列）
    void SetStatIconBitmap(ID2D1Bitmap* icons) { m_statIconBitmap = icons; }

    void DrawStatIcon(
        int index, float left, float top, float size, float alpha = 1.0f
    ) const;

    // 比較矢印。stronger なら赤い上向き、そうでなければ緑の下向き
    void DrawCompareArrow(float centerX, float centerY, bool stronger) const;

    bool HasIcons() const { return m_iconBitmap != nullptr; }

    //--------------------------------------------------------
    // シナジーを画像で描く。size は一辺の長さ
    //--------------------------------------------------------
    void DrawSynergyIcon(
        SynergyType type,
        float left,
        float top,
        float size,
        float alpha = 1.0f
    ) const;

    bool IsReady() const { return m_renderTarget != nullptr; }

    ID2D1RenderTarget* GetRenderTarget() const { return m_renderTarget; }

    IDWriteTextFormat* Small() const { return m_smallFormat; }
    IDWriteTextFormat* Normal() const { return m_normalFormat; }
    IDWriteTextFormat* Header() const { return m_headerFormat; }
    IDWriteTextFormat* Big() const { return m_bigFormat; }

    //--------------------------------------------------------
    // 色
    //--------------------------------------------------------
    void SetColor(float r, float g, float b, float a = 1.0f) const;
    void SetRarityColor(Rarity rarity, float alpha = 1.0f) const;

    //--------------------------------------------------------
    // 図形
    //--------------------------------------------------------
    void FillRect(float l, float t, float r, float b) const;
    void FillPanel(float l, float t, float r, float b, float radius) const;
    void FillRound(float l, float t, float r, float b, float radius) const;
    void DrawRound(float l, float t, float r, float b,
                   float radius, float thickness) const;
    void DrawLine(float x1, float y1, float x2, float y2,
                  float thickness) const;

    // 値バー（背景・本体・枠）
    void DrawBar(float x, float y, float width, float height,
                 float ratio, float r, float g, float b) const;

    //--------------------------------------------------------
    // 文字
    //--------------------------------------------------------
    void DrawText(
        const std::wstring& text,
        float l, float t, float r, float b,
        IDWriteTextFormat* format
    ) const;

private:
    ID2D1RenderTarget* m_renderTarget = nullptr;
    ID2D1SolidColorBrush* m_brush = nullptr;
    ID2D1SolidColorBrush* m_panelBrush = nullptr;

    IDWriteTextFormat* m_smallFormat = nullptr;
    IDWriteTextFormat* m_normalFormat = nullptr;
    IDWriteTextFormat* m_headerFormat = nullptr;
    IDWriteTextFormat* m_bigFormat = nullptr;

    ID2D1Bitmap* m_iconBitmap = nullptr;
    ID2D1Bitmap* m_skillIconBitmap = nullptr;
    ID2D1Bitmap* m_statIconBitmap = nullptr;
};

#endif

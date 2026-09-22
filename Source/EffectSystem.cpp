/*--------------------------------------------------------------------

ファイル名：EffectSystem.cpp
お仕事　　：画面エフェクトの生成・更新・描画

effects.png
    1セル 54x53、横4フレーム×縦3種類
    0行目：爆発（オレンジ）
    1行目：着弾（青）
    2行目：撃破（赤）

bullets.png
    1セル 62x40
    0:自機の弾 1:敵の弾 2:マズルフラッシュ 3:散弾

----------------------------------------------------------------------*/

#include "EffectSystem.h"

#include "Camera.h"
#include "ResourceManager.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float EFFECT_CELL_W = 54.0f;
    constexpr float EFFECT_CELL_H = 53.0f;
    constexpr int EFFECT_FRAMES = 4;

    constexpr float BULLET_CELL_W = 62.0f;
    constexpr float BULLET_CELL_H = 40.0f;

    // 種類ごとの継続時間と大きさ
    float GetDuration(EffectType type)
    {
        switch (type)
        {
        case EffectType::Muzzle:    return 0.07f;
        case EffectType::Impact:    return 0.26f;
        case EffectType::Explosion: return 0.38f;
        case EffectType::Death:     return 0.42f;
        case EffectType::Dust:      return 0.26f;
        }

        return 0.3f;
    }

    float GetBaseSize(EffectType type)
    {
        switch (type)
        {
        case EffectType::Muzzle:    return 54.0f;
        case EffectType::Impact:    return 46.0f;
        case EffectType::Explosion: return 120.0f;
        case EffectType::Death:     return 96.0f;
        case EffectType::Dust:      return 40.0f;
        }

        return 48.0f;
    }

    // effects.png の何行目を使うか
    int GetRow(EffectType type)
    {
        switch (type)
        {
        case EffectType::Explosion: return 0;
        case EffectType::Death:     return 2;
        default:                    return 1;   // Impact / Dust
        }
    }
}

void EffectSystem::Initialize(ResourceManager* resources)
{
    m_resources = resources;
    m_effects.clear();

    // RenderTarget が作り直されている可能性があるので捨てる
    m_brush.Reset();
    m_brushOwner = nullptr;
}

ID2D1SolidColorBrush* EffectSystem::AcquireBrush(
    ID2D1RenderTarget* renderTarget
) const
{
    if (m_brush && m_brushOwner == renderTarget)
    {
        return m_brush.Get();
    }

    m_brush.Reset();
    m_brushOwner = nullptr;

    if (!renderTarget)
    {
        return nullptr;
    }

    HRESULT hr = renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::White),
        m_brush.ReleaseAndGetAddressOf()
    );

    if (FAILED(hr))
    {
        m_brush.Reset();
        return nullptr;
    }

    m_brushOwner = renderTarget;

    return m_brush.Get();
}

void EffectSystem::Reset()
{
    m_effects.clear();
}

void EffectSystem::Spawn(
    EffectType type,
    float worldX,
    float worldY,
    float scale,
    float rotation
)
{
    //--------------------------------------------------------
    // 上限に達していたら、一番古いものを捨てる
    //--------------------------------------------------------
    if (static_cast<int>(m_effects.size()) >= MAX_EFFECTS)
    {
        m_effects.erase(m_effects.begin());
    }

    EffectInstance effect;

    effect.type = type;
    effect.worldX = worldX;
    effect.worldY = worldY;
    effect.scale = scale;
    effect.rotation = rotation;
    effect.timer = 0.0f;
    effect.duration = GetDuration(type);

    effect.seed = m_seedCounter;

    // 一周しても偏らない程度の大きさで回す
    m_seedCounter = (m_seedCounter + 1) % 1024;

    m_effects.push_back(effect);
}

void EffectSystem::Update(float deltaTime)
{
    for (EffectInstance& effect : m_effects)
    {
        effect.timer += deltaTime;
    }

    m_effects.erase(
        std::remove_if(
            m_effects.begin(),
            m_effects.end(),
            [](const EffectInstance& e)
            {
                return e.timer >= e.duration;
            }
        ),
        m_effects.end()
    );
}

//------------------------------------------------------------
// 着弾の火花
//
// 弾が当たった一点から
//    ・中心の閃光
//    ・広がって消えるリング
//    ・弾が来た向きへ跳ね返る火花
// を重ねて出す。当たった位置と向きが同時に分かる。
//------------------------------------------------------------
void EffectSystem::DrawImpactBurst(
    ID2D1RenderTarget* renderTarget,
    const EffectInstance& effect,
    float screenX,
    float t
) const
{
    ID2D1SolidColorBrush* brush = AcquireBrush(renderTarget);

    if (!brush)
    {
        return;
    }

    const float size = GetBaseSize(EffectType::Impact) * effect.scale;
    const float y = effect.worldY;

    // 1.0 から 0.0 へ落ちる
    const float fade = 1.0f - t;

    // 0.0 から 1.0 へ、最初は速く後はゆっくり
    const float grow = 1.0f - fade * fade;

    const D2D1_POINT_2F center = D2D1::Point2F(screenX, y);

    //--------------------------------------------------------
    // 中心の閃光。すぼみながら消える。
    //--------------------------------------------------------
    const float coreRadius = size * 0.17f * (1.0f - t * 0.85f);

    if (coreRadius > 0.5f)
    {
        brush->SetColor(D2D1::ColorF(1.0f, 0.98f, 0.89f, fade * fade));

        renderTarget->FillEllipse(
            D2D1::Ellipse(center, coreRadius, coreRadius),
            brush
        );
    }

    //--------------------------------------------------------
    // 広がるリング
    //--------------------------------------------------------
    const float ringRadius = size * (0.14f + 0.62f * grow);

    brush->SetColor(D2D1::ColorF(0.72f, 0.88f, 1.0f, fade * fade * 0.95f));

    renderTarget->DrawEllipse(
        D2D1::Ellipse(center, ringRadius, ringRadius),
        brush,
        1.0f + size * 0.045f * fade
    );

    //--------------------------------------------------------
    // 火花
    //   弾の進行方向の逆側へ、扇状に散らす。
    //--------------------------------------------------------
    constexpr int SPARK_COUNT = 7;
    constexpr float DEG_TO_RAD = 3.14159265f / 180.0f;

    // 跳ね返る向き
    const float baseAngle = (effect.rotation + 180.0f) * DEG_TO_RAD;

    brush->SetColor(D2D1::ColorF(1.0f, 0.82f, 0.42f, fade));

    for (int i = 0; i < SPARK_COUNT; ++i)
    {
        //----------------------------------------------------
        // seed で1発ごとに散り方を変える。
        // 乱数を持ち込まずに、整数の剰余でばらつかせている。
        //----------------------------------------------------
        const int mix = (effect.seed * 37 + i * 61) % 101;

        // -60度 ～ +60度
        const float spread = (mix / 100.0f - 0.5f) * 120.0f * DEG_TO_RAD;

        // 火花ごとに長さを変える
        const float lengthScale = 0.60f + ((mix % 7) / 7.0f) * 0.85f;

        const float angle = baseAngle + spread;

        const float dirX = std::cos(angle);
        const float dirY = std::sin(angle);

        const float inner = size * (0.16f + 0.46f * grow);
        const float outer = inner + size * 0.52f * lengthScale * fade;

        renderTarget->DrawLine(
            D2D1::Point2F(
                center.x + dirX * inner,
                center.y + dirY * inner
            ),
            D2D1::Point2F(
                center.x + dirX * outer,
                center.y + dirY * outer
            ),
            brush,
            1.0f + size * 0.022f * fade
        );
    }
}

void EffectSystem::Draw(
    ID2D1RenderTarget* renderTarget,
    const Camera& camera
) const
{
    if (!renderTarget || !m_resources)
    {
        return;
    }

    ID2D1Bitmap* effectBitmap =
        m_resources->GetBitmap(L"Assets\\texture\\effects.png");

    ID2D1Bitmap* bulletBitmap =
        m_resources->GetBitmap(L"Assets\\texture\\bullets.png");

    for (const EffectInstance& effect : m_effects)
    {
        const float screenX = camera.WorldToScreenX(effect.worldX);

        // 画面外は描かない
        if (screenX < -160.0f || screenX > camera.GetViewWidth() + 160.0f)
        {
            continue;
        }

        const float t = std::clamp(effect.timer / effect.duration, 0.0f, 1.0f);

        const float size = GetBaseSize(effect.type) * effect.scale;
        const float half = size * 0.5f;

        const D2D1_RECT_F destination = D2D1::RectF(
            screenX - half,
            effect.worldY - half,
            screenX + half,
            effect.worldY + half
        );

        //----------------------------------------------------
        // マズルフラッシュだけは bullets.png の1コマを使い、
        // 撃った向きへ回転させる
        //----------------------------------------------------
        if (effect.type == EffectType::Muzzle)
        {
            if (!bulletBitmap)
            {
                continue;
            }

            D2D1_MATRIX_3X2_F oldTransform;
            renderTarget->GetTransform(&oldTransform);

            renderTarget->SetTransform(
                D2D1::Matrix3x2F::Rotation(
                    effect.rotation,
                    D2D1::Point2F(screenX, effect.worldY)
                ) * oldTransform
            );

            const D2D1_RECT_F source = D2D1::RectF(
                BULLET_CELL_W * 2.0f, m_bulletRow * BULLET_CELL_H,
                BULLET_CELL_W * 3.0f, (m_bulletRow + 1.0f) * BULLET_CELL_H
            );

            const float w = size;
            const float h = size * (BULLET_CELL_H / BULLET_CELL_W);

            renderTarget->DrawBitmap(
                bulletBitmap,
                D2D1::RectF(
                    screenX - w * 0.5f, effect.worldY - h * 0.5f,
                    screenX + w * 0.5f, effect.worldY + h * 0.5f
                ),
                1.0f - t,
                D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
                source
            );

            renderTarget->SetTransform(oldTransform);
            continue;
        }

        if (effectBitmap)
        {
            //------------------------------------------------
            // 4コマのうち、経過時間に応じたコマを出す
            //------------------------------------------------
            int frame = static_cast<int>(t * EFFECT_FRAMES);

            frame = std::clamp(frame, 0, EFFECT_FRAMES - 1);

            const int row = GetRow(effect.type);

            const D2D1_RECT_F source = D2D1::RectF(
                frame * EFFECT_CELL_W,
                row * EFFECT_CELL_H,
                (frame + 1) * EFFECT_CELL_W,
                (row + 1) * EFFECT_CELL_H
            );

            renderTarget->DrawBitmap(
                effectBitmap,
                destination,
                1.0f - t * 0.35f,
                D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
                source
            );
        }

        //----------------------------------------------------
        // 着弾だけは火花を上に重ねる。
        // 図形で描いているので、effects.png が読めていなくても
        // 当たったことは必ず分かる。
        //----------------------------------------------------
        if (effect.type == EffectType::Impact)
        {
            DrawImpactBurst(renderTarget, effect, screenX, t);
        }
    }
}

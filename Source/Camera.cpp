/*--------------------------------------------------------------------

ファイル名：Camera.cpp
お仕事　　：横スクロール用のカメラ

----------------------------------------------------------------------*/

#include "Camera.h"

#include <algorithm>
#include <cmath>

namespace
{
    // 追従の滑らかさ（1秒あたりの追いつき割合）
    constexpr float FOLLOW_SPEED = 7.0f;
}

void Camera::Initialize(
    float worldWidth,
    float viewWidth,
    float viewHeight
)
{
    m_viewWidth = viewWidth;
    m_viewHeight = viewHeight;

    // ワールドが画面より狭いとスクロールできないので下限を設ける
    m_worldWidth = std::max(worldWidth, viewWidth);

    m_x = 0.0f;
}

float Camera::GetScrollRange() const
{
    return std::max(0.0f, m_worldWidth - m_viewWidth);
}

void Camera::SnapTo(float targetCenterX)
{
    m_x = std::clamp(
        targetCenterX - m_viewWidth * 0.5f,
        0.0f,
        GetScrollRange()
    );
}

void Camera::Update(float deltaTime, float targetCenterX)
{
    const float desired = std::clamp(
        targetCenterX - m_viewWidth * 0.5f,
        0.0f,
        GetScrollRange()
    );

    //--------------------------------------------------------
    // 一定割合で近づける。
    // deltaTime に依存しない指数補間にしておくと、
    // フレームレートが変わっても速度が変わらない。
    //--------------------------------------------------------
    const float t = 1.0f - std::exp(-FOLLOW_SPEED * deltaTime);

    m_x += (desired - m_x) * t;

    // 誤差でわずかにはみ出すことがあるので締める
    m_x = std::clamp(m_x, 0.0f, GetScrollRange());
}

//------------------------------------------------------------
// 背景の視差
//   parallax 0.0 … まったく動かない（遠景）
//   parallax 1.0 … 地面と同じ速度で動く
//------------------------------------------------------------
float Camera::GetParallaxOffset(float parallax) const
{
    return -m_x * parallax;
}

bool Camera::IsVisible(float worldX, float width) const
{
    const float left = worldX - m_x;

    // 少し余裕を持たせて判定する
    constexpr float MARGIN = 120.0f;

    return (left + width >= -MARGIN) && (left <= m_viewWidth + MARGIN);
}

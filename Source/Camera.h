/*--------------------------------------------------------------------

ファイル名：Camera.h
お仕事　　：横スクロール用のカメラ

設計書 Ver.3 19-1.3「Camera / Scroll管理」

ワールド座標（マップ全体）を、スクリーン座標（1280x720）へ変換する。
背景は視差（Parallax）を掛けて、手前より遅く動かす。

----------------------------------------------------------------------*/

#ifndef CAMERA_H
#define CAMERA_H

class Camera
{
public:
    // ステージ開始時に呼ぶ
    void Initialize(float worldWidth, float viewWidth, float viewHeight);

    // 追従対象（プレイヤー中心）を渡して毎フレーム更新する
    void Update(float deltaTime, float targetCenterX);

    // 追従を待たずに即座に合わせる（ステージ開始時など）
    void SnapTo(float targetCenterX);

    //--------------------------------------------------------
    // 座標変換
    //--------------------------------------------------------
    float WorldToScreenX(float worldX) const { return worldX - m_x; }
    float ScreenToWorldX(float screenX) const { return screenX + m_x; }

    float GetX() const { return m_x; }
    float GetWorldWidth() const { return m_worldWidth; }
    float GetViewWidth() const { return m_viewWidth; }
    float GetViewHeight() const { return m_viewHeight; }

    // カメラが動ける幅（0ならスクロールしない）
    float GetScrollRange() const;

    // 背景を視差付きで描くときの左端オフセット
    float GetParallaxOffset(float parallax) const;

    // 画面外の判定（描画スキップ用）
    bool IsVisible(float worldX, float width) const;

private:
    float m_x = 0.0f;
    float m_worldWidth = 1280.0f;
    float m_viewWidth = 1280.0f;
    float m_viewHeight = 720.0f;
};

#endif

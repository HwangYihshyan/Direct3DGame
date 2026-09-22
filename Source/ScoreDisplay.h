/*--------------------------------------------------------------------

ファイル名：ScoreDisplay.h
お仕事　　：スコアの計算補助と、画面右上への表示

設計書 Ver.3 19-2.13 / 19-6.1

加点の内訳は ShowScore が持っている。
このクラスは
    ・表示用の数値をなめらかに追従させる
    ・加点があった瞬間に強調する
    ・画面右上へ描く
を受け持つ。

----------------------------------------------------------------------*/

#ifndef SCOREDISPLAY_H
#define SCOREDISPLAY_H

#include "ShowScore.h"
#include "UIRenderer.h"

class ScoreDisplay
{
public:
    void Reset();

    // 毎フレーム呼ぶ
    void Update(float deltaTime, const ShowScore& score);

    // 画面右上へ描く
    void Draw(
        const UIRenderer& ui,
        const ShowScore& score,
        float screenWidth
    ) const;

    // 表示中の数値（カウントアップ途中の値）
    int GetDisplayScore() const;

private:
    // 実際のスコアへ徐々に近づけていく
    float m_displayed = 0.0f;

    // 加点直後に大きくなる
    float m_punch = 0.0f;

    int m_lastScore = 0;
};

#endif

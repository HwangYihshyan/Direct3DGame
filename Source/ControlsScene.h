/*--------------------------------------------------------------------

ファイル名：ControlsScene.h
お仕事　　：操作方法の説明画面

OPTION 画面の「CONTROLS」から開く。
遊び方が分からなくなったときに、いつでも確認できるようにする。

----------------------------------------------------------------------*/

#ifndef CONTROLSSCENE_H
#define CONTROLSSCENE_H

#include "Scene.h"

class ControlsScene : public Scene
{
public:
    void Reset() override;

    SceneType Update(
        const InputTracker& input,
        float mouseX,
        float mouseY,
        float deltaTime
    ) override;

    void Draw(const SceneContext& context) override;

private:
    void DrawColumn(
        const SceneContext& context,
        const wchar_t* title,
        const wchar_t* const* keys,
        const wchar_t* const* texts,
        int count,
        float left,
        float right,
        float top
    ) const;

private:
    float m_time = 0.0f;
    float m_blink = 0.0f;
};

#endif

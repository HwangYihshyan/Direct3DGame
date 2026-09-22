/*--------------------------------------------------------------------

ファイル名：GameOverScene.h
お仕事　　：Player HP <= 0 のときの画面

設計書 Ver.3 19-1.5
    到達Stage / 生存時間 / Kill / Score / 取得装備
    RETRY / TITLE

----------------------------------------------------------------------*/

#ifndef GAMEOVERSCENE_H
#define GAMEOVERSCENE_H

#include "ResultData.h"
#include "Scene.h"

class GameOverScene : public Scene
{
public:
    GameOverScene();

    // ライフが残っていなければ RETRY を出さない
    void SetResult(const ResultData& result);

    void Reset() override;

    SceneType Update(
        const InputTracker& input,
        float mouseX,
        float mouseY,
        float deltaTime
    ) override;

    void Draw(const SceneContext& context) override;

    bool WantsRetry() const { return m_wantsRetry; }

private:
    MenuController m_menu;
    ResultData m_result;

    float m_fade = 0.0f;
    bool m_wantsRetry = false;
};

#endif

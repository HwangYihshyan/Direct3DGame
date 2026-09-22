/*--------------------------------------------------------------------

ファイル名：GameClearScene.h
お仕事　　：全ステージ踏破時の画面

設計書 Ver.3 19-1.6
    Total Time / Total Kill / Final Score / Final Build /
    Synergy一覧 / 装備一覧 / 最高Damage / 取得SSS数

----------------------------------------------------------------------*/

#ifndef GAMECLEARSCENE_H
#define GAMECLEARSCENE_H

#include "ResultData.h"
#include "Scene.h"

class GameClearScene : public Scene
{
public:
    GameClearScene();

    void SetResult(const ResultData& result);

    void Reset() override;

    SceneType Update(
        const InputTracker& input,
        float mouseX,
        float mouseY,
        float deltaTime
    ) override;

    void Draw(const SceneContext& context) override;

private:
    MenuController m_menu;
    ResultData m_result;

    float m_fade = 0.0f;
};

#endif

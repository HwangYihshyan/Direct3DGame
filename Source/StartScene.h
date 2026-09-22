/*--------------------------------------------------------------------

ファイル名：StartScene.h
お仕事　　：タイトル画面

設計書 Ver.3 19-1.1
    START GAME → LevelSelectScene
    OPTION     → OptionScene
    EXIT       → 終了

----------------------------------------------------------------------*/

#ifndef STARTSCENE_H
#define STARTSCENE_H

#include "Scene.h"

class StartScene : public Scene
{
public:
    StartScene();

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

    // EXIT を選んだときの確認
    bool m_confirmExit = false;
    int m_confirmChoice = 1;   // 0:YES 1:NO
    float m_confirmBlink = 0.0f;

    void DrawExitConfirm(const SceneContext& context) const;

    // ロゴのフェードイン（設計書 19-5.1 StartEffect）
    float m_time = 0.0f;
};

#endif

/*--------------------------------------------------------------------

ファイル名：LevelSelectScene.h
お仕事　　：ステージと難易度の選択

設計書 Ver.3 19-1.2
    STAGE 1 森林 / STAGE 2 平原 / STAGE 3 郊外
    難易度 / ステージ概要 / 推奨装備 / BACK

通常プレイは STAGE 1 から。
解放済みステージだけ選べる。

----------------------------------------------------------------------*/

#ifndef LEVELSELECTSCENE_H
#define LEVELSELECTSCENE_H

#include "CommonLevel.h"
#include "PlayerCharacter.h"
#include "Scene.h"

class LevelSelectScene : public Scene
{
public:
    LevelSelectScene();

    void Reset() override;

    SceneType Update(
        const InputTracker& input,
        float mouseX,
        float mouseY,
        float deltaTime
    ) override;

    void Draw(const SceneContext& context) override;

    int GetSelectedStage() const { return m_selectedStage; }
    DifficultyType GetSelectedDifficulty() const { return m_difficulty; }
    PlayerType GetSelectedPlayer() const { return m_player; }

    // クリアしたステージを解放する
    void UnlockStage(int stageIndex);

private:
    void RebuildMenu();

private:
    MenuController m_menu;

    int m_selectedStage = 0;
    DifficultyType m_difficulty = DifficultyType::Normal;
    PlayerType m_player = PlayerType::Player1;

    bool m_stageUnlocked[3] = { true, false, false };
};

#endif

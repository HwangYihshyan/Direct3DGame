/*--------------------------------------------------------------------

ファイル名：RankingScene.h
お仕事　　：名前の入力とランキングの表示

ライフを使い切ったとき、またはゲームクリア後に開く。
名前を入れる段階と、一覧を見る段階の2つを持つ。

----------------------------------------------------------------------*/

#ifndef RANKINGSCENE_H
#define RANKINGSCENE_H

#include "Ranking.h"
#include "ResultData.h"
#include "Scene.h"

class RankingScene : public Scene
{
public:
    RankingScene();

    // 結果を渡してから開く
    void Begin(const ResultData& result);

    // 名前を入れずに一覧だけ見る
    void BeginViewOnly();

    void Reset() override;

    SceneType Update(
        const InputTracker& input,
        float mouseX,
        float mouseY,
        float deltaTime
    ) override;

    void Draw(const SceneContext& context) override;

private:
    void UpdateNameInput(const InputTracker& input);
    void CommitName();

    void DrawNameInput(const SceneContext& context) const;
    void DrawList(const SceneContext& context) const;

private:
    enum class Phase
    {
        NameInput,
        List
    };

    Ranking m_ranking;
    ResultData m_result;

    Phase m_phase = Phase::List;

    std::wstring m_name;

    float m_blink = 0.0f;
    int m_rank = -1;
};

#endif

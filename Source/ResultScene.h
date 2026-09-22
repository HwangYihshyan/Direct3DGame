/*--------------------------------------------------------------------

ファイル名：ResultScene.h
お仕事　　：ステージ終了後の結果表示

設計書 Ver.3 19-1.4
    Clear Time / Kill Count / Score / 取得装備数 / 最高Rarity /
    発動Synergy / 被Damage / 与Damage / 命中率 / HeadShot率
    NEXT / RETRY / TITLE

Game Over 画面と同じ見せ方にそろえ、
経過時間・スコア・命中率だけを並べる。

----------------------------------------------------------------------*/

#ifndef RESULTSCENE_H
#define RESULTSCENE_H

#include "ResultData.h"
#include "Scene.h"

class ResultScene : public Scene
{
public:
    ResultScene();

    void SetResult(const ResultData& result);

    void Reset() override;

    SceneType Update(
        const InputTracker& input,
        float mouseX,
        float mouseY,
        float deltaTime
    ) override;

    void Draw(const SceneContext& context) override;

    // 直前に選んだ項目（NEXT なら次ステージへ進む）
    bool WantsNextStage() const { return m_wantsNext; }
    bool WantsRetry() const { return m_wantsRetry; }

private:
    MenuController m_menu;
    ResultData m_result;

    bool m_wantsNext = false;
    bool m_wantsRetry = false;
};

#endif

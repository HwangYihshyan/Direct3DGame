/*--------------------------------------------------------------------

ファイル名：ShowScore.h
お仕事　　：スコアの計算と保持

設計書 Ver.3 19-2.13
    Normal Kill / Elite Kill / Boss Kill / HeadShot /
    NoDamage Bonus / Clear Time Bonus / Rarity Pickup Bonus

----------------------------------------------------------------------*/

#ifndef SHOWSCORE_H
#define SHOWSCORE_H

#include "EquipmentTypes.h"

class ShowScore
{
public:
    void Reset();

    // 敵を倒した（scoreValue は EnemyStats の値）
    void AddKill(int scoreValue, bool elite, bool boss);

    void AddHeadShot();

    // 装備を拾った
    void AddPickup(Rarity rarity);

    // ステージクリア時のボーナス
    void AddStageClear(int elapsedSeconds, int targetSeconds, bool noDamage);

    // RETRY のペナルティでスコアを減らす
    void ApplyPenalty(int amount);

    int GetPenaltyTotal() const { return m_penaltyTotal; }

    // コンボは一定時間で切れる
    void Update(float deltaTime);

    int GetScore() const { return m_score; }
    int GetCombo() const { return m_combo; }
    int GetMaxCombo() const { return m_maxCombo; }
    int GetKillCount() const { return m_killCount; }
    int GetHeadShotCount() const { return m_headShotCount; }

    float GetComboTimer() const { return m_comboTimer; }

private:
    int m_score = 0;
    int m_combo = 0;
    int m_maxCombo = 0;
    int m_killCount = 0;
    int m_headShotCount = 0;

    float m_comboTimer = 0.0f;
    int m_penaltyTotal = 0;
};

#endif

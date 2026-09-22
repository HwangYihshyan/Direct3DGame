/*--------------------------------------------------------------------

ファイル名：ShowScore.cpp
お仕事　　：スコアの計算と保持

----------------------------------------------------------------------*/

#include "ShowScore.h"

#include <algorithm>

namespace
{
    // この秒数だけ撃破が途切れるとコンボが切れる
    constexpr float COMBO_KEEP_TIME = 3.0f;

    constexpr int HEADSHOT_SCORE = 40;
}

void ShowScore::Reset()
{
    m_score = 0;
    m_combo = 0;
    m_maxCombo = 0;
    m_killCount = 0;
    m_headShotCount = 0;
    m_comboTimer = 0.0f;
    m_penaltyTotal = 0;
}

//------------------------------------------------------------
// RETRY のペナルティ
// スコアが負にならないよう 0 で止める。
//------------------------------------------------------------
void ShowScore::ApplyPenalty(int amount)
{
    if (amount <= 0)
    {
        return;
    }

    const int applied = std::min(amount, m_score);

    m_score -= applied;
    m_penaltyTotal += applied;

    // やり直したのでコンボは切れる
    m_combo = 0;
    m_comboTimer = 0.0f;
}

void ShowScore::Update(float deltaTime)
{
    if (m_comboTimer <= 0.0f)
    {
        return;
    }

    m_comboTimer -= deltaTime;

    if (m_comboTimer <= 0.0f)
    {
        m_comboTimer = 0.0f;
        m_combo = 0;
    }
}

void ShowScore::AddKill(int scoreValue, bool elite, bool boss)
{
    ++m_killCount;

    ++m_combo;
    m_maxCombo = std::max(m_maxCombo, m_combo);
    m_comboTimer = COMBO_KEEP_TIME;

    float value = static_cast<float>(scoreValue);

    if (elite)
    {
        value *= 1.5f;
    }

    if (boss)
    {
        value *= 2.0f;
    }

    // コンボ倍率は最大2.0倍で頭打ち
    const float comboScale =
        std::min(1.0f + m_combo * 0.02f, 2.0f);

    m_score += static_cast<int>(value * comboScale);
}

void ShowScore::AddHeadShot()
{
    ++m_headShotCount;
    m_score += HEADSHOT_SCORE;
}

void ShowScore::AddPickup(Rarity rarity)
{
    // 高レアほど加点を大きくする
    static const int TABLE[RARITY_COUNT] =
    {
        10, 25, 60, 140, 320, 700, 1500
    };

    int index = static_cast<int>(rarity);

    if (index < 0 || index >= RARITY_COUNT)
    {
        index = 0;
    }

    m_score += TABLE[index];
}

void ShowScore::AddStageClear(
    int elapsedSeconds,
    int targetSeconds,
    bool noDamage
)
{
    m_score += 1000;

    //--------------------------------------------------------
    // 目安時間より早ければ加点
    //--------------------------------------------------------
    if (targetSeconds > elapsedSeconds)
    {
        m_score += (targetSeconds - elapsedSeconds) * 15;
    }

    if (noDamage)
    {
        m_score += 3000;
    }
}

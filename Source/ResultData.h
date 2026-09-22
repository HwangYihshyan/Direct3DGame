/*--------------------------------------------------------------------

ファイル名：ResultData.h
お仕事　　：GameScene から結果画面へ渡す集計データ

設計書 Ver.3 19-1.4 ResultScene
    Clear Time / Kill Count / Score / 取得装備数 / 最高Rarity /
    発動Synergy / 被Damage / 与Damage / 命中率 / HeadShot率

----------------------------------------------------------------------*/

#ifndef RESULTDATA_H
#define RESULTDATA_H

#include <string>
#include <vector>

#include "EquipmentTypes.h"

struct SynergyResult
{
    SynergyType type = SynergyType::Assault;
    int count = 0;
    int tier = 0;
};

struct ResultData
{
    // どのステージの結果か
    int stageIndex = 0;
    bool cleared = false;

    float clearTime = 0.0f;

    // 残りリトライ回数
    int livesLeft = 0;

    int killCount = 0;
    int eliteKillCount = 0;
    int score = 0;
    int maxCombo = 0;

    int pickupCount = 0;
    int mythicCount = 0;
    Rarity bestRarity = Rarity::Common;

    float damageDealt = 0.0f;
    float damageTaken = 0.0f;
    float highestDamage = 0.0f;

    int shotsFired = 0;
    int shotsHit = 0;
    int headShotCount = 0;

    std::vector<SynergyResult> synergies;

    // 装備の一覧（表示用の文字列）
    std::vector<std::wstring> equipmentNames;

    //--------------------------------------------------------
    // 命中率
    //
    // 貫通する弾は1発で複数体に当たるため、
    // 命中数が発射数を上回ることがある。
    // 表示としておかしいので 100% で止める。
    //--------------------------------------------------------
    float GetAccuracy() const
    {
        if (shotsFired <= 0)
        {
            return 0.0f;
        }

        const float value =
            static_cast<float>(shotsHit) / static_cast<float>(shotsFired);

        return (value > 1.0f) ? 1.0f : value;
    }

    float GetHeadShotRate() const
    {
        if (shotsHit <= 0)
        {
            return 0.0f;
        }

        const float value =
            static_cast<float>(headShotCount) / static_cast<float>(shotsHit);

        return (value > 1.0f) ? 1.0f : value;
    }

    void Clear()
    {
        *this = ResultData();
    }
};

#endif

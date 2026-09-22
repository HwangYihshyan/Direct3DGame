/*--------------------------------------------------------------------

ファイル名：Enemy.cpp
お仕事　　：敵の生成とステージ設定

基礎ステータスは EnemyStats.cpp の表から取り、
ここではステージ倍率と難易度倍率を掛けるだけにしている。

----------------------------------------------------------------------*/

#include "Enemy.h"
#include "EnemyStats.h"

namespace
{
    const StageConfig STAGES[STAGE_COUNT] =
    {
        //  type              quota interval maxAlive hpScale dmgScale elite% boss
        { StageType::Forest,   20,   1.60f,     6,     1.00f,   1.00f,    6,  false },
        { StageType::Suburb,   28,   1.30f,     8,     2.05f,   1.40f,   10,  false },
        { StageType::Town,     36,   1.10f,     9,     3.40f,   1.85f,   15,  false },
        { StageType::Boss,      1,   3.20f,     4,     3.60f,   2.10f,    0,  true  }
    };
}

const StageConfig& GetStageConfig(int stageIndex)
{
    if (stageIndex < 0)
    {
        stageIndex = 0;
    }

    if (stageIndex >= STAGE_COUNT)
    {
        stageIndex = STAGE_COUNT - 1;
    }

    return STAGES[stageIndex];
}

Enemy MakeEnemy(
    EnemyKind kind,
    const StageConfig& config,
    float x,
    float groundY,
    int id,
    float hpMultiplier,
    float damageMultiplier,
    float speedMultiplier
)
{
    const EnemyStats& base = GetEnemyBaseStats(kind);

    Enemy e;

    e.kind = kind;
    e.id = id;

    e.width = base.width;
    e.height = base.height;

    e.maxHp = base.maxHp * config.hpScale * hpMultiplier;
    e.hp = e.maxHp;

    e.contactDamage = base.attack * config.damageScale * damageMultiplier;

    e.speed = base.moveSpeed * speedMultiplier;
    e.attackRange = base.attackRange;
    e.attackInterval = base.attackInterval;
    e.ranged = base.ranged;

    e.shootInterval = base.shootInterval;
    e.shootRange = base.shootRange;
    e.bulletSpeed = base.bulletSpeed;
    //--------------------------------------------------------
    // 弾のDamage
    //
    // 接触Damageと同じ倍率をそのまま掛けると、
    // 後半のステージで弾だけで押し切られてしまう。
    // ステージによる強化は半分ほどに抑える。
    //--------------------------------------------------------
    const float bulletScale =
        1.0f + (config.damageScale - 1.0f) * 0.5f;

    e.bulletDamage = base.attack * base.bulletDamage
                   * bulletScale * damageMultiplier;
    e.shootSpread = base.shootSpread;
    e.keepDistance = base.keepDistance;

    //--------------------------------------------------------
    // 全員が同時に撃たないよう、最初だけ待ち時間をずらす
    //--------------------------------------------------------
    if (e.shootInterval > 0.0f)
    {
        const float offset =
            static_cast<float>((id * 37) % 100) * 0.01f;

        e.shootTimer = e.shootInterval * (0.4f + offset * 0.8f);
    }

    e.armorReduction = base.defense;
    e.scoreValue = base.scoreValue;
    e.dropRate = base.dropRate;

    e.x = x;
    e.y = groundY - e.height;
    e.grounded = true;

    return e;
}

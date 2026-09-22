/*--------------------------------------------------------------------

ファイル名：Enemy.h
お仕事　　：敵1体分の状態

設計書 Ver.3 19-2.7 ～ 19-2.11
    Enemy1 近接 / Enemy2 遠距離 / Enemy3 Tank /
    Enemy4 Fast / Enemy5 Elite・Boss

基礎ステータスは EnemyStats.h、
湧かせ方は EnemySpawner.h が持つ。
このファイルは「今どういう状態か」だけを扱う。

座標はワールド座標。画面への変換は Camera が行う。

----------------------------------------------------------------------*/

#ifndef ENEMY_H
#define ENEMY_H

#include "EquipmentGenerator.h"

enum class EnemyKind
{
    Grunt,      // Enemy1 近接
    Shooter,    // Enemy2 遠距離
    Brute,      // Enemy3 Tank
    Runner,     // Enemy4 Fast
    Elite,      // Enemy5 Elite
    Boss        // Enemy5 Boss
};

struct Enemy
{
    EnemyKind kind = EnemyKind::Grunt;

    int id = 0;

    float x = 0.0f;
    float y = 0.0f;
    float width = 62.0f;
    float height = 112.0f;

    float velocityY = 0.0f;
    bool grounded = false;

    float hp = 100.0f;
    float maxHp = 100.0f;

    float speed = 90.0f;
    float contactDamage = 40.0f;
    float attackCooldown = 0.0f;
    float attackInterval = 0.85f;
    float attackRange = 40.0f;
    bool ranged = false;

    //--------------------------------------------------------
    // 射撃
    //--------------------------------------------------------
    float shootInterval = 0.0f;
    float shootRange = 0.0f;
    float bulletSpeed = 420.0f;
    float bulletDamage = 0.5f;
    float shootSpread = 0.10f;
    bool keepDistance = false;

    // 装甲による軽減率（Armor Penetrationで減る）
    float armorReduction = 0.0f;

    int scoreValue = 100;
    float dropRate = 18.0f;

    bool alive = true;
    float hitFlash = 0.0f;

    // Cripple Shot による減速
    float slowTimer = 0.0f;
    float slowFactor = 1.0f;

    // 見た目・行動用のタイマー
    float shootTimer = 0.0f;
    float actionTimer = 0.0f;
    int phase = 0;

    float CenterX() const { return x + width * 0.5f; }
    float CenterY() const { return y + height * 0.5f; }

    float GetHpRatio() const
    {
        return (maxHp > 0.0f) ? (hp / maxHp) : 0.0f;
    }

    bool IsEliteOrBoss() const
    {
        return kind == EnemyKind::Elite || kind == EnemyKind::Boss;
    }

    // この線より上に当たればHeadShot
    float GetHeadLineY() const
    {
        return y + height * 0.30f;
    }
};

//------------------------------------------------------------
// ステージ設定
//------------------------------------------------------------
struct StageConfig
{
    StageType type;
    int killQuota;          // クリアに必要な撃破数
    float spawnInterval;    // 湧き間隔（秒）
    int maxAlive;           // 同時出現数
    float hpScale;
    float damageScale;
    int elitePercent;       // Elite出現率
    bool bossStage;
};

constexpr int STAGE_COUNT = 4;

const StageConfig& GetStageConfig(int stageIndex);

//------------------------------------------------------------
// 敵を1体作る
//   difficultyScale は CommonLevel の難易度補正
//------------------------------------------------------------
Enemy MakeEnemy(
    EnemyKind kind,
    const StageConfig& config,
    float x,
    float groundY,
    int id,
    float hpMultiplier = 1.0f,
    float damageMultiplier = 1.0f,
    float speedMultiplier = 1.0f
);

#endif

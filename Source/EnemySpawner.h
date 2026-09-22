/*--------------------------------------------------------------------

ファイル名：EnemySpawner.h
お仕事　　：敵の湧きをGameSceneから分離する

設計書 Ver.3 19-8.8
    入力：Stage / ElapsedTime / Difficulty
    出力：Enemy Spawn Request

湧かせる「場所」はマップのSpawnPointから選ぶ。
プレイヤーの近くには湧かせない。

----------------------------------------------------------------------*/

#ifndef ENEMYSPAWNER_H
#define ENEMYSPAWNER_H

#include <random>
#include <vector>

#include "CommonLevel.h"
#include "Enemy.h"
#include "MapBase.h"

//------------------------------------------------------------
// 湧き要求
//------------------------------------------------------------
struct SpawnRequest
{
    EnemyKind kind = EnemyKind::Grunt;
    float worldX = 0.0f;
};

class EnemySpawner
{
public:
    EnemySpawner();

    void Initialize(
        const MapBase* map,
        const StageConfig* config,
        DifficultyType difficulty
    );

    void Reset();

    // 毎フレーム呼ぶ。湧かせるべき敵を返す（無ければ空）
    std::vector<SpawnRequest> Update(
        float deltaTime,
        int aliveCount,
        int killCount,
        float playerWorldX
    );

    //--------------------------------------------------------
    // 区画ぶんの敵をまとめて出す（ウェーブ方式）
    //   left / right … その区画のワールド座標
    //--------------------------------------------------------
    std::vector<SpawnRequest> SpawnWave(
        float left,
        float right,
        int count,
        float playerWorldX
    );

    // ボスを1体だけ湧かせる
    SpawnRequest MakeBossRequest(float playerWorldX) const;

    bool IsBossSpawned() const { return m_bossSpawned; }
    void MarkBossSpawned() { m_bossSpawned = true; }

    // 同時出現上限（難易度補正込み）
    int GetMaxAlive() const { return m_maxAlive; }

private:
    EnemyKind RollKind();
    float PickSpawnX(float playerWorldX);

private:
    std::mt19937 m_rng;

    const MapBase* m_map = nullptr;
    const StageConfig* m_config = nullptr;

    DifficultyModifier m_modifier;

    float m_timer = 0.0f;
    float m_interval = 1.6f;
    int m_maxAlive = 6;
    int m_elitePercent = 6;

    bool m_bossSpawned = false;
};

#endif

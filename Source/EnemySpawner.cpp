/*--------------------------------------------------------------------

ファイル名：EnemySpawner.cpp
お仕事　　：敵の湧き制御

----------------------------------------------------------------------*/

#include "EnemySpawner.h"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace
{
    // この距離より近い場所には湧かせない
    constexpr float MIN_SPAWN_DISTANCE = 420.0f;

    // 画面外すぎても出てこないので上限も設ける
    constexpr float MAX_SPAWN_DISTANCE = 1100.0f;
}

EnemySpawner::EnemySpawner()
{
    m_rng.seed(
        static_cast<unsigned>(
            std::chrono::high_resolution_clock::now()
            .time_since_epoch().count()
            )
    );
}

void EnemySpawner::Initialize(
    const MapBase* map,
    const StageConfig* config,
    DifficultyType difficulty
)
{
    m_map = map;
    m_config = config;
    m_modifier = GetDifficultyModifier(difficulty);

    Reset();
}

void EnemySpawner::Reset()
{
    m_bossSpawned = false;
    m_timer = 1.0f;

    if (!m_config)
    {
        return;
    }

    m_interval = m_config->spawnInterval;

    m_maxAlive = std::max(
        2,
        static_cast<int>(m_config->maxAlive * m_modifier.enemyCount + 0.5f)
    );

    m_elitePercent = static_cast<int>(
        m_config->elitePercent * m_modifier.eliteRate + 0.5f
        );
}

//------------------------------------------------------------
// 敵種を決める
//------------------------------------------------------------
EnemyKind EnemySpawner::RollKind()
{
    std::uniform_int_distribution<int> dist(1, 100);

    if (m_elitePercent > 0 && dist(m_rng) <= m_elitePercent)
    {
        return EnemyKind::Elite;
    }

    const int roll = dist(m_rng);

    // 近接 → 遠距離 → 高速 → Tank の順で確率を割り当てる
    if (roll <= 38) return EnemyKind::Grunt;
    if (roll <= 60) return EnemyKind::Shooter;
    if (roll <= 82) return EnemyKind::Runner;

    return EnemyKind::Brute;
}

//------------------------------------------------------------
// 湧く場所を決める
//   マップのSpawnPointのうち、
//   プレイヤーから適度に離れた点を選ぶ。
//------------------------------------------------------------
float EnemySpawner::PickSpawnX(float playerWorldX)
{
    if (!m_map)
    {
        return playerWorldX + MIN_SPAWN_DISTANCE;
    }

    const std::vector<float>& points = m_map->GetSpawnPoints();

    std::vector<float> candidates;

    for (float x : points)
    {
        const float distance = std::fabs(x - playerWorldX);

        if (distance >= MIN_SPAWN_DISTANCE && distance <= MAX_SPAWN_DISTANCE)
        {
            candidates.push_back(x);
        }
    }

    //--------------------------------------------------------
    // 条件に合う点が無い場合は、
    // プレイヤーの左右どちらかへ固定距離で湧かせる
    //--------------------------------------------------------
    if (candidates.empty())
    {
        std::uniform_int_distribution<int> side(0, 1);

        const float offset =
            (side(m_rng) == 0) ? -MIN_SPAWN_DISTANCE : MIN_SPAWN_DISTANCE;

        return std::clamp(
            playerWorldX + offset,
            m_map->GetLeftLimit() + 20.0f,
            m_map->GetRightLimit() - 120.0f
        );
    }

    std::uniform_int_distribution<int> pick(
        0, static_cast<int>(candidates.size()) - 1
    );

    return candidates[pick(m_rng)];
}

//------------------------------------------------------------
// 毎フレームの更新
//------------------------------------------------------------
std::vector<SpawnRequest> EnemySpawner::Update(
    float deltaTime,
    int aliveCount,
    int killCount,
    float playerWorldX
)
{
    std::vector<SpawnRequest> result;

    if (!m_config)
    {
        return result;
    }

    // ボスステージは Update では湧かせすぎない
    if (!m_config->bossStage && killCount >= m_config->killQuota)
    {
        return result;
    }

    if (aliveCount >= m_maxAlive)
    {
        return result;
    }

    m_timer -= deltaTime;

    if (m_timer > 0.0f)
    {
        return result;
    }

    m_timer = m_interval;

    SpawnRequest request;

    request.kind = RollKind();
    request.worldX = PickSpawnX(playerWorldX);

    result.push_back(request);

    return result;
}

//------------------------------------------------------------
// 区画ぶんの敵をまとめて出す
//
// プレイヤーの近くには湧かせず、
// 区画の中で左右に散らして配置する。
//------------------------------------------------------------
std::vector<SpawnRequest> EnemySpawner::SpawnWave(
    float left,
    float right,
    int count,
    float playerWorldX
)
{
    std::vector<SpawnRequest> result;

    if (count <= 0 || right - left < 200.0f)
    {
        return result;
    }

    constexpr float MARGIN = 110.0f;

    const float from = left + MARGIN;
    const float to = right - MARGIN;

    std::uniform_real_distribution<float> jitter(-60.0f, 60.0f);

    for (int i = 0; i < count; ++i)
    {
        //----------------------------------------------------
        // 区画を等分した位置を基準に、少しばらけさせる
        //----------------------------------------------------
        const float t =
            (count == 1) ? 0.5f : static_cast<float>(i) / (count - 1);

        float x = from + (to - from) * t + jitter(m_rng);

        //----------------------------------------------------
        // プレイヤーに近すぎる場合は遠い側へ寄せる
        //----------------------------------------------------
        if (std::fabs(x - playerWorldX) < 260.0f)
        {
            x = (playerWorldX < (left + right) * 0.5f) ? to : from;
        }

        SpawnRequest request;

        request.kind = RollKind();
        request.worldX = std::clamp(x, from, to);

        result.push_back(request);
    }

    return result;
}

SpawnRequest EnemySpawner::MakeBossRequest(float playerWorldX) const
{
    SpawnRequest request;

    request.kind = EnemyKind::Boss;

    if (m_map)
    {
        // プレイヤーから離れた位置に出す
        const float right = m_map->GetRightLimit() - 320.0f;
        const float left = m_map->GetLeftLimit() + 320.0f;

        request.worldX =
            (playerWorldX < m_map->GetWorldWidth() * 0.5f) ? right : left;
    }
    else
    {
        request.worldX = playerWorldX + 600.0f;
    }

    return request;
}

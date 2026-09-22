/*--------------------------------------------------------------------

ファイル名：EnemyStats.h
お仕事　　：敵1体分の基礎ステータス

設計書 Ver.3 19-8.7
    HP / Attack / Defense / MoveSpeed / AttackRange /
    AttackInterval / ScoreValue / DropRate / EliteFlag / BossFlag

Enemy.cpp へ数値を直接書かず、ここへ集約する。

----------------------------------------------------------------------*/

#ifndef ENEMYSTATS_H
#define ENEMYSTATS_H

enum class EnemyKind;

struct EnemyStats
{
    float maxHp = 900.0f;
    float attack = 46.0f;

    // 0.0～1.0。Armor Penetration で削られる
    float defense = 0.0f;

    float moveSpeed = 92.0f;

    // 接触または射撃を行う距離
    float attackRange = 40.0f;

    // 攻撃の間隔（秒）
    float attackInterval = 0.85f;

    // 見た目の大きさ
    float width = 62.0f;
    float height = 112.0f;

    int scoreValue = 100;

    // 装備ドロップ率（%）
    float dropRate = 18.0f;

    bool eliteFlag = false;
    bool bossFlag = false;

    // 遠距離攻撃を行うか
    bool ranged = false;

    //--------------------------------------------------------
    // 射撃の設定
    //
    // 弱い敵ほど間隔が長く、狙いも甘い。
    // 強い敵ほど速く正確に撃ってくる。
    //--------------------------------------------------------
    float shootInterval = 0.0f;   // 0 なら撃たない
    float shootRange = 0.0f;      // この距離まで届く
    float bulletSpeed = 420.0f;
    float bulletDamage = 0.5f;    // 接触Damageに対する割合
    float shootSpread = 0.10f;    // 狙いのぶれ（ラジアン）

    // 近づかず距離を取るか
    bool keepDistance = false;
};

const EnemyStats& GetEnemyBaseStats(EnemyKind kind);

#endif

/*--------------------------------------------------------------------

ファイル名：EnemyStats.cpp
お仕事　　：敵種別ごとの基礎ステータス表

設計書 Ver.3 19-2.7 ～ 19-2.11
    Enemy1 近接 / Enemy2 遠距離 / Enemy3 Tank /
    Enemy4 Fast / Enemy5 Elite・Boss

----------------------------------------------------------------------*/

#include "Enemy.h"
#include "EnemyStats.h"

namespace
{
    EnemyStats MakeGrunt()
    {
        EnemyStats s;
        s.maxHp = 900.0f;
        s.attack = 46.0f;
        s.defense = 0.0f;
        s.moveSpeed = 92.0f;
        s.attackRange = 40.0f;
        s.attackInterval = 0.85f;
        s.width = 84.0f;
        s.height = 94.0f;
        s.scoreValue = 100;
        s.dropRate = 18.0f;

        // 近づきながら、たまに撃つ
        s.shootInterval = 3.8f;
        s.shootRange = 540.0f;
        s.bulletSpeed = 400.0f;
        s.bulletDamage = 0.34f;
        s.shootSpread = 0.12f;

        return s;
    }

    EnemyStats MakeRunner()
    {
        EnemyStats s;
        s.maxHp = 560.0f;
        s.attack = 34.0f;
        s.moveSpeed = 178.0f;
        s.attackRange = 36.0f;
        s.attackInterval = 0.70f;
        s.width = 74.0f;
        s.height = 82.0f;
        s.scoreValue = 120;
        s.dropRate = 18.0f;

        // 一番弱い。ほとんど当たらない牽制程度
        s.shootInterval = 5.4f;
        s.shootRange = 420.0f;
        s.bulletSpeed = 380.0f;
        s.bulletDamage = 0.26f;
        s.shootSpread = 0.20f;

        return s;
    }

    EnemyStats MakeBrute()
    {
        EnemyStats s;
        s.maxHp = 2300.0f;
        s.attack = 82.0f;
        s.defense = 0.15f;
        s.moveSpeed = 58.0f;
        s.attackRange = 52.0f;
        s.attackInterval = 1.10f;
        s.width = 112.0f;
        s.height = 126.0f;
        s.scoreValue = 260;
        s.dropRate = 22.0f;

        // 遅いが1発が重い
        s.shootInterval = 3.6f;
        s.shootRange = 500.0f;
        s.bulletSpeed = 330.0f;
        s.bulletDamage = 0.58f;
        s.shootSpread = 0.13f;

        return s;
    }

    // 遠距離型。近づかれると下がる
    EnemyStats MakeShooter()
    {
        EnemyStats s;
        s.maxHp = 720.0f;
        s.attack = 52.0f;
        s.defense = 0.0f;
        s.moveSpeed = 74.0f;
        s.attackRange = 460.0f;
        s.attackInterval = 1.60f;
        s.width = 80.0f;
        s.height = 90.0f;
        s.scoreValue = 160;
        s.dropRate = 20.0f;
        s.ranged = true;

        // 撃つことが仕事。距離を取り続ける
        s.shootInterval = 2.0f;
        s.shootRange = 620.0f;
        s.bulletSpeed = 480.0f;
        s.bulletDamage = 0.52f;
        s.shootSpread = 0.05f;
        s.keepDistance = true;

        return s;
    }

    EnemyStats MakeElite()
    {
        EnemyStats s;
        s.maxHp = 3400.0f;
        s.attack = 92.0f;
        s.defense = 0.20f;
        s.moveSpeed = 108.0f;
        s.attackRange = 56.0f;
        s.attackInterval = 0.80f;
        s.width = 102.0f;
        s.height = 114.0f;
        s.scoreValue = 800;
        s.dropRate = 55.0f;
        s.eliteFlag = true;

        // 速く正確。近づきながら撃ってくる
        s.shootInterval = 2.0f;
        s.shootRange = 640.0f;
        s.bulletSpeed = 500.0f;
        s.bulletDamage = 0.48f;
        s.shootSpread = 0.06f;

        return s;
    }

    EnemyStats MakeBoss()
    {
        EnemyStats s;
        s.maxHp = 10000.0f;
        s.attack = 130.0f;
        s.defense = 0.25f;
        s.moveSpeed = 62.0f;
        s.attackRange = 520.0f;
        s.attackInterval = 1.20f;
        s.width = 212.0f;
        s.height = 236.0f;
        s.scoreValue = 5000;
        s.dropRate = 100.0f;
        s.bossFlag = true;
        s.ranged = true;

        // ボスは専用の攻撃を持っているので、共通の射撃は使わない
        s.shootInterval = 0.0f;
        s.keepDistance = true;

        return s;
    }

    const EnemyStats GRUNT = MakeGrunt();
    const EnemyStats RUNNER = MakeRunner();
    const EnemyStats BRUTE = MakeBrute();
    const EnemyStats SHOOTER = MakeShooter();
    const EnemyStats ELITE = MakeElite();
    const EnemyStats BOSS = MakeBoss();
}

const EnemyStats& GetEnemyBaseStats(EnemyKind kind)
{
    switch (kind)
    {
    case EnemyKind::Grunt:   return GRUNT;
    case EnemyKind::Runner:  return RUNNER;
    case EnemyKind::Brute:   return BRUTE;
    case EnemyKind::Shooter: return SHOOTER;
    case EnemyKind::Elite:   return ELITE;
    case EnemyKind::Boss:    return BOSS;
    }

    return GRUNT;
}

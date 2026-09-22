/*--------------------------------------------------------------------

ファイル名：GameScene.cpp
お仕事　　：BULLET DRAFT ゲーム本体

操作：
    A / D          移動
    SPACE          ジャンプ
    SHIFT          ダッシュ（無敵あり）
    左クリック     射撃
    R              リロード
    Q / 1 / 2      メイン・サブ切り替え
    F              足元の装備を拾って装備
    TAB            装備一覧
    ESC            タイトルへ戻る

----------------------------------------------------------------------*/
#define NOMINMAX
#include "GameScene.h"
#include "Rarity.h"
#include "AudioManager.h"
#include "TerrainTileBottom.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>

namespace
{
    constexpr float SCREEN_WIDTH = 1280.0f;
    constexpr float SCREEN_HEIGHT = 720.0f;


    constexpr float GRAVITY = 1900.0f;
    constexpr float JUMP_POWER = 700.0f;
    constexpr float BASE_MOVE_SPEED = 300.0f;

    //--------------------------------------------------------
    // player_sheet.png
    //   1セル 61x68、横7列×縦2行。
    //   上段：歩行、下段：構え・射撃・膝立ち
    //--------------------------------------------------------
    constexpr float SPRITE_W = 61.0f;
    constexpr float SPRITE_H = 68.0f;

    constexpr int WALK_FRAMES = 5;
    constexpr float WALK_FRAME_TIME = 0.10f;

    // 膝立ちのポーズ（ジャンプ中に使う）
    constexpr int JUMP_FRAME_COLUMN = 5;
    constexpr int JUMP_FRAME_ROW = 1;

    // 射撃の構え
    constexpr int SHOOT_FRAME_COLUMN = 3;
    constexpr int SHOOT_MUZZLE_COLUMN = 4;
    constexpr int SHOOT_FRAME_ROW = 1;

    constexpr float SHOOT_ANIM_TIME = 0.16f;

    // ダッシュ
    constexpr float DASH_TIME = 0.16f;
    constexpr float DASH_SPEED = 1250.0f;
    constexpr float DASH_COOLDOWN = 1.30f;

    // バーストライフルのバースト間隔
    constexpr float BURST_GAP = 0.24f;

    // 武器スロットの添字
    constexpr int MAIN_WEAPON_INDEX = 0;
    constexpr int SUB_WEAPON_INDEX = 1;

    //--------------------------------------------------------
    // 乱数
    //--------------------------------------------------------
    std::mt19937& Rng()
    {
        static std::mt19937 rng{ std::random_device{}() };
        return rng;
    }

    float RandFloat(float minValue, float maxValue)
    {
        if (minValue >= maxValue)
        {
            return minValue;
        }

        std::uniform_real_distribution<float> dist(minValue, maxValue);
        return dist(Rng());
    }

    int RandInt(int minValue, int maxValue)
    {
        if (minValue >= maxValue)
        {
            return minValue;
        }

        std::uniform_int_distribution<int> dist(minValue, maxValue);
        return dist(Rng());
    }

    bool RollChance(float probability)
    {
        return RandFloat(0.0f, 1.0f) < probability;
    }

    //--------------------------------------------------------
    // 円と矩形の当たり判定
    //--------------------------------------------------------
    bool CircleHitsRect(
        float cx, float cy, float radius,
        float rx, float ry, float rw, float rh
    )
    {
        const float nearestX = std::max(rx, std::min(cx, rx + rw));
        const float nearestY = std::max(ry, std::min(cy, ry + rh));

        const float dx = cx - nearestX;
        const float dy = cy - nearestY;

        return (dx * dx + dy * dy) <= (radius * radius);
    }

    bool RectsOverlap(
        float ax, float ay, float aw, float ah,
        float bx, float by, float bw, float bh
    )
    {
        return
            ax < bx + bw &&
            ax + aw > bx &&
            ay < by + bh &&
            ay + ah > by;
    }

    //--------------------------------------------------------
    // 敵の種類の表示名
    //--------------------------------------------------------
    const wchar_t* GetEnemyName(EnemyKind kind)
    {
        switch (kind)
        {
        case EnemyKind::Grunt:   return L"歩兵";
        case EnemyKind::Shooter: return L"射手";
        case EnemyKind::Runner:  return L"遊撃兵";
        case EnemyKind::Brute:   return L"重甲兵";
        case EnemyKind::Elite:   return L"エリート";
        case EnemyKind::Boss:    return L"ボス";
        }

        return L"敵";
    }

    std::wstring FormatFloat(float value, int decimals)
    {
        wchar_t buffer[64];

        if (decimals <= 0)
        {
            swprintf_s(buffer, L"%.0f", value);
        }
        else
        {
            swprintf_s(buffer, L"%.1f", value);
        }

        return buffer;
    }
}

//============================================================
// 初期化
//============================================================

void GameScene::LoadTextures()
{
    //--------------------------------------------------------
    // 画像が無くても起動できるようにする（設計書 19-13）
    // 読み込めなかったものは nullptr のままになる。
    //--------------------------------------------------------
    m_characterBitmap = m_resources.GetBitmap(
        GetPlayerCharacter(m_playerType).spriteSheet
    );
    m_enemyBitmap = m_resources.GetBitmap(L"Assets\\texture\\enemies.png");
    m_bossBitmap = m_resources.GetBitmap(L"Assets\\texture\\boss.png");
    m_itemBitmap = m_resources.GetBitmap(L"Assets\\texture\\items.png");
    m_bulletBitmap = m_resources.GetBitmap(L"Assets\\texture\\bullets.png");
    m_iconBitmap = m_resources.GetBitmap(L"Assets\\texture\\synergy_icons.png");
    m_chestBitmap = m_resources.GetBitmap(L"Assets\\texture\\chests.png");

    // HP回復アイテム（1枚絵・64角）
    m_healthBitmap =
        m_resources.GetBitmap(L"Assets\\texture\\health_pickup.png");

    m_ui.SetIconBitmap(m_iconBitmap);

    m_ui.SetStatIconBitmap(
        m_resources.GetBitmap(L"Assets\\texture\\stat_icons.png")
    );

    m_ui.SetSkillIconBitmap(
        m_resources.GetBitmap(L"Assets\\texture\\skill_icons.png")
    );
}


bool GameScene::CreateBrushes()
{
    HRESULT hr = m_renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f),
        &m_brush
    );

    if (FAILED(hr))
    {
        return false;
    }

    hr = m_renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(0.02f, 0.035f, 0.05f, 0.90f),
        &m_panelBrush
    );

    return SUCCEEDED(hr);
}

bool GameScene::CreateTextFormats(IDWriteFactory* writeFactory)
{
    if (!writeFactory)
    {
        return false;
    }

    struct FormatRequest
    {
        float size;
        DWRITE_FONT_WEIGHT weight;
        IDWriteTextFormat** target;
    };

    const FormatRequest requests[] =
    {
        { 18.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD, &m_smallFormat },
        { 22.0f, DWRITE_FONT_WEIGHT_BOLD,      &m_normalFormat },
        { 28.0f, DWRITE_FONT_WEIGHT_BOLD,      &m_headerFormat },
        { 54.0f, DWRITE_FONT_WEIGHT_BLACK,     &m_bigFormat }
    };

    for (const FormatRequest& request : requests)
    {
        HRESULT hr = writeFactory->CreateTextFormat(
            L"Yu Gothic UI",
            nullptr,
            request.weight,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            request.size,
            L"ja-jp",
            request.target
        );

        if (FAILED(hr))
        {
            return false;
        }

        (*request.target)->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

        (*request.target)->SetParagraphAlignment(
            DWRITE_PARAGRAPH_ALIGNMENT_NEAR
        );

        (*request.target)->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
    }

    m_bigFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_bigFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    return true;
}

bool GameScene::Initialize(
    ID2D1RenderTarget* renderTarget,
    IDWriteFactory* writeFactory
)
{
    if (!renderTarget)
    {
        return false;
    }

    m_renderTarget = renderTarget;

    if (!m_resources.Initialize(renderTarget))
    {
        return false;
    }

    if (!CreateBrushes())
    {
        return false;
    }

    if (!CreateTextFormats(writeFactory))
    {
        return false;
    }

    // 画像が無くても起動できるようにする（背景は単色で代用）
    m_ui.Initialize(
        renderTarget,
        m_brush.Get(),
        m_panelBrush.Get(),
        m_smallFormat.Get(),
        m_normalFormat.Get(),
        m_headerFormat.Get(),
        m_bigFormat.Get()
    );

    LoadTextures();

    m_effectSystem.Initialize(&m_resources);

    m_effectSystem.SetBulletRow(
        (m_playerType == PlayerType::Player1) ? 0 : 1
    );

    Reset();

    return true;
}

//============================================================
// マップ依存の値
//============================================================

float GameScene::GroundY() const
{
    return m_map ? m_map->GetGroundY() : 620.0f;
}

float GameScene::LeftLimit() const
{
    return m_map ? m_map->GetLeftLimit() : 20.0f;
}

float GameScene::RightLimit() const
{
    return m_map ? m_map->GetRightLimit() : 1260.0f;
}

float GameScene::AreaLeft() const
{
    return m_areaIndex * SCREEN_WIDTH;
}

float GameScene::AreaRight() const
{
    return AreaLeft() + SCREEN_WIDTH;
}

float GameScene::ToScreenX(float worldX) const
{
    return m_camera.WorldToScreenX(worldX);
}


//============================================================
// リセット
//============================================================

void GameScene::Reset()
{
    Reset(0, m_difficulty);
}

void GameScene::Reset(int stageIndex, DifficultyType difficulty)
{
    Reset(stageIndex, difficulty, m_playerType);
}

void GameScene::Reset(
    int stageIndex,
    DifficultyType difficulty,
    PlayerType player
)
{
    m_difficulty = difficulty;
    m_playerType = player;

    m_loadout.Clear();

    // 初期シナジーがキャラで変わるので、先に決めておく
    m_loadout.SetCharacter(player);

    m_skills[0] = CreatePlayerSkill(player, SkillSlot::First);
    m_skills[1] = CreatePlayerSkill(player, SkillSlot::Second);
    m_loadout.GiveStarterGear();

    m_hp = m_loadout.GetStats().maxHP;
    m_shield = 0.0f;

    m_playerX = 200.0f;
    m_playerY = 620.0f - m_playerHeight;
    m_velocityY = 0.0f;
    m_isGrounded = true;
    m_facingRight = true;

    m_invincibleTimer = 0.0f;
    m_damageFlash = 0.0f;

    m_dashTimer = 0.0f;
    m_dashCooldown = 0.0f;
    m_dropThroughTimer = 0.0f;

    m_animTimer = 0.0f;
    m_animFrame = 0;
    m_isMoving = false;
    m_shootAnimTimer = 0.0f;
    m_recoilOffset = 0.0f;

    m_usingMain = true;
    m_shotTimer = 0.0f;
    m_reloadTimer = 0.0f;
    m_isReloading = false;
    m_burstRemaining = 0;
    m_burstTimer = 0.0f;

    m_bullets.clear();
    m_enemies.clear();
    m_explosions.clear();
    m_damageTexts.clear();
    m_worldMessages.clear();
    m_enemyHpTracks.clear();
    m_drops.clear();
    m_ammoPickups.clear();
    m_healthPickups.clear();

    m_nextEnemyId = 1;
    m_totalKills = 0;

    m_showInventory = false;
    m_hoveredDrop = -1;

    m_toastText.clear();
    m_toastTimer = 0.0f;

    m_backToTitle = false;
    m_optionRequested = false;

    m_input.Clear();

    LoadTextures();

    m_score.Reset();
    m_scoreDisplay.Reset();

    // 1回のプレイで使えるリトライは3回まで
    m_lives = LIFE_MAX;
    m_inventory.Clear();
    m_result.Clear();

    m_eliteKillCount = 0;

    RefreshWeapons(true);

    BeginStage(stageIndex);

    m_lastUpdate = std::chrono::steady_clock::now();
}

//============================================================
// ステージ
//============================================================

const StageConfig& GameScene::CurrentStage() const
{
    return GetStageConfig(m_stageIndex);
}

void GameScene::BeginStage(int stageIndex)
{
    m_stageIndex = std::clamp(stageIndex, 0, STAGE_COUNT - 1);

    //--------------------------------------------------------
    // マップを作り、カメラと湧きを初期化する
    //--------------------------------------------------------
    m_map = CreateMap(m_stageIndex);

    m_camera.Initialize(m_map->GetWorldWidth(), SCREEN_WIDTH, SCREEN_HEIGHT);

    m_spawner.Initialize(m_map.get(), &CurrentStage(), m_difficulty);

    // プレイヤーをマップ左端へ戻す
    m_playerX = m_map->GetLeftLimit() + 140.0f;
    m_playerY = m_map->GetGroundY() - m_playerHeight;
    m_velocityY = 0.0f;
    m_isGrounded = true;

    m_camera.SnapTo(SCREEN_WIDTH * 0.5f);

    //--------------------------------------------------------
    // 1画面ぶんを1区画として分割する
    //--------------------------------------------------------
    m_areaCount = std::max(
        1,
        static_cast<int>(m_map->GetWorldWidth() / SCREEN_WIDTH)
    );

    m_areaIndex = 0;
    m_areaCleared = false;
    m_waveSpawned = false;
    m_advanceHintTimer = 0.0f;

    m_killCount = 0;
    m_spawnTimer = 1.0f;
    m_bossSpawned = false;

    m_stageTime = 0.0f;
    m_tookDamageThisStage = false;
    m_stageClearPending = false;
    m_stageClearWait = 0.0f;

    //--------------------------------------------------------
    // 目標を1つ選び、数え始める基準を控えておく
    //--------------------------------------------------------
    m_objective = RollObjective(m_stageIndex, m_difficulty);

    m_headShotBase = m_score.GetHeadShotCount();
    m_eliteBase = m_eliteKillCount;
    m_pickupBase = m_inventory.GetTotalPickup();
    m_comboBase = 0;

    m_enemies.clear();
    m_bullets.clear();
    m_explosions.clear();
    m_damageTexts.clear();
    m_drops.clear();
    m_ammoPickups.clear();
    m_healthPickups.clear();

    //--------------------------------------------------------
    // 障害物を配置し直す
    //--------------------------------------------------------
    m_obstacles.clear();

    for (const MapObstacle& source : m_map->GetObstacles())
    {
        ObstacleState state;

        state.data = source;
        state.hp = source.maxHp;
        state.alive = true;

        m_obstacles.push_back(state);
    }

    m_terrainBitmap = m_resources.GetBitmap(m_map->GetTerrainTexture());

    //--------------------------------------------------------
    // このステージの BGM へ切り替える。
    // 同じ曲ならそのまま流れ続けるので、
    // リトライで頭出しになることはない。
    //--------------------------------------------------------
    Audio::PlayBgm(m_map->GetBgmPath());

    m_effectSystem.Reset();

    m_dropSystem.NotifyStageStart();

    SpawnChest();

    m_state = GameState::StageIntro;
    m_stateTimer = 2.4f;

    // ステージ開始時に弾薬を補給し、シールドを張り直す
    RefreshWeapons(true);

    const PlayerStats& stats = m_loadout.GetStats();
    m_shield = stats.maxHP * stats.shieldPercent;

    // ステージクリアの報酬として最大HPの35%を回復する。
    // 全回復にしないのは、Vampire / Guardian を積む価値を残すため。
    if (m_stageIndex > 0)
    {
        HealPlayer(stats.maxHP * 0.35f);
    }

    m_loadout.GetSynergy().ResetRuntime();
}

void GameScene::AdvanceStage()
{
    BeginStage(m_stageIndex + 1);
}

void GameScene::RetryStage()
{
    if (m_lives <= 0)
    {
        return;
    }

    --m_lives;

    //--------------------------------------------------------
    // 装備はそのまま持ち越す。
    // 代わりにスコアを減らすことで、やり直しに重みを持たせる。
    //   減点は「今のスコアの25%」か「1500点」の大きい方。
    //--------------------------------------------------------
    const int penalty = std::max(1500, m_score.GetScore() / 4);

    m_score.ApplyPenalty(penalty);

    m_toastText = L"リトライ　Score -" + std::to_wstring(penalty);
    m_toastTimer = 2.4f;

    BeginStage(m_stageIndex);

    // HPは満タンに戻す
    m_hp = m_loadout.GetStats().maxHP;

    m_state = GameState::StageIntro;
    m_stateTimer = 2.4f;
}


void GameScene::SpawnChest()
{
    //--------------------------------------------------------
    // 宝箱の位置はマップが持っている（設計書 19-2.14 DropPoint）
    //--------------------------------------------------------
    for (int i = 0; i < CHEST_MAX; ++i)
    {
        m_chests[i].active = false;
        m_chests[i].opened = false;
    }

    if (!m_map)
    {
        return;
    }

    const std::vector<MapChestPoint>& points = m_map->GetChestPoints();

    const int fortune = m_loadout.GetSynergy().GetFortuneLevel();

    //--------------------------------------------------------
    // 宝箱の中身は、ステージ目標に合うシナジーを出やすくする。
    // Fortune が高いほど、その寄せ方も強くなる。
    //--------------------------------------------------------
    m_dropSystem.SetObjectiveSynergies(
        GetObjectiveSynergies(m_objective.type),
        fortune
    );

    for (int i = 0; i < CHEST_MAX && i < static_cast<int>(points.size()); ++i)
    {
        TreasureChest& chest = m_chests[i];

        chest.active = true;
        chest.opened = false;
        chest.x = points[i].worldX;

        //----------------------------------------------------
        // baseY が未指定なら地面。
        // 足場の上へ置くと、登る理由になる。
        //----------------------------------------------------
        const float baseY = (points[i].baseY > 0.0f)
            ? points[i].baseY
            : m_map->GetGroundY();

        chest.y = baseY - 52.0f;

        //----------------------------------------------------
        // 中身を先に決めて、見た目の色を合わせる
        //----------------------------------------------------
        chest.contents.clear();
        chest.displayRarity = Rarity::Common;
        chest.animTimer = 0.0f;

        for (int k = 0; k < 2; ++k)
        {
            const EquipmentDrop drop = m_dropSystem.ForceDrop(
                CurrentStage().type,
                DropSource::TreasureChest,
                fortune
            );

            if (!drop.dropped)
            {
                continue;
            }

            if (drop.GetRarity() > chest.displayRarity)
            {
                chest.displayRarity = drop.GetRarity();
            }

            chest.contents.push_back(drop);
        }
    }
}

//------------------------------------------------------------
// 敵の湧き
//------------------------------------------------------------
void GameScene::SpawnEnemyWave()
{
    const StageConfig& config = CurrentStage();
    const DifficultyModifier& modifier = GetDifficultyModifier(m_difficulty);

    //--------------------------------------------------------
    // 最終区画のボスステージだけはボスを出す
    //--------------------------------------------------------
    const bool bossArea =
        config.bossStage && (m_areaIndex == m_areaCount - 1);

    if (bossArea)
    {
        m_bossSpawned = true;
        m_spawner.MarkBossSpawned();

        m_enemies.push_back(
            MakeEnemy(
                EnemyKind::Boss,
                config,
                AreaLeft() + SCREEN_WIDTH * 0.72f,
                GroundY(),
                m_nextEnemyId++,
                modifier.enemyHp,
                modifier.enemyAttack,
                modifier.enemySpeed
            )
        );

        return;
    }

    //--------------------------------------------------------
    // 区画に出す敵の数
    // ステージと難易度で増える
    //--------------------------------------------------------
    int count = 4 + m_stageIndex;

    count = static_cast<int>(count * modifier.enemyCount + 0.5f);
    count = std::clamp(count, 3, m_spawner.GetMaxAlive());

    const std::vector<SpawnRequest> requests = m_spawner.SpawnWave(
        AreaLeft(),
        AreaRight(),
        count,
        m_playerX + m_playerWidth * 0.5f
    );

    for (const SpawnRequest& request : requests)
    {
        m_enemies.push_back(
            MakeEnemy(
                request.kind,
                config,
                request.worldX,
                GroundY(),
                m_nextEnemyId++,
                modifier.enemyHp,
                modifier.enemyAttack,
                modifier.enemySpeed
            )
        );
    }
}


//============================================================
// 武器
//============================================================

void GameScene::RefreshWeapons(bool refillAmmo)
{
    const int previousMag[2] =
    {
        m_ammoInMagazine[0],
        m_ammoInMagazine[1]
    };

    m_mainEffective = m_loadout.BuildEffectiveWeapon(true);
    m_subEffective = m_loadout.BuildEffectiveWeapon(false);

    const EffectiveWeapon* weapons[2] =
    {
        &m_mainEffective,
        &m_subEffective
    };

    for (int i = 0; i < 2; ++i)
    {
        if (!weapons[i]->valid)
        {
            m_ammoInMagazine[i] = 0;
            m_ammoReserve[i] = 0;
            continue;
        }

        if (refillAmmo)
        {
            m_ammoInMagazine[i] = weapons[i]->magazineSize;
            m_ammoReserve[i] = weapons[i]->reserveMax;
        }
        else
        {
            // マガジン容量が変わった場合の調整
            m_ammoInMagazine[i] =
                std::min(previousMag[i], weapons[i]->magazineSize);

            m_ammoReserve[i] =
                std::min(m_ammoReserve[i], weapons[i]->reserveMax);
        }
    }

    m_isReloading = false;
    m_reloadTimer = 0.0f;
    m_burstRemaining = 0;
}

void GameScene::StartReload()
{
    const int index = m_usingMain ? 0 : 1;

    const EffectiveWeapon& weapon =
        m_usingMain ? m_mainEffective : m_subEffective;

    if (!weapon.valid || m_isReloading)
    {
        return;
    }

    if (m_ammoInMagazine[index] >= weapon.magazineSize)
    {
        return;
    }

    if (index != SUB_WEAPON_INDEX && m_ammoReserve[index] <= 0)
    {
        return;
    }

    m_isReloading = true;
    m_reloadTimer = weapon.reloadTime;
    m_burstRemaining = 0;
}

//------------------------------------------------------------
// 弾を1発撃つ
//------------------------------------------------------------
void GameScene::SpawnBullet(
    float angleRadian,
    const EffectiveWeapon& weapon,
    float damageScale
)
{
    const float spreadRadian =
        weapon.spreadDegree * 3.14159265f / 180.0f;

    const float offset =
        RandFloat(-spreadRadian * 0.5f, spreadRadian * 0.5f);

    const float finalAngle = angleRadian + offset;

    Bullet bullet;

    const float muzzleX =
        m_playerX + m_playerWidth * 0.5f + std::cos(finalAngle) * 46.0f;

    const float muzzleY =
        m_playerY + m_playerHeight * 0.42f + std::sin(finalAngle) * 46.0f;

    bullet.x = muzzleX;
    bullet.y = muzzleY;
    bullet.vx = std::cos(finalAngle) * weapon.bulletSpeed;
    bullet.vy = std::sin(finalAngle) * weapon.bulletSpeed;

    bullet.range = weapon.bulletRange;
    bullet.radius = (weapon.pelletCount > 1) ? 4.0f : 6.0f;
    bullet.fromPlayer = true;

    bullet.baseDamage = weapon.attack * damageScale;

    bullet.critRate = weapon.critRate;
    bullet.critDamage = weapon.critDamage;

    bullet.headShotMultiplier = weapon.headShotMultiplier;
    bullet.headShotBonus = weapon.headShotDamageBonus;

    bullet.armorPenetration = weapon.armorPenetration;

    bullet.falloffStart = weapon.falloffStart;
    bullet.falloffEnd = weapon.falloffEnd;
    bullet.falloffMinScale = weapon.falloffMinScale;

    bullet.pierceLeft = weapon.pierceCount;
    bullet.knockBack = weapon.knockBack;

    // 同じ射撃で出た弾はすべて同じ番号になる
    bullet.shotId = m_nextShotId;

    m_bullets.push_back(std::move(bullet));
}

//------------------------------------------------------------
// 射撃
//------------------------------------------------------------
void GameScene::FireCurrentWeapon()
{
    const int index = m_usingMain ? 0 : 1;

    const EffectiveWeapon& weapon =
        m_usingMain ? m_mainEffective : m_subEffective;

    if (!weapon.valid || m_isReloading)
    {
        return;
    }

    if (m_ammoInMagazine[index] <= 0)
    {
        StartReload();
        return;
    }

    //--------------------------------------------------------
    // 3発目のDamage +40%（Burst Rifleの基本効果）
    //--------------------------------------------------------
    float damageScale = 1.0f;

    if (weapon.fireMode == FireMode::Burst)
    {
        const int shotIndex = 3 - m_burstRemaining;

        if (shotIndex == 3)
        {
            damageScale = 1.40f;
        }
    }

    //--------------------------------------------------------
    // MYTHIC Perfect Chamber：8発目のDamage +100%
    //--------------------------------------------------------
    MythicRuntime& mythic = m_loadout.GetMythic();

    damageScale *= (1.0f + GetSkillBuff().attackBonus);

    const float chamberScale = mythic.OnShotFired();

    if (chamberScale > 1.0f)
    {
        damageScale *= chamberScale;

        m_toastText = L"ミシック : 完全装填";
        m_toastTimer = 0.8f;
    }

    //--------------------------------------------------------
    // 散弾はペレット数だけ弾を出す
    //--------------------------------------------------------
    const int pellets = std::max(1, weapon.pelletCount);

    //--------------------------------------------------------
    // ここから出る弾はすべて「同じ1回の射撃」として扱う。
    // 散弾が同時に頭へ当たっても、
    // ヘッドショットは1回しか数えない。
    //--------------------------------------------------------
    ++m_nextShotId;

    for (int i = 0; i < pellets; ++i)
    {
        SpawnBullet(m_aimAngle, weapon, damageScale);
    }

    // 散弾でも発砲音は1回だけ
    Audio::PlaySe(SeId::Shot);

    //--------------------------------------------------------
    // 弾薬の消費
    //   突撃兵の 制圧射撃：発動中は消費しない
    //   ミシック 無限弾倉：リロード直後3秒間は消費しない
    //   銃手 6：20%の確率で消費しない
    //--------------------------------------------------------
    if (GetSkillBuff().infiniteAmmo || mythic.IsAmmoFree())
    {
        // 消費しない
    }
    else if (m_loadout.GetSynergy().ShouldConsumeAmmo())
    {
        --m_ammoInMagazine[index];
    }
    else
    {
        m_toastText = L"銃手 : 弾薬を消費しなかった";
        m_toastTimer = 0.9f;
    }

    // 散弾は出た弾の数だけ数える。1発と数えると命中率が跳ね上がる
    m_shotsFired += pellets;

    //--------------------------------------------------------
    // 銃口の閃光（設計書 19-5.3）
    //--------------------------------------------------------
    m_effectSystem.Spawn(
        EffectType::Muzzle,
        m_playerX + m_playerWidth * 0.5f + std::cos(m_aimAngle) * 44.0f,
        m_playerY + m_playerHeight * 0.42f + std::sin(m_aimAngle) * 44.0f,
        1.0f,
        m_aimAngle * 180.0f / 3.14159265f
    );

    m_shootAnimTimer = SHOOT_ANIM_TIME;
    m_recoilOffset = weapon.recoilKick * 2.4f;

    if (m_ammoInMagazine[index] <= 0)
    {
        StartReload();
    }
}

//============================================================
// 更新
//============================================================

//------------------------------------------------------------
// 設定画面から戻ってきたとき
//
//   ・時計を今に合わせ直す。そうしないと設定画面にいた分の
//     時間がまとめて進んでしまう。
//   ・押しっぱなしのキーを「押した瞬間」と誤解しないよう、
//     入力の前フレームを今の状態でそろえる。
//     これが無いと、決定キーを押したまま戻った瞬間に
//     もう一度「設定」が選ばれてしまう。
//------------------------------------------------------------
void GameScene::ResumeFromOption()
{
    m_lastUpdate = std::chrono::steady_clock::now();
    m_input.Sync();
}

void GameScene::UpdateMouse(HWND hwnd)
{
    //--------------------------------------------------------
    // 操作にはマウスを使わないが、
    // シナジーやステータスの説明を出すために位置だけ拾う。
    //--------------------------------------------------------
    POINT cursor{};

    if (GetCursorPos(&cursor) && ScreenToClient(hwnd, &cursor))
    {
        RECT client{};
        GetClientRect(hwnd, &client);

        const float clientWidth =
            static_cast<float>(client.right - client.left);

        const float clientHeight =
            static_cast<float>(client.bottom - client.top);

        if (clientWidth > 0.0f && clientHeight > 0.0f)
        {
            m_uiMouseX =
                (static_cast<float>(cursor.x) / clientWidth) * SCREEN_WIDTH;

            m_uiMouseY =
                (static_cast<float>(cursor.y) / clientHeight) * SCREEN_HEIGHT;
        }
    }
}

//------------------------------------------------------------
// 向きと照準
//
// マウスは使わない。
// 向きは A / D、上下は ↑ / ↓ で決める。
//
// ここはゲームが動いているときだけ呼ぶこと。
// ポーズ中や装備一覧を開いている間に呼ぶと、
// メニューを W / S で選んだだけで照準が上下に振れてしまう。
//------------------------------------------------------------
void GameScene::UpdateAim()
{
    const bool left = m_input.IsDown('A') || m_input.IsDown(VK_LEFT);
    const bool right = m_input.IsDown('D') || m_input.IsDown(VK_RIGHT);

    if (left && !right)
    {
        m_facingRight = false;
    }
    else if (right && !left)
    {
        m_facingRight = true;
    }

    //--------------------------------------------------------
    // 上下へは45度ずつ傾ける
    //--------------------------------------------------------
    float elevation = 0.0f;

    if (m_input.IsDown(VK_UP) || m_input.IsDown('W'))
    {
        elevation = -0.62f;
    }
    else if (m_input.IsDown(VK_DOWN) || m_input.IsDown('S'))
    {
        elevation = 0.62f;
    }

    m_aimAngle = m_facingRight ? elevation : (3.14159265f - elevation);

    //--------------------------------------------------------
    // 照準の見た目用に、狙っている先を求めておく
    //--------------------------------------------------------
    const float centerX = m_playerX + m_playerWidth * 0.5f;
    const float centerY = m_playerY + m_playerHeight * 0.42f;

    constexpr float AIM_LENGTH = 230.0f;

    m_mouseWorldX = centerX + std::cos(m_aimAngle) * AIM_LENGTH;
    m_mouseY = centerY + std::sin(m_aimAngle) * AIM_LENGTH;
    m_mouseX = ToScreenX(m_mouseWorldX);
}


//------------------------------------------------------------
// プレイヤー
//------------------------------------------------------------
void GameScene::UpdatePlayer(float deltaTime)
{
    const PlayerStats& stats = m_loadout.GetStats();

    const EffectiveWeapon& weapon =
        m_usingMain ? m_mainEffective : m_subEffective;

    //--------------------------------------------------------
    // 移動速度
    //  基礎 × (1 + Move Speed%) × 武器ごとの重量ペナルティ
    //--------------------------------------------------------
    const SkillBuff buff = GetSkillBuff();

    float moveSpeed =
        BASE_MOVE_SPEED *
        (1.0f + stats.moveSpeedPercent + buff.moveSpeedBonus);

    if (weapon.valid)
    {
        moveSpeed *= weapon.moveSpeedScale;
    }

    float moveInput = 0.0f;

    if (m_input.IsDown('A') || m_input.IsDown(VK_LEFT))
    {
        moveInput -= 1.0f;
    }

    if (m_input.IsDown('D') || m_input.IsDown(VK_RIGHT))
    {
        moveInput += 1.0f;
    }

    m_isMoving = (std::fabs(moveInput) > 0.001f);

    //--------------------------------------------------------
    // ダッシュ
    //--------------------------------------------------------
    if (m_dashCooldown > 0.0f)
    {
        m_dashCooldown -= deltaTime;
    }

    const bool dashKey =
        m_input.IsPressed(VK_LSHIFT) || m_input.IsPressed(VK_RSHIFT);

    if (dashKey && m_dashCooldown <= 0.0f && m_dashTimer <= 0.0f)
    {
        m_dashTimer = DASH_TIME;

        m_dashDirection =
            m_isMoving ? moveInput : (m_facingRight ? 1.0f : -1.0f);

        const float cooldownScale =
            std::max(1.0f - stats.dashCooldownReduction, 0.35f);

        m_dashCooldown = DASH_COOLDOWN * cooldownScale;

        Audio::PlaySe(SeId::Dash);
    }

    if (m_dashTimer > 0.0f)
    {
        m_dashTimer -= deltaTime;

        const float distanceScale = 1.0f + stats.dashDistancePercent;

        m_playerX += m_dashDirection * DASH_SPEED * distanceScale * deltaTime;
    }
    else
    {
        m_playerX += moveInput * moveSpeed * deltaTime;
    }

    //--------------------------------------------------------
    // すり抜け中の時間を進める
    //--------------------------------------------------------
    if (m_dropThroughTimer > 0.0f)
    {
        m_dropThroughTimer -= deltaTime;
    }

    //--------------------------------------------------------
    // ジャンプと重力
    //
    //   下（↓ または S）を押しながらジャンプすると、
    //   跳ばずに今乗っている段差をすり抜けて下へ降りる。
    //   地面に立っているときは降りる先が無いので普通に跳ぶ。
    //--------------------------------------------------------
    const bool holdDown =
        m_input.IsDown(VK_DOWN) || m_input.IsDown('S');

    // 地面ではなく、段差や障害物の上に立っているか
    const bool onPlatform =
        m_isGrounded &&
        (m_playerY + m_playerHeight < GroundY() - 1.0f);

    if (m_input.IsPressed(VK_SPACE) && m_isGrounded)
    {
        if (holdDown && onPlatform)
        {
            //------------------------------------------------
            // すり抜けて降りる。
            //
            // 少しだけ下へずらして落ち始めることで、
            // 同じ天板へ即座に着地し直すのを防ぐ。
            // 時間はごく短くてよい。足元が天板より6px下がれば
            // ResolveStanding が拾わなくなる。
            //------------------------------------------------
            m_dropThroughTimer = 0.18f;

            m_playerY += 8.0f;
            m_velocityY = 60.0f;
            m_isGrounded = false;
        }
        else
        {
            m_velocityY = -JUMP_POWER;
            m_isGrounded = false;

            // 踏み切りエフェクト。キャラごとの絵をシート3行目から出す
            m_jumpEffectTimer = 0.30f;
        }
    }

    if (!m_isGrounded)
    {
        m_velocityY += GRAVITY * deltaTime;
    }

    m_playerY += m_velocityY * deltaTime;

    if (m_playerY + m_playerHeight >= GroundY())
    {
        m_playerY = GroundY() - m_playerHeight;
        m_velocityY = 0.0f;
        m_isGrounded = true;
    }

    // 段差と障害物の上に乗る／外れたら落ちる
    ResolveStanding();

    m_previousBottom = m_playerY + m_playerHeight;

    //--------------------------------------------------------
    // 画面内へ収める
    //--------------------------------------------------------
    //--------------------------------------------------------
    // 区画の外へは出られない。
    // 制圧するまでは右端の少し手前で止まる。
    //--------------------------------------------------------
    //--------------------------------------------------------
    // 区画の端まで歩けるようにする。
    //
    // 区画を移った直後は、まだカメラが前の区画にいるので、
    // 左は1体ぶん余分に許しておく。
    // 実際に見えている範囲は、このあとカメラで挟む。
    //--------------------------------------------------------
    const float areaLeft = AreaLeft() - m_playerWidth;

    const float areaRight = AreaRight() - m_playerWidth;

    m_playerX = std::max(areaLeft, m_playerX);
    m_playerX = std::min(areaRight, m_playerX);

    //--------------------------------------------------------
    // 画面の外へは出さない。
    //
    // カメラは区画に固定しているので、区画の境目では
    // 「区画の中だが画面の外」という位置が作れてしまう。
    // カメラの写している範囲でもう一度挟む。
    //--------------------------------------------------------
    // 画面のふちまで歩けるようにするので、余白は取らない
    constexpr float VIEW_MARGIN = 0.0f;

    const float viewLeft = m_camera.GetX() + VIEW_MARGIN;

    const float viewRight =
        m_camera.GetX() + SCREEN_WIDTH - m_playerWidth - VIEW_MARGIN;

    if (viewRight > viewLeft)
    {
        m_playerX = std::clamp(m_playerX, viewLeft, viewRight);
    }

    //--------------------------------------------------------
    // 上へも出さない。
    // 高い段差から跳んだり、Blink で浮いたときに
    // 画面の上へ抜けてしまうため。
    //--------------------------------------------------------
    if (m_playerY < 0.0f)
    {
        m_playerY = 0.0f;

        if (m_velocityY < 0.0f)
        {
            m_velocityY = 0.0f;
        }
    }

    //--------------------------------------------------------
    // アニメーション
    //--------------------------------------------------------
    if (m_isMoving && m_isGrounded)
    {
        m_animTimer += deltaTime;

        while (m_animTimer >= WALK_FRAME_TIME)
        {
            m_animTimer -= WALK_FRAME_TIME;
            m_animFrame = (m_animFrame + 1) % WALK_FRAMES;
        }
    }
    else
    {
        m_animTimer = 0.0f;
        m_animFrame = 0;
    }

    if (m_shootAnimTimer > 0.0f)
    {
        m_shootAnimTimer = std::max(0.0f, m_shootAnimTimer - deltaTime);
    }

    if (m_jumpEffectTimer > 0.0f)
    {
        m_jumpEffectTimer = std::max(0.0f, m_jumpEffectTimer - deltaTime);
    }

    if (m_recoilOffset > 0.0f)
    {
        m_recoilOffset = std::max(0.0f, m_recoilOffset - deltaTime * 40.0f);
    }

    if (m_invincibleTimer > 0.0f)
    {
        m_invincibleTimer -= deltaTime;
    }

    if (m_damageFlash > 0.0f)
    {
        m_damageFlash = std::max(0.0f, m_damageFlash - deltaTime * 3.0f);
    }
}

//------------------------------------------------------------
// 段差・障害物の上に乗る
//
// 落下中に足元が天板を跨いだときだけ着地させる。
// 下から突き上げたときは通り抜ける。
//------------------------------------------------------------
void GameScene::ResolveStanding()
{
    const float bottom = m_playerY + m_playerHeight;

    //--------------------------------------------------------
    // 足元に何もなくなったら落下を始める。
    // これが無いと足場から歩いて外れても空中に留まってしまう。
    //--------------------------------------------------------
    if (m_isGrounded && m_velocityY >= 0.0f && m_dropThroughTimer <= 0.0f)
    {
        bool supported = (bottom >= GroundY() - 1.0f);

        const auto overlaps = [&](float left, float width, float topY)
            {
                if (m_playerX + m_playerWidth < left + 4.0f) return false;
                if (m_playerX > left + width - 4.0f) return false;

                return std::fabs(bottom - topY) <= 2.0f;
            };

        if (!supported && m_map)
        {
            for (const MapPlatform& platform : m_map->GetPlatforms())
            {
                if (overlaps(platform.worldX, platform.width, platform.topY))
                {
                    supported = true;
                    break;
                }
            }
        }

        if (!supported)
        {
            for (const ObstacleState& obstacle : m_obstacles)
            {
                if (!obstacle.alive)
                {
                    continue;
                }

                if (overlaps(obstacle.data.worldX, obstacle.data.width,
                             obstacle.TopY()))
                {
                    supported = true;
                    break;
                }
            }
        }

        if (!supported)
        {
            m_isGrounded = false;
        }
    }

    if (m_velocityY < 0.0f)
    {
        return;
    }

    //--------------------------------------------------------
    // 下＋ジャンプで降りている最中は、段差にも障害物にも
    // 乗らない。地面（GroundY）だけは UpdatePlayer 側で
    // 拾っているので、落ちきってしまうことはない。
    //--------------------------------------------------------
    if (m_dropThroughTimer > 0.0f)
    {
        return;
    }

    const auto tryLand = [&](float topY, float left, float width)
        {
            if (m_playerX + m_playerWidth < left ||
                m_playerX > left + width)
            {
                return;
            }

            // 前フレームは天板より上にいたか
            if (m_previousBottom > topY + 6.0f)
            {
                return;
            }

            if (bottom < topY)
            {
                return;
            }

            m_playerY = topY - m_playerHeight;
            m_velocityY = 0.0f;
            m_isGrounded = true;
        };

    if (m_map)
    {
        for (const MapPlatform& platform : m_map->GetPlatforms())
        {
            tryLand(platform.topY, platform.worldX, platform.width);
        }
    }

    for (const ObstacleState& obstacle : m_obstacles)
    {
        if (!obstacle.alive)
        {
            continue;
        }

        tryLand(obstacle.TopY(), obstacle.data.worldX, obstacle.data.width);
    }
}

//------------------------------------------------------------
// 弾が障害物に当たったか
//
// 当たった場合、プレイヤーの弾なら耐久値を削る。
// 敵の弾は遮蔽物として止まるだけ。
//------------------------------------------------------------
bool GameScene::HitObstacle(Bullet& bullet)
{
    //--------------------------------------------------------
    // 今いる区画の障害物だけを相手にする。
    // 隣の区画に置かれた障害物は、境目ごしに撃てない。
    //--------------------------------------------------------
    const float areaLeft = AreaLeft();
    const float areaRight = AreaRight();

    for (ObstacleState& obstacle : m_obstacles)
    {
        if (!obstacle.alive)
        {
            continue;
        }

        if (obstacle.data.worldX + obstacle.data.width <= areaLeft ||
            obstacle.data.worldX >= areaRight)
        {
            continue;
        }

        if (!CircleHitsRect(
            bullet.x, bullet.y, bullet.radius,
            obstacle.data.worldX, obstacle.TopY(),
            obstacle.data.width, obstacle.data.height))
        {
            continue;
        }

        if (bullet.fromPlayer)
        {
            obstacle.hp -= bullet.baseDamage;
            obstacle.hitFlash = 0.10f;

            m_effectSystem.Spawn(
                EffectType::Impact,
                bullet.x,
                bullet.y,
                0.85f,
                std::atan2(bullet.vy, bullet.vx) * 180.0f / 3.14159265f
            );

            if (obstacle.hp <= 0.0f)
            {
                obstacle.alive = false;

                Audio::PlaySe(SeId::Break);

                m_effectSystem.Spawn(
                    EffectType::Explosion,
                    obstacle.data.worldX + obstacle.data.width * 0.5f,
                    obstacle.TopY() + obstacle.data.height * 0.5f,
                    0.9f
                );
            }
        }

        return true;
    }

    return false;
}


//------------------------------------------------------------
// 射撃入力
//------------------------------------------------------------
void GameScene::UpdateWeapon(float deltaTime)
{
    const int index = m_usingMain ? 0 : 1;

    const EffectiveWeapon& weapon =
        m_usingMain ? m_mainEffective : m_subEffective;

    //--------------------------------------------------------
    // リロード
    //--------------------------------------------------------
    if (m_isReloading)
    {
        m_reloadTimer -= deltaTime;

        if (m_reloadTimer <= 0.0f)
        {
            m_isReloading = false;

            if (weapon.valid)
            {
                const int need =
                    weapon.magazineSize - m_ammoInMagazine[index];

                //--------------------------------------------
                // サブ武器は予備弾が無限。
                // メインが弾切れになっても詰まないようにするため。
                //--------------------------------------------
                if (index == SUB_WEAPON_INDEX)
                {
                    m_ammoInMagazine[index] += need;
                }
                else
                {
                    const int load = std::min(need, m_ammoReserve[index]);

                    m_ammoInMagazine[index] += load;
                    m_ammoReserve[index] -= load;
                }

                // MYTHIC Endless Magazine の効果時間を開始する
                m_loadout.GetMythic().OnReloadFinished();
            }
        }
    }

    if (m_input.IsPressed('R'))
    {
        StartReload();
    }

    //--------------------------------------------------------
    // 武器切り替え
    //--------------------------------------------------------
    bool switchWeapon = false;

    if (m_input.IsPressed('Q'))
    {
        m_usingMain = !m_usingMain;
        switchWeapon = true;
    }

    if (m_input.IsPressed('1') && !m_usingMain)
    {
        m_usingMain = true;
        switchWeapon = true;
    }

    if (m_input.IsPressed('2') && m_usingMain)
    {
        m_usingMain = false;
        switchWeapon = true;
    }

    if (switchWeapon)
    {
        m_isReloading = false;
        m_reloadTimer = 0.0f;
        m_burstRemaining = 0;
        m_shotTimer = 0.12f;
        return;
    }

    if (!weapon.valid)
    {
        return;
    }

    //--------------------------------------------------------
    // 発射タイミング
    //--------------------------------------------------------
    if (m_shotTimer > 0.0f)
    {
        m_shotTimer -= deltaTime;
    }

    // バースト継続中
    if (m_burstRemaining > 0)
    {
        m_burstTimer -= deltaTime;

        if (m_burstTimer <= 0.0f)
        {
            FireCurrentWeapon();

            --m_burstRemaining;

            if (m_burstRemaining > 0)
            {
                m_burstTimer = weapon.shotInterval /
                    (1.0f + m_loadout.GetMythic().GetFireRateBonus());
            }
            else
            {
                m_shotTimer = BURST_GAP;
            }
        }

        return;
    }

    //--------------------------------------------------------
    // メインが完全に弾切れなら自動でサブへ持ち替える
    //--------------------------------------------------------
    if (m_usingMain &&
        m_ammoInMagazine[MAIN_WEAPON_INDEX] <= 0 &&
        m_ammoReserve[MAIN_WEAPON_INDEX] <= 0 &&
        m_subEffective.valid)
    {
        m_usingMain = false;
        m_isReloading = false;
        m_reloadTimer = 0.0f;
        m_shotTimer = 0.15f;

        m_toastText = L"メイン弾切れ： サブ武器へ";
        m_toastTimer = 1.4f;

        return;
    }

    // 射撃は J キー
    const bool triggerHeld = m_input.IsDown('J');
    const bool triggerPressed = m_input.IsPressed('J');

    bool wantFire = false;

    switch (weapon.fireMode)
    {
    case FireMode::FullAuto:
    case FireMode::Shotgun:
        wantFire = triggerHeld;
        break;

    case FireMode::SemiAuto:
    case FireMode::Burst:
        wantFire = triggerPressed;
        break;
    }

    if (!wantFire || m_shotTimer > 0.0f || m_isReloading)
    {
        return;
    }

    if (weapon.fireMode == FireMode::Burst)
    {
        m_burstRemaining = 3;
        m_burstTimer = 0.0f;
        return;
    }

    FireCurrentWeapon();

    // MYTHIC Overclock Core とスキルで連射が速くなる
    float fireRateBonus = m_loadout.GetMythic().GetFireRateBonus();

    fireRateBonus += GetSkillBuff().fireRateBonus;

    m_shotTimer = weapon.shotInterval / (1.0f + fireRateBonus);
}

//------------------------------------------------------------
// 弾
//------------------------------------------------------------
void GameScene::UpdateBullets(float deltaTime)
{
    //--------------------------------------------------------
    // このフレームでヘッドショットを数えた射撃を忘れる。
    // 別の射撃なら、同じフレームでもきちんと数える。
    //--------------------------------------------------------
    m_headShotShotId = 0;

    for (Bullet& bullet : m_bullets)
    {
        if (!bullet.alive)
        {
            continue;
        }

        const float dx = bullet.vx * deltaTime;
        const float dy = bullet.vy * deltaTime;

        bullet.x += dx;
        bullet.y += dy;

        bullet.traveled += std::sqrt(dx * dx + dy * dy);

        //----------------------------------------------------
        // 射程・区画の外
        //
        // 弾は今いる区画から出さない。
        // カメラは区画の中央に固定されているので、
        // 区画の境目はそのまま画面の左右端になる。
        // 隣の区画の敵や障害物へ届いてしまうのを防ぐ。
        //
        // 弾の縁で見るので、丸ごと境目の内側に収まる。
        //----------------------------------------------------
        const float areaLeft = AreaLeft();
        const float areaRight = AreaRight();

        if (bullet.traveled > bullet.range ||
            bullet.x - bullet.radius < areaLeft ||
            bullet.x + bullet.radius > areaRight ||
            bullet.y < -60.0f || bullet.y > SCREEN_HEIGHT + 60.0f)
        {
            bullet.alive = false;
            continue;
        }

        //----------------------------------------------------
        // 障害物は弾を止める
        //----------------------------------------------------
        if (HitObstacle(bullet))
        {
            bullet.alive = false;
            continue;
        }

        //----------------------------------------------------
        // 敵の弾 → プレイヤー
        //----------------------------------------------------
        if (!bullet.fromPlayer)
        {

            if (CircleHitsRect(
                bullet.x, bullet.y, bullet.radius,
                m_playerX, m_playerY, m_playerWidth, m_playerHeight))
            {
                DamagePlayer(bullet.baseDamage);
                bullet.alive = false;
            }

            continue;
        }

        //----------------------------------------------------
        // プレイヤーの弾 → 敵
        //----------------------------------------------------
        for (Enemy& enemy : m_enemies)
        {
            if (!enemy.alive)
            {
                continue;
            }

            //------------------------------------------------
            // 区画をまたいでは撃てない。
            // 敵は区画の中へ収められているので普段は通るが、
            // 境目ぎわに立たれたときの取りこぼしを防ぐ。
            //------------------------------------------------
            if (enemy.x + enemy.width <= areaLeft ||
                enemy.x >= areaRight)
            {
                continue;
            }

            // 貫通中に同じ敵を二度撃たない
            if (std::find(
                bullet.hitEnemyIds.begin(),
                bullet.hitEnemyIds.end(),
                enemy.id) != bullet.hitEnemyIds.end())
            {
                continue;
            }

            if (!CircleHitsRect(
                bullet.x, bullet.y, bullet.radius,
                enemy.x, enemy.y, enemy.width, enemy.height))
            {
                continue;
            }

            bullet.hitEnemyIds.push_back(enemy.id);

            //------------------------------------------------
            // ヘッドショット判定
            //
            // 散弾は1回の射撃で何発も同時に頭へ当たる。
            // そのたびに数えると、表示も撃破数も跳ね上がるので、
            // 同じ射撃のぶんは最初の1発だけをヘッドショットとして扱う。
            // 2発目以降は通常命中になる（倍率も乗らない）。
            //------------------------------------------------
            bool isHeadShot = (bullet.y <= enemy.GetHeadLineY());

            if (isHeadShot)
            {
                if (m_headShotShotId == bullet.shotId)
                {
                    isHeadShot = false;
                }
                else
                {
                    m_headShotShotId = bullet.shotId;
                }
            }

            //------------------------------------------------
            // ダメージ計算
            //------------------------------------------------
            SynergySystem& synergy = m_loadout.GetSynergy();

            float damage = bullet.baseDamage;

            // 距離減衰（ショットガン）
            if (bullet.falloffEnd > bullet.falloffStart)
            {
                const float t = std::clamp(
                    (bullet.traveled - bullet.falloffStart) /
                    (bullet.falloffEnd - bullet.falloffStart),
                    0.0f, 1.0f
                );

                damage *= 1.0f + (bullet.falloffMinScale - 1.0f) * t;
            }

            // HeadShot
            if (isHeadShot)
            {
                damage *=
                    bullet.headShotMultiplier * (1.0f + bullet.headShotBonus);
            }

            MythicRuntime& mythic = m_loadout.GetMythic();

            // MYTHIC Deadeye Protocol：HeadShot後2秒間 Crit Rate +30%
            if (isHeadShot)
            {
                mythic.OnHeadShot();
            }

            // Crit判定（Redeye 6 はHeadShotが必ずCrit）
            const SkillBuff skillBuff = GetSkillBuff();

            if (isHeadShot && skillBuff.headShotBonus > 0.0f)
            {
                damage *= (1.0f + skillBuff.headShotBonus);
            }

            const float effectiveCritRate = std::min(
                bullet.critRate + mythic.GetCritRateBonus()
                    + skillBuff.critRateBonus,
                1.0f
            );

            bool isCrit = RollChance(effectiveCritRate);

            if (isHeadShot && synergy.HeadShotForcesCrit())
            {
                isCrit = true;
            }

            if (isCrit)
            {
                float critDamage = bullet.critDamage;

                if (enemy.IsEliteOrBoss())
                {
                    critDamage += synergy.GetEliteCritDamageBonus();
                }

                damage *= (1.0f + critDamage);
            }

            // Executioner：瀕死の敵へのボーナス
            damage *= 1.0f + synergy.GetExecutionerBonus(
                enemy.GetHpRatio(),
                enemy.IsEliteOrBoss()
            );

            // Assault 6：Overdrive
            damage *= 1.0f + synergy.GetOverdriveAttackBonus();

            // MYTHIC Overclock Core：20回命中で発動
            mythic.OnWeaponHit();

            // Assault 4 / Demolition の命中カウンタ
            const SynergyHitResult hitResult = synergy.OnWeaponHit();

            damage *= hitResult.damageMultiplier;

            // 敵の装甲（Armor Penetrationで貫通）
            const float armor =
                enemy.armorReduction * (1.0f - bullet.armorPenetration);

            damage *= (1.0f - armor);

            //------------------------------------------------
            // 着弾エフェクト。
            //   HeadShot は少し大きく出す。
            //   火花を弾の来た向きへ散らすため、
            //   進行方向を角度（度）にして渡す。
            //------------------------------------------------
            m_effectSystem.Spawn(
                EffectType::Impact,
                bullet.x,
                bullet.y,
                isHeadShot ? 1.45f : 1.0f,
                std::atan2(bullet.vy, bullet.vx) * 180.0f / 3.14159265f
            );

            //------------------------------------------------
            // 押し戻し（ショットガン）
            //
            // 弾が飛んでいる向きへ少しだけ下がらせる。
            // 散弾は1発ずつ押すので、近距離で全弾当たるほど
            // 強く、離れて当たる数が減るほど弱くなる。
            //
            // ボスと重甲兵は踏ん張って動きにくい。
            //------------------------------------------------
            if (bullet.knockBack > 0.0f)
            {
                float resist = 1.0f;

                if (enemy.kind == EnemyKind::Boss)
                {
                    resist = 0.15f;
                }
                else if (enemy.kind == EnemyKind::Brute)
                {
                    resist = 0.45f;
                }
                else if (enemy.kind == EnemyKind::Elite)
                {
                    resist = 0.70f;
                }

                const float direction = (bullet.vx >= 0.0f) ? 1.0f : -1.0f;

                enemy.x += direction * bullet.knockBack * resist;

                // 区画の外へは押し出さない
                enemy.x = std::clamp(
                    enemy.x,
                    AreaLeft() + 8.0f,
                    AreaRight() - enemy.width - 8.0f
                );
            }

            ApplyDamageToEnemy(enemy, damage, isCrit, isHeadShot);

            //------------------------------------------------
            // Demolition：追加爆発
            //------------------------------------------------
            if (hitResult.triggerExplosion)
            {
                SpawnExplosion(
                    bullet.x,
                    bullet.y,
                    bullet.baseDamage * hitResult.explosionDamageRatio,
                    hitResult.explosionRadiusScale
                );
            }

            //------------------------------------------------
            // 貫通
            //------------------------------------------------
            if (bullet.pierceLeft > 0)
            {
                --bullet.pierceLeft;
            }
            else
            {
                bullet.alive = false;
                break;
            }
        }
    }

    m_bullets.erase(
        std::remove_if(
            m_bullets.begin(),
            m_bullets.end(),
            [](const Bullet& b) { return !b.alive; }
        ),
        m_bullets.end()
    );
}

//------------------------------------------------------------
// 敵へのダメージ適用
//------------------------------------------------------------
void GameScene::ApplyDamageToEnemy(
    Enemy& enemy,
    float damage,
    bool isCrit,
    bool isHeadShot
)
{
    if (!enemy.alive || damage <= 0.0f)
    {
        return;
    }

    SynergySystem& synergy = m_loadout.GetSynergy();

    enemy.hp -= damage;
    enemy.hitFlash = 0.10f;

    //--------------------------------------------------------
    // 撃った敵のHPを3秒だけ画面下へ出す
    //--------------------------------------------------------
    {
        bool found = false;

        for (EnemyHpTrack& track : m_enemyHpTracks)
        {
            if (track.id == enemy.id)
            {
                track.hp = std::max(0.0f, enemy.hp - damage);
                track.maxHp = enemy.maxHp;
                track.timer = 3.0f;
                found = true;
                break;
            }
        }

        if (!found)
        {
            EnemyHpTrack track;

            track.id = enemy.id;
            track.name = GetEnemyName(enemy.kind);
            track.hp = std::max(0.0f, enemy.hp - damage);
            track.maxHp = enemy.maxHp;
            track.elite = enemy.IsEliteOrBoss();
            track.timer = 3.0f;

            m_enemyHpTracks.push_back(track);

            // 古いものから消して5件までにする
            while (m_enemyHpTracks.size() > 5)
            {
                m_enemyHpTracks.erase(m_enemyHpTracks.begin());
            }
        }
    }

    m_damageDealt += damage;
    m_highestDamage = std::max(m_highestDamage, damage);

    ++m_shotsHit;

    if (isHeadShot)
    {
        m_score.AddHeadShot();

        Audio::PlaySe(SeId::HeadShot);
    }

    SpawnDamageText(
        enemy.CenterX() + RandFloat(-18.0f, 18.0f),
        enemy.y + enemy.height * 0.3f,
        damage,
        isCrit,
        isHeadShot
    );

    //--------------------------------------------------------
    // Vampire 6：与Damageの2%を吸収
    //--------------------------------------------------------
    float lifesteal =
        synergy.OnDamageDealt(damage, m_loadout.GetStats().maxHP);

    // Blood Pact など、吸収系のスキル
    lifesteal += damage * GetSkillBuff().lifesteal;

    if (lifesteal > 0.0f)
    {
        HealPlayer(lifesteal);
    }

    //--------------------------------------------------------
    // Executioner 6：通常敵はHP15%以下で即Execution
    //--------------------------------------------------------
    if (enemy.hp > 0.0f &&
        synergy.ShouldExecute(enemy.GetHpRatio(), enemy.IsEliteOrBoss()))
    {
        enemy.hp = 0.0f;

        m_toastText = L"処刑 : 断罪";
        m_toastTimer = 0.9f;
    }

    if (enemy.hp <= 0.0f)
    {
        enemy.hp = 0.0f;
        enemy.alive = false;

        OnEnemyKilled(enemy);
    }
}

//------------------------------------------------------------
// 撃破時
//------------------------------------------------------------
void GameScene::OnEnemyKilled(const Enemy& enemy)
{
    ++m_killCount;
    ++m_totalKills;

    // 撃破エフェクト（設計書 19-5.7）
    m_effectSystem.Spawn(
        EffectType::Death,
        enemy.CenterX(),
        enemy.CenterY(),
        enemy.IsEliteOrBoss() ? 1.8f : 1.0f
    );

    if (enemy.kind == EnemyKind::Elite)
    {
        ++m_eliteKillCount;
    }

    m_score.AddKill(
        enemy.scoreValue,
        enemy.kind == EnemyKind::Elite,
        enemy.kind == EnemyKind::Boss
    );

    SynergySystem& synergy = m_loadout.GetSynergy();

    //--------------------------------------------------------
    // Vampire：撃破時回復
    //--------------------------------------------------------
    const float heal = synergy.OnKill(
        enemy.IsEliteOrBoss(),
        m_loadout.GetStats().maxHP
    );

    if (heal > 0.0f)
    {
        HealPlayer(heal);
    }

    MythicRuntime& mythic = m_loadout.GetMythic();

    //--------------------------------------------------------
    // MYTHIC Blood Bombardment
    // 敵撃破時20%で武器Damage150%の爆発
    //--------------------------------------------------------
    if (mythic.RollBloodBombardment())
    {
        const EffectiveWeapon& current =
            m_usingMain ? m_mainEffective : m_subEffective;

        if (current.valid)
        {
            SpawnExplosion(
                enemy.CenterX(),
                enemy.CenterY(),
                current.attack * current.pelletCount * 1.5f,
                1.3f
            );

            m_toastText = L"ミシック : 血の砲撃";
            m_toastTimer = 1.0f;
        }
    }

    //--------------------------------------------------------
    // MYTHIC Ammo Fabricator：敵撃破時10%で弾薬を補充
    //--------------------------------------------------------
    if (mythic.RollAmmoFabricator())
    {
        RefillReserve(0.25f);

        m_toastText = L"ミシック : 弾薬生成";
        m_toastTimer = 1.0f;
    }

    //--------------------------------------------------------
    // 弾薬ドロップ
    // Elite / Boss は確定、通常敵は40%（Ammo Dropで上昇）
    //--------------------------------------------------------
    const float ammoChance =
        (enemy.IsEliteOrBoss())
        ? 1.0f
        : 0.55f * (1.0f + m_loadout.GetStats().ammoDropPercent);

    if (RollChance(ammoChance))
    {
        SpawnAmmoPickup(enemy.CenterX(), GroundY() - 34.0f);
    }

    //--------------------------------------------------------
    // HP回復ドロップ
    //
    // 基本は 12%、Elite / Boss は 40%。
    // さらに、HPが減っているほど出やすくする。
    //   満タン       … そのまま
    //   HP20%以下    … +18%
    // 追い詰められたときほど拾える形にして、
    // 立て直しの目が残るようにしている。
    //--------------------------------------------------------
    {
        const PlayerStats& stats = m_loadout.GetStats();

        const float hpRatio =
            std::clamp(m_hp / std::max(stats.maxHP, 1.0f), 0.0f, 1.0f);

        float healthChance = enemy.IsEliteOrBoss() ? 0.40f : 0.12f;

        healthChance += (1.0f - hpRatio) * 0.18f;

        // 満タンのときは落とさない。拾えないものが散らかるため。
        if (hpRatio < 0.995f && RollChance(healthChance))
        {
            // 最大HPの18%を回復する
            SpawnHealthPickup(
                enemy.CenterX(),
                GroundY() - 34.0f,
                stats.maxHP * 0.18f
            );
        }
    }

    //--------------------------------------------------------
    // ドロップ判定
    //--------------------------------------------------------
    DropSource source = DropSource::NormalEnemy;

    if (enemy.kind == EnemyKind::Boss)
    {
        source = DropSource::Boss;
    }
    else if (enemy.kind == EnemyKind::Elite)
    {
        source = DropSource::EliteEnemy;
    }

    const int fortune = synergy.GetFortuneLevel();

    if (enemy.kind == EnemyKind::Boss)
    {
        // ボスは必ず3個落とす
        for (int i = 0; i < 3; ++i)
        {
            SpawnDropAt(
                enemy.CenterX() + (i - 1) * 90.0f,
                GroundY() - 40.0f,
                m_dropSystem.ForceDrop(
                    CurrentStage().type,
                    DropSource::Boss,
                    fortune
                ),
                false
            );
        }

        return;
    }

    // 敵からのドロップは寄せない
    m_dropSystem.SetObjectiveSynergies({}, 0);

    std::vector<EquipmentDrop> drops = m_dropSystem.TryDrop(
        CurrentStage().type,
        source,
        fortune
    );

    //--------------------------------------------------------
    // MYTHIC Fortune Armor：Elite撃破時15%で追加装備
    //--------------------------------------------------------
    if (enemy.kind == EnemyKind::Elite && mythic.RollFortuneArmor())
    {
        drops.push_back(
            m_dropSystem.ForceDrop(
                CurrentStage().type,
                DropSource::EliteEnemy,
                fortune
            )
        );

        m_toastText = L"ミシック : 幸運の護り";
        m_toastTimer = 1.2f;
    }

    for (size_t i = 0; i < drops.size(); ++i)
    {
        if (!drops[i].dropped)
        {
            continue;
        }

        SpawnDropAt(
            enemy.CenterX() + static_cast<float>(i) * 52.0f,
            GroundY() - 40.0f,
            drops[i],
            false
        );
    }
}

//------------------------------------------------------------
// 敵の行動
//------------------------------------------------------------
void GameScene::UpdateEnemies(float deltaTime)
{
    const float playerCenterX = m_playerX + m_playerWidth * 0.5f;

    for (Enemy& enemy : m_enemies)
    {
        if (!enemy.alive)
        {
            continue;
        }

        if (enemy.hitFlash > 0.0f)
        {
            enemy.hitFlash -= deltaTime;
        }

        // 歩行アニメーション用
        enemy.actionTimer += deltaTime;

        if (enemy.attackCooldown > 0.0f)
        {
            enemy.attackCooldown -= deltaTime;
        }

        //----------------------------------------------------
        // プレイヤーへ寄る
        //----------------------------------------------------
        const float direction =
            (playerCenterX > enemy.CenterX()) ? 1.0f : -1.0f;

        //----------------------------------------------------
        // 減速中は遅くなる
        //----------------------------------------------------
        if (enemy.slowTimer > 0.0f)
        {
            enemy.slowTimer -= deltaTime;

            if (enemy.slowTimer <= 0.0f)
            {
                enemy.slowFactor = 1.0f;
            }
        }

        float speed = enemy.speed * enemy.slowFactor;

        // ボスと、距離を取る型だけは近づきすぎない
        if (enemy.kind == EnemyKind::Boss || enemy.keepDistance)
        {
            const float keep =
                (enemy.kind == EnemyKind::Boss) ? 320.0f : 300.0f;

            const float distance =
                std::fabs(playerCenterX - enemy.CenterX());

            if (distance < keep)
            {
                speed = -enemy.speed * 0.6f;
            }
        }

        enemy.x += direction * speed * deltaTime;

        enemy.x = std::max(AreaLeft() + 8.0f, enemy.x);
        enemy.x = std::min(AreaRight() - enemy.width - 8.0f, enemy.x);

        //----------------------------------------------------
        // 接触ダメージ
        //----------------------------------------------------
        if (enemy.attackCooldown <= 0.0f &&
            RectsOverlap(
                enemy.x, enemy.y, enemy.width, enemy.height,
                m_playerX, m_playerY, m_playerWidth, m_playerHeight))
        {
            DamagePlayer(enemy.contactDamage);
            enemy.attackCooldown = 0.85f;

            Audio::PlaySe(SeId::EnemyHit);
        }

        //----------------------------------------------------
        // 敵の射撃
        //
        // 強い敵ほど短い間隔で、狙いも正確に撃つ。
        // 弱い敵は間隔が長く、狙いも大きくぶれる。
        //----------------------------------------------------
        if (enemy.shootInterval > 0.0f)
        {
            enemy.shootTimer -= deltaTime;

            const float dx = playerCenterX - enemy.CenterX();
            const float dy =
                (m_playerY + m_playerHeight * 0.5f) - enemy.CenterY();

            const float distance = std::sqrt(dx * dx + dy * dy);

            //------------------------------------------------
            // 近すぎるときは殴りにいくので撃たない。
            // 画面に映っていない敵にも撃たせない。
            //------------------------------------------------
            const bool tooClose = (distance < 130.0f);

            const bool onScreen =
                m_camera.IsVisible(enemy.x, enemy.width);

            //------------------------------------------------
            // 画面が弾だらけにならないよう上限を設ける
            //------------------------------------------------
            int enemyBullets = 0;

            for (const Bullet& b : m_bullets)
            {
                if (!b.fromPlayer)
                {
                    ++enemyBullets;
                }
            }

            const bool tooMany = (enemyBullets >= 7);

            if (enemy.shootTimer <= 0.0f &&
                distance <= enemy.shootRange &&
                !tooClose && onScreen && !tooMany)
            {
                enemy.shootTimer = enemy.shootInterval;

                //--------------------------------------------
                // 狙いをぶらす。弱い敵ほど大きくぶれる
                //--------------------------------------------
                const float spread =
                    RandFloat(-enemy.shootSpread, enemy.shootSpread);

                const float angle = std::atan2(dy, dx) + spread;

                Bullet bullet;

                bullet.x = enemy.CenterX();
                bullet.y = enemy.CenterY();
                bullet.vx = std::cos(angle) * enemy.bulletSpeed;
                bullet.vy = std::sin(angle) * enemy.bulletSpeed;
                bullet.radius = 8.0f;
                bullet.range = enemy.shootRange + 160.0f;
                bullet.fromPlayer = false;
                bullet.baseDamage = enemy.bulletDamage;

                m_bullets.push_back(std::move(bullet));
            }
        }

        //----------------------------------------------------
        // ボスの射撃
        //----------------------------------------------------
        if (enemy.kind == EnemyKind::Boss)
        {
            enemy.shootTimer -= deltaTime;

            // HPが半分を切ったら攻撃が激しくなる
            const bool enraged = (enemy.GetHpRatio() < 0.5f);

            if (enemy.shootTimer <= 0.0f)
            {
                enemy.shootTimer = enraged ? 1.15f : 1.85f;

                const float baseAngle = std::atan2(
                    (m_playerY + m_playerHeight * 0.5f) - enemy.CenterY(),
                    playerCenterX - enemy.CenterX()
                );

                const int shots = enraged ? 5 : 3;

                for (int i = 0; i < shots; ++i)
                {
                    const float spread =
                        (static_cast<float>(i) - (shots - 1) * 0.5f) * 0.16f;

                    Bullet bullet;

                    bullet.x = enemy.CenterX();
                    bullet.y = enemy.CenterY();
                    bullet.vx = std::cos(baseAngle + spread) * 520.0f;
                    bullet.vy = std::sin(baseAngle + spread) * 520.0f;
                    bullet.radius = 11.0f;
                    bullet.range = 1800.0f;
                    bullet.fromPlayer = false;
                    bullet.baseDamage = enemy.contactDamage * 0.55f;

                    m_bullets.push_back(std::move(bullet));
                }
            }
        }
    }

    m_enemies.erase(
        std::remove_if(
            m_enemies.begin(),
            m_enemies.end(),
            [](const Enemy& e) { return !e.alive; }
        ),
        m_enemies.end()
    );
}

//------------------------------------------------------------
// 爆発（Demolition）
//------------------------------------------------------------
void GameScene::SpawnExplosion(
    float x,
    float y,
    float damage,
    float radiusScale
)
{
    Explosion explosion;

    explosion.x = x;
    explosion.y = y;
    explosion.radius = 95.0f * radiusScale;
    explosion.damage = damage;
    explosion.timer = 0.32f;

    m_explosions.push_back(explosion);

    m_effectSystem.Spawn(
        EffectType::Explosion,
        x,
        y,
        radiusScale
    );
}

void GameScene::UpdateExplosions(float deltaTime)
{
    for (Explosion& explosion : m_explosions)
    {
        //----------------------------------------------------
        // ダメージは発生した瞬間に1回だけ
        //----------------------------------------------------
        if (!explosion.damageApplied)
        {
            explosion.damageApplied = true;

            for (Enemy& enemy : m_enemies)
            {
                if (!enemy.alive)
                {
                    continue;
                }

                const float dx = enemy.CenterX() - explosion.x;
                const float dy = enemy.CenterY() - explosion.y;

                if (dx * dx + dy * dy <=
                    explosion.radius * explosion.radius)
                {
                    ApplyDamageToEnemy(
                        enemy,
                        explosion.damage,
                        false,
                        false
                    );
                }
            }
        }

        explosion.timer -= deltaTime;
    }

    m_explosions.erase(
        std::remove_if(
            m_explosions.begin(),
            m_explosions.end(),
            [](const Explosion& e) { return e.timer <= 0.0f; }
        ),
        m_explosions.end()
    );
}

//------------------------------------------------------------
// プレイヤーの被弾・回復
//------------------------------------------------------------
void GameScene::DamagePlayer(float rawDamage)
{
    if (m_state != GameState::Playing)
    {
        return;
    }

    // ダッシュ中・被弾直後・Bullet Screen 中は無敵
    if (m_invincibleTimer > 0.0f || m_dashTimer > 0.0f)
    {
        return;
    }

    if (GetSkillBuff().invincible)
    {
        return;
    }

    const PlayerStats& stats = m_loadout.GetStats();

    //--------------------------------------------------------
    // Velocity：回避
    //--------------------------------------------------------
    if (RollChance(stats.dodge))
    {
        m_loadout.GetSynergy().OnDodgeSuccess();

        m_toastText = L"回避";
        m_toastTimer = 0.7f;

        // MYTHIC Combat Flow：回避成功でDash Cooldownを30%回復
        m_dashCooldown *=
            m_loadout.GetMythic().GetDashCooldownScaleOnDodge();

        m_invincibleTimer = 0.20f;
        return;
    }

    //--------------------------------------------------------
    // Defense と Damage Reduction
    //--------------------------------------------------------
    // MYTHIC Last Stand：HP30%以下で Defense +35%
    const float hpRatio = m_hp / std::max(stats.maxHP, 1.0f);

    const float defense =
        stats.defense * m_loadout.GetMythic().GetDefenseMultiplier(hpRatio);

    const float mitigation = defense / (defense + 400.0f);

    float damage =
        rawDamage * (1.0f - mitigation) * (1.0f - stats.damageReduction);

    if (damage < 1.0f)
    {
        damage = 1.0f;
    }

    //--------------------------------------------------------
    // Shieldから先に削る
    //--------------------------------------------------------
    if (m_shield > 0.0f)
    {
        const float absorbed = std::min(m_shield, damage);

        m_shield -= absorbed;
        damage -= absorbed;
    }

    m_hp -= damage;

    m_damageTaken += damage;
    m_tookDamageThisStage = true;

    m_invincibleTimer = 0.38f;
    m_damageFlash = 1.0f;

    if (m_hp <= 0.0f)
    {
        m_hp = 0.0f;

        m_state = GameState::GameOver;
        m_stateTimer = 0.0f;

        BuildResultData(false);
    }
}

void GameScene::HealPlayer(float amount)
{
    if (amount <= 0.0f)
    {
        return;
    }

    m_hp = std::min(m_hp + amount, m_loadout.GetStats().maxHP);
}

//------------------------------------------------------------
// ドロップ
//------------------------------------------------------------
void GameScene::SpawnDropAt(
    float x,
    float y,
    const EquipmentDrop& item,
    bool fromChest
)
{
    if (!item.dropped)
    {
        return;
    }

    WorldDrop drop;

    drop.x = std::clamp(x, LeftLimit() + 30.0f, RightLimit() - 30.0f);
    drop.y = y;
    drop.bobTimer = RandFloat(0.0f, 6.28f);
    drop.fromChest = fromChest;
    drop.item = item;

    m_drops.push_back(std::move(drop));
}

//------------------------------------------------------------
// 弾薬ピックアップ
//
// 予備弾が尽きると撃てなくなって詰むため、
// 敵からは一定確率で弾薬が落ちる。
// LEG防具の「Ammo Drop」OPで出現率が上がる。
//------------------------------------------------------------
void GameScene::SpawnAmmoPickup(float x, float y)
{
    AmmoPickup pickup;

    pickup.x = std::clamp(x, LeftLimit() + 30.0f, RightLimit() - 30.0f);
    pickup.y = y;
    pickup.bobTimer = RandFloat(0.0f, 6.28f);

    m_ammoPickups.push_back(pickup);
}

//------------------------------------------------------------
// HP回復アイテムを1つ落とす
//------------------------------------------------------------
void GameScene::SpawnHealthPickup(float x, float y, float amount)
{
    HealthPickup pickup;

    pickup.x = std::clamp(x, LeftLimit() + 30.0f, RightLimit() - 30.0f);
    pickup.y = y;
    pickup.bobTimer = RandFloat(0.0f, 6.28f);
    pickup.amount = amount;

    m_healthPickups.push_back(pickup);
}

//------------------------------------------------------------
// 弾倉を満タンにする
//
//   予備弾は減らさない。効果中は弾薬を消費しないので、
//   ここで詰めた弾はそのまま効果が切れたあとも残る。
//
//   メインとサブの両方を満タンにしている。
//   効果中に持ち替えた先が空だと、そこで撃てなくなり
//   「弾薬無制限」が途切れてしまうため。
//------------------------------------------------------------
void GameScene::RefillMagazine()
{
    const EffectiveWeapon* weapons[2] =
    {
        &m_mainEffective,
        &m_subEffective
    };

    for (int i = 0; i < 2; ++i)
    {
        if (!weapons[i]->valid)
        {
            continue;
        }

        m_ammoInMagazine[i] = weapons[i]->magazineSize;
    }

    //--------------------------------------------------------
    // 途中だったリロードは打ち切る。
    // すでに満タンなので待たせる理由がない。
    //--------------------------------------------------------
    m_isReloading = false;
    m_reloadTimer = 0.0f;
}

void GameScene::RefillReserve(float ratio)
{
    const EffectiveWeapon* weapons[2] =
    {
        &m_mainEffective,
        &m_subEffective
    };

    for (int i = 0; i < 2; ++i)
    {
        if (!weapons[i]->valid)
        {
            continue;
        }

        const int add = static_cast<int>(weapons[i]->reserveMax * ratio);

        m_ammoReserve[i] =
            std::min(m_ammoReserve[i] + add, weapons[i]->reserveMax);
    }
}

void GameScene::UpdateDrops(float deltaTime)
{
    for (WorldDrop& drop : m_drops)
    {
        drop.bobTimer += deltaTime * 3.0f;
    }

    //--------------------------------------------------------
    // 弾薬は触れるだけで自動取得
    //--------------------------------------------------------
    const float playerCenterX = m_playerX + m_playerWidth * 0.5f;
    const float playerCenterY = m_playerY + m_playerHeight * 0.5f;

    for (AmmoPickup& pickup : m_ammoPickups)
    {
        pickup.bobTimer += deltaTime * 4.0f;

        //----------------------------------------------------
        // 一定距離まで近づいたら吸い寄せる。
        // 戦闘中でも自然に補給できるようにするため。
        //----------------------------------------------------
        const float dx = playerCenterX - pickup.x;
        const float dy = playerCenterY - pickup.y;

        const float distance = std::sqrt(dx * dx + dy * dy);

        if (distance > 1.0f && distance < 300.0f)
        {
            const float pullSpeed = 460.0f;

            pickup.x += (dx / distance) * pullSpeed * deltaTime;
            pickup.y += (dy / distance) * pullSpeed * deltaTime;
        }
    }

    for (auto it = m_ammoPickups.begin(); it != m_ammoPickups.end(); )
    {
        const bool touching = RectsOverlap(
            it->x - 22.0f, it->y - 22.0f, 44.0f, 44.0f,
            m_playerX, m_playerY, m_playerWidth, m_playerHeight
        );

        if (touching)
        {
            RefillReserve(0.35f);

            m_toastText = L"弾薬を補充した";
            m_toastTimer = 0.9f;

            it = m_ammoPickups.erase(it);
        }
        else
        {
            ++it;
        }
    }

    //--------------------------------------------------------
    // HP回復アイテムも触れるだけで取得する
    //--------------------------------------------------------
    for (HealthPickup& pickup : m_healthPickups)
    {
        pickup.bobTimer += deltaTime * 3.4f;

        const float dx = playerCenterX - pickup.x;
        const float dy = playerCenterY - pickup.y;

        const float distance = std::sqrt(dx * dx + dy * dy);

        if (distance > 1.0f && distance < 300.0f)
        {
            const float pullSpeed = 460.0f;

            pickup.x += (dx / distance) * pullSpeed * deltaTime;
            pickup.y += (dy / distance) * pullSpeed * deltaTime;
        }
    }

    for (auto it = m_healthPickups.begin(); it != m_healthPickups.end(); )
    {
        const bool touching = RectsOverlap(
            it->x - 22.0f, it->y - 22.0f, 44.0f, 44.0f,
            m_playerX, m_playerY, m_playerWidth, m_playerHeight
        );

        if (touching)
        {
            HealPlayer(it->amount);

            Audio::PlaySe(SeId::Heal);

            m_effectSystem.Spawn(
                EffectType::Dust, it->x, it->y, 1.1f
            );

            m_toastText = L"HPを回復した";
            m_toastTimer = 0.9f;

            it = m_healthPickups.erase(it);
        }
        else
        {
            ++it;
        }
    }

    //--------------------------------------------------------
    // 宝箱
    //--------------------------------------------------------
    // 宝箱をこのフレームで開けたか
    bool openedChest = false;

    for (TreasureChest& chest : m_chests)
    {
        if (!chest.active)
        {
            continue;
        }

        chest.animTimer += deltaTime;

        if (chest.opened)
        {
            //------------------------------------------------
            // 開いたあと3秒で消える。
            // 消える前は点滅させて、無くなることを知らせる。
            //------------------------------------------------
            if (chest.animTimer >= 1.5f)
            {
                chest.active = false;
            }

            continue;
        }

        const bool touching = RectsOverlap(
            chest.x - 34.0f, chest.y - 34.0f, 68.0f, 68.0f,
            m_playerX, m_playerY, m_playerWidth, m_playerHeight
        );

        if (!touching || !m_input.IsPressed('F'))
        {
            continue;
        }

        chest.opened = true;
        chest.animTimer = 0.0f;

        Audio::PlaySe(SeId::Chest);

        // このフレームは宝箱を開けただけにする
        openedChest = true;

        // 中身はステージ開始時に決めてある
        for (size_t i = 0; i < chest.contents.size(); ++i)
        {
            SpawnDropAt(
                chest.x + ((i == 0) ? -50.0f : 50.0f),
                GroundY() - 40.0f,
                chest.contents[i],
                true
            );
        }

        m_toastText = L"宝箱を開けた";
        m_toastTimer = 1.2f;
    }

    //--------------------------------------------------------
    // 拾う
    //--------------------------------------------------------
    m_hoveredDrop = FindTouchingDrop();

    //--------------------------------------------------------
    // 宝箱を開けた押しで装備まで拾ってしまうと、
    // 開けた瞬間に持ち物が入れ替わる。
    // 同じ押しでは片方だけにする。
    //--------------------------------------------------------
    if (!openedChest && m_hoveredDrop >= 0 && m_input.IsPressed('F'))
    {
        EquipDrop(m_hoveredDrop);
        m_hoveredDrop = -1;
    }

    // 捨てる
    if (m_hoveredDrop >= 0 && m_input.IsPressed('G'))
    {
        m_drops.erase(m_drops.begin() + m_hoveredDrop);
        m_hoveredDrop = -1;
    }
}

int GameScene::FindTouchingDrop() const
{
    for (size_t i = 0; i < m_drops.size(); ++i)
    {
        if (RectsOverlap(
            m_drops[i].x - 27.0f, m_drops[i].y - 27.0f, 54.0f, 54.0f,
            m_playerX, m_playerY, m_playerWidth, m_playerHeight))
        {
            return static_cast<int>(i);
        }
    }

    return -1;
}

void GameScene::EquipDrop(int dropIndex)
{
    if (dropIndex < 0 || dropIndex >= static_cast<int>(m_drops.size()))
    {
        return;
    }

    const EquipmentDrop item = m_drops[dropIndex].item;

    //--------------------------------------------------------
    // 今つけているものは捨てずに、その場へ落とす。
    // 拾い直せるので、付け替えて試すことができる。
    //--------------------------------------------------------
    EquipmentDrop replaced;

    if (item.isWeapon)
    {
        const bool isMain =
            (item.weapon.data.category == WeaponCategory::Main);

        if (isMain && m_loadout.HasMainWeapon())
        {
            replaced.dropped = true;
            replaced.isWeapon = true;
            replaced.weapon = m_loadout.GetMainWeapon();
        }
        else if (!isMain && m_loadout.HasSubWeapon())
        {
            replaced.dropped = true;
            replaced.isWeapon = true;
            replaced.weapon = m_loadout.GetSubWeapon();
        }
    }
    else if (m_loadout.HasArmor(item.armor.data.slot))
    {
        replaced.dropped = true;
        replaced.isWeapon = false;
        replaced.armor = m_loadout.GetArmor(item.armor.data.slot);
    }

    m_inventory.NotifyPickup(item);
    m_score.AddPickup(item.GetRarity());

    if (item.isWeapon)
    {
        m_loadout.EquipWeapon(item.weapon);

        // その武器スロットだけ弾薬を満タンにする
        RefreshWeapons(false);

        const int index =
            (item.weapon.data.category == WeaponCategory::Main) ? 0 : 1;

        const EffectiveWeapon& weapon =
            (index == 0) ? m_mainEffective : m_subEffective;

        m_ammoInMagazine[index] = weapon.magazineSize;
        m_ammoReserve[index] = weapon.reserveMax;

        m_toastText = L"装備： " + item.weapon.data.name;

        ShowWorldMessage(
            item.weapon.data.name + std::wstring(L" を装備"),
            item.weapon.data.rarity
        );
    }
    else
    {
        m_loadout.EquipArmor(item.armor);

        RefreshWeapons(false);

        m_toastText = L"装備： " + item.armor.data.name;

        ShowWorldMessage(
            item.armor.data.name + std::wstring(L" を装備"),
            item.armor.data.rarity
        );
    }

    m_toastTimer = 1.6f;

    m_drops.erase(m_drops.begin() + dropIndex);

    //--------------------------------------------------------
    // 外した装備を足元へ置く
    //--------------------------------------------------------
    if (replaced.dropped)
    {
        SpawnDropAt(
            m_playerX + m_playerWidth * 0.5f + 70.0f,
            GroundY() - 40.0f,
            replaced,
            false
        );
    }

    // 最大HPが増えた分は回復しない（増加分のみ追従）
    m_hp = std::min(m_hp, m_loadout.GetStats().maxHP);
}

//------------------------------------------------------------
// 湧き
//------------------------------------------------------------
void GameScene::UpdateSpawning(float deltaTime)
{
    UpdateArea(deltaTime);
}


//------------------------------------------------------------
// 区画ごとの進行
//
//   敵が出現する
//     ↓ 全滅させる
//   AREA CLEAR
//     ↓ 右端まで歩く
//   次の区画へ進み、また敵が出現する
//
// 一度進んだら前の区画へは戻れない。
//------------------------------------------------------------
void GameScene::UpdateArea(float deltaTime)
{
    if (m_advanceHintTimer > 0.0f)
    {
        m_advanceHintTimer -= deltaTime;
    }

    //--------------------------------------------------------
    // まだ制圧していない区画
    //--------------------------------------------------------
    if (!m_areaCleared)
    {
        if (!m_waveSpawned)
        {
            SpawnEnemyWave();
            m_waveSpawned = true;
            return;
        }

        if (m_enemies.empty())
        {
            m_areaCleared = true;
            m_advanceHintTimer = 2.5f;

            m_toastText = L"エリア クリア";
            m_toastTimer = 1.6f;
        }

        return;
    }

    //--------------------------------------------------------
    // 制圧済み。右端まで進んだら次の区画へ
    //--------------------------------------------------------
    //--------------------------------------------------------
    // 端まで歩けるようにしたので、制圧した瞬間に
    // すでに右端へ立っていることがある。
    // そのまま進めてしまうと、進む先の線が見えないまま
    // 次の区画へ移ってしまう。
    // 制圧してから少しの間は先へ行かせない。
    //--------------------------------------------------------
    if (m_advanceHintTimer > 1.9f)
    {
        return;
    }

    const bool atRightEdge =
        (m_playerX >= AreaRight() - m_playerWidth - 4.0f);

    if (!atRightEdge)
    {
        return;
    }

    if (m_areaIndex + 1 < m_areaCount)
    {
        ++m_areaIndex;

        m_areaCleared = false;
        m_waveSpawned = false;

        return;
    }

    //--------------------------------------------------------
    // 最後の区画まで終わったらステージクリア
    // （ボスステージは撃破判定が別にあるので何もしない）
    //--------------------------------------------------------
    if (!CurrentStage().bossStage && m_state == GameState::Playing)
    {
        m_state = GameState::StageClear;
        m_stateTimer = 3.0f;

        //----------------------------------------------------
        // ゴールラインを越えた合図。
        // ステージのBGMを止めてファンファーレへ差し替える。
        // リザルトへ移る直前に UpdateStageFlow が止める。
        //----------------------------------------------------
        Audio::PlayJingle(BgmPath::StageClear);

        m_score.AddStageClear(
            static_cast<int>(m_stageTime),
            GetLevelSetting(m_stageIndex).targetSeconds,
            !m_tookDamageThisStage
        );

        //----------------------------------------------------
        // 目標を達成していればボーナス
        //----------------------------------------------------
        if (IsObjectiveCleared() && m_objective.bonus > 0)
        {
            m_score.AddKill(m_objective.bonus, false, false);

            m_toastText =
                L"目標達成　+" + std::to_wstring(m_objective.bonus);

            m_toastTimer = 2.6f;
        }
    }
}


//------------------------------------------------------------
// エフェクト
//------------------------------------------------------------
//------------------------------------------------------------
// 頭上へお知らせを出す
//------------------------------------------------------------
void GameScene::ShowWorldMessage(const std::wstring& text, Rarity rarity)
{
    WorldMessage message;

    message.text = text;
    message.rarity = rarity;
    message.timer = 2.2f;
    message.offsetY = 0.0f;

    m_worldMessages.push_back(message);

    // 一度に出るのは3件までにする
    while (m_worldMessages.size() > 3)
    {
        m_worldMessages.erase(m_worldMessages.begin());
    }
}

void GameScene::SpawnDamageText(
    float x,
    float y,
    float value,
    bool crit,
    bool head
)
{
    DamageText text;

    text.x = x;
    text.y = y;
    text.value = value;
    text.isCrit = crit;
    text.isHeadShot = head;
    text.timer = 0.75f;
    text.velocityY = RandFloat(-95.0f, -60.0f);

    m_damageTexts.push_back(text);

    // 画面が埋まらないように上限を設ける
    if (m_damageTexts.size() > 60)
    {
        m_damageTexts.erase(m_damageTexts.begin());
    }
}

void GameScene::UpdateEffects(float deltaTime)
{
    for (ObstacleState& obstacle : m_obstacles)
    {
        if (obstacle.hitFlash > 0.0f)
        {
            obstacle.hitFlash -= deltaTime;
        }
    }

    for (EnemyHpTrack& track : m_enemyHpTracks)
    {
        track.timer -= deltaTime;
    }

    m_enemyHpTracks.erase(
        std::remove_if(
            m_enemyHpTracks.begin(),
            m_enemyHpTracks.end(),
            [](const EnemyHpTrack& t) { return t.timer <= 0.0f; }
        ),
        m_enemyHpTracks.end()
    );

    for (WorldMessage& message : m_worldMessages)
    {
        message.timer -= deltaTime;
        message.offsetY -= deltaTime * 18.0f;
    }

    m_worldMessages.erase(
        std::remove_if(
            m_worldMessages.begin(),
            m_worldMessages.end(),
            [](const WorldMessage& m) { return m.timer <= 0.0f; }
        ),
        m_worldMessages.end()
    );

    for (DamageText& text : m_damageTexts)
    {
        text.timer -= deltaTime;
        text.y += text.velocityY * deltaTime;
        text.velocityY += 120.0f * deltaTime;
    }

    m_damageTexts.erase(
        std::remove_if(
            m_damageTexts.begin(),
            m_damageTexts.end(),
            [](const DamageText& t) { return t.timer <= 0.0f; }
        ),
        m_damageTexts.end()
    );

    if (m_toastTimer > 0.0f)
    {
        m_toastTimer -= deltaTime;
    }

    //--------------------------------------------------------
    // Guardian 6：10秒ごとのShield付与
    //--------------------------------------------------------
    SynergySystem& synergy = m_loadout.GetSynergy();

    const float grant =
        synergy.ConsumeShieldGrant(m_loadout.GetStats().maxHP);

    if (grant > 0.0f)
    {
        m_shield = std::min(
            m_shield + grant,
            m_loadout.GetStats().maxHP * 0.6f
        );

        m_toastText = L"守護 : シールド獲得";
        m_toastTimer = 1.0f;
    }

    //--------------------------------------------------------
    // MYTHIC Emergency Shield：15秒ごとに最大HP15%
    //--------------------------------------------------------
    const float mythicShield =
        m_loadout.GetMythic().ConsumeShieldGrant(m_loadout.GetStats().maxHP);

    if (mythicShield > 0.0f)
    {
        m_shield = std::min(
            m_shield + mythicShield,
            m_loadout.GetStats().maxHP * 0.6f
        );

        m_toastText = L"ミシック : 緊急シールド";
        m_toastTimer = 1.2f;
    }
}

//------------------------------------------------------------
// ステージ進行
//------------------------------------------------------------
void GameScene::UpdateStageFlow(float deltaTime)
{
    const StageConfig& config = CurrentStage();

    switch (m_state)
    {
    case GameState::StageIntro:
        m_stateTimer -= deltaTime;

        if (m_stateTimer <= 0.0f)
        {
            m_state = GameState::Playing;
        }
        break;

    case GameState::Playing:
        if (config.bossStage)
        {
            //--------------------------------------------
            // ボスが倒れたらクリア
            //--------------------------------------------
            bool bossAlive = false;

            for (const Enemy& enemy : m_enemies)
            {
                if (enemy.kind == EnemyKind::Boss && enemy.alive)
                {
                    bossAlive = true;
                    break;
                }
            }

            if (m_bossSpawned && !bossAlive)
            {
                m_state = GameState::Victory;
                m_stateTimer = 0.0f;

                BuildResultData(true);
            }
        }
        // 通常ステージのクリア判定は UpdateArea が行う
        break;

    case GameState::StageClear:
    {
        m_stateTimer -= deltaTime;
        m_stageClearWait += deltaTime;

        if (m_stageClearPending)
        {
            break;
        }

        //----------------------------------------------------
        // ゴールで鳴らしたファンファーレを鳴らし切ってから
        // ResultScene へ移る。
        //
        // 曲が置かれていない場合は IsJinglePlaying が
        // すぐ false になるので、演出の3秒だけで先へ進む。
        // 長すぎる曲を入れられても止まらないよう、
        // 上限も見ている。
        //----------------------------------------------------
        constexpr float STAGE_CLEAR_WAIT_MAX = 20.0f;

        const bool introDone = (m_stateTimer <= 0.0f);

        const bool jingleDone =
            !Audio::IsJinglePlaying() ||
            (m_stageClearWait >= STAGE_CLEAR_WAIT_MAX);

        if (introDone && jingleDone)
        {
            //------------------------------------------------
            // ここを立てた次のフレームで main が
            // ResultScene へ切り替える。
            // 鳴り終わっているはずだが、上限で抜けた場合に
            // 備えてここで止めておく。
            //------------------------------------------------
            Audio::StopBgm();

            m_stageClearPending = true;
            BuildResultData(true);
        }
    }
    break;

    case GameState::GameOver:
    case GameState::Victory:
        m_stateTimer += deltaTime;

        if (m_input.IsPressed(VK_RETURN) ||
            m_input.IsPressed(VK_ESCAPE))
        {
            m_backToTitle = true;
        }
        break;
    }
}

//------------------------------------------------------------
// メイン更新
//------------------------------------------------------------
void GameScene::Update(HWND hwnd)
{
    const auto now = std::chrono::steady_clock::now();

    float deltaTime =
        std::chrono::duration<float>(now - m_lastUpdate).count();

    m_lastUpdate = now;

    // ウィンドウ復帰時に大きく飛ばないよう制限
    deltaTime = std::min(deltaTime, 0.05f);

    m_input.Capture();

    UpdateMouse(hwnd);

    //--------------------------------------------------------
    // 装備一覧
    //--------------------------------------------------------
    if (m_input.IsPressed(VK_TAB))
    {
        m_showInventory = !m_showInventory;
    }

    //--------------------------------------------------------
    // 装備一覧を開いている間は、時間を完全に止める。
    // 敵・弾・タイマー・制限時間もここで打ち切る。
    //--------------------------------------------------------
    if (m_showInventory)
    {
        return;
    }

    //--------------------------------------------------------
    // ポーズ（設計書 19-6.4）
    //--------------------------------------------------------
    if (m_pauseUI.IsOpen())
    {
        const PauseResult result =
            m_pauseUI.Update(m_input, m_uiMouseX, m_uiMouseY, deltaTime);

        if (result == PauseResult::Restart)
        {
            RetryStage();
        }
        else if (result == PauseResult::Title)
        {
            m_backToTitle = true;
        }
        else if (result == PauseResult::Option)
        {
            // 画面の切り替えは main が行う
            m_optionRequested = true;
        }

        // 開いている間はゲームを止める
        return;
    }

    if (m_input.IsPressed(VK_ESCAPE) &&
        m_state != GameState::GameOver &&
        m_state != GameState::Victory)
    {
        m_pauseUI.Open();
        return;
    }

    UpdateStageFlow(deltaTime);

    if (m_state == GameState::GameOver ||
        m_state == GameState::Victory)
    {
        UpdateEffects(deltaTime);
        return;
    }

    //--------------------------------------------------------
    // 進行中の処理
    //--------------------------------------------------------
    m_loadout.GetSynergy().Update(
        deltaTime,
        m_loadout.GetStats().maxHP
    );

    m_loadout.GetMythic().Update(deltaTime);

    //--------------------------------------------------------
    // 固有スキル（設計書 19-7）
    //   K = 1つ目 / L = 2つ目
    //--------------------------------------------------------
    for (int i = 0; i < 2; ++i)
    {
        if (m_skills[i])
        {
            m_skills[i]->Update(deltaTime);
        }
    }

    if (m_state == GameState::Playing)
    {
        if (m_input.IsPressed('K'))
        {
            UseSkill(0);
        }

        if (m_input.IsPressed('L'))
        {
            UseSkill(1);
        }
    }

    //--------------------------------------------------------
    // 向きと照準はここで決める。
    // ポーズ中・装備一覧中・ゲームオーバー後は
    // ここまで来ないので、照準は止まったままになる。
    //--------------------------------------------------------
    UpdateAim();

    UpdatePlayer(deltaTime);

    if (m_state == GameState::Playing)
    {
        m_stageTime += deltaTime;

        //----------------------------------------------------
        // 制限時間を超えたらその場で失敗
        //----------------------------------------------------
        const int limit = GetLevelSetting(m_stageIndex).targetSeconds;

        if (limit > 0 && m_stageTime >= static_cast<float>(limit))
        {
            m_toastText = L"TIME UP";
            m_toastTimer = 2.0f;

            m_state = GameState::GameOver;
            m_stateTimer = 0.0f;

            BuildResultData(false);
        }

        UpdateWeapon(deltaTime);
        UpdateSpawning(deltaTime);
    }

    m_score.Update(deltaTime);
    m_scoreDisplay.Update(deltaTime, m_score);

    // 背景の敵軍は、止まっている間も歩き続ける
    m_marchTime += deltaTime;
    m_effectSystem.Update(deltaTime);

    //--------------------------------------------------------
    // カメラはプレイヤーを追いかける
    //--------------------------------------------------------
    //--------------------------------------------------------
    // カメラはプレイヤーではなく「今の区画」に合わせる。
    // これで前の区画へスクロールして戻ることがなくなる。
    //--------------------------------------------------------
    m_camera.Update(deltaTime, AreaLeft() + SCREEN_WIDTH * 0.5f);

    UpdateBullets(deltaTime);
    UpdateEnemies(deltaTime);
    UpdateExplosions(deltaTime);
    UpdateDrops(deltaTime);
    UpdateEffects(deltaTime);
}

//------------------------------------------------------------
// 発動中のスキル効果をまとめる
//------------------------------------------------------------
SkillBuff GameScene::GetSkillBuff() const
{
    SkillBuff total;

    for (int i = 0; i < 2; ++i)
    {
        if (!m_skills[i])
        {
            continue;
        }

        const SkillBuff buff = m_skills[i]->GetBuff();

        total.fireRateBonus += buff.fireRateBonus;
        total.moveSpeedBonus += buff.moveSpeedBonus;
        total.critRateBonus += buff.critRateBonus;
        total.headShotBonus += buff.headShotBonus;
        total.attackBonus += buff.attackBonus;
        total.lifesteal += buff.lifesteal;

        total.invincible = total.invincible || buff.invincible;
        total.infiniteAmmo = total.infiniteAmmo || buff.infiniteAmmo;
    }

    return total;
}

//------------------------------------------------------------
// スキルを使う
//------------------------------------------------------------
void GameScene::UseSkill(int slot)
{
    if (slot < 0 || slot > 1 || !m_skills[slot])
    {
        return;
    }

    if (!m_skills[slot]->CanUse())
    {
        return;
    }

    const SkillActivation activation = m_skills[slot]->Execute(
        m_loadout.GetSynergy(),
        m_loadout.GetStats().maxHP
    );

    if (!activation.activated)
    {
        return;
    }

    ApplySkillActivation(activation);

    //--------------------------------------------------------
    // スキル音はキャラクターと枠（K / L）で4通り
    //--------------------------------------------------------
    const bool assault =
        (m_loadout.GetCharacterType() == PlayerType::Player2);

    if (slot == 0)
    {
        Audio::PlaySe(assault ? SeId::SkillAssault : SeId::SkillHeavy);
    }
    else
    {
        Audio::PlaySe(assault ? SeId::SkillAssaultL : SeId::SkillHeavyL);
    }

    m_toastText = std::wstring(L"スキル : ") + m_skills[slot]->GetName();
    m_toastTimer = 1.4f;
}

//------------------------------------------------------------
// 発動した瞬間の効果を反映する
//------------------------------------------------------------
void GameScene::ApplySkillActivation(const SkillActivation& activation)
{
    const float centerX = m_playerX + m_playerWidth * 0.5f;
    const float centerY = m_playerY + m_playerHeight * 0.5f;

    //--------------------------------------------------------
    // 弾倉を満タンにする（制圧射撃）
    //
    // 弾切れのまま発動すると、FireCurrentWeapon が
    // 「弾が無い」で先にリロードへ回してしまい、
    // 弾薬無制限の効果を活かせないため。
    //--------------------------------------------------------
    if (activation.refillMagazine)
    {
        RefillMagazine();
    }

    if (activation.shield > 0.0f)
    {
        m_shield = std::min(
            m_shield + activation.shield,
            m_loadout.GetStats().maxHP * 0.6f
        );
    }

    if (activation.heal > 0.0f)
    {
        HealPlayer(activation.heal);
    }

    //--------------------------------------------------------
    // 衝撃波：周囲の敵を押し返してDamageを与える
    //--------------------------------------------------------
    if (activation.shockWave)
    {
        m_effectSystem.Spawn(
            EffectType::Explosion, centerX, centerY,
            activation.shockRadius / 90.0f
        );

        for (Enemy& enemy : m_enemies)
        {
            if (!enemy.alive)
            {
                continue;
            }

            const float dx = enemy.CenterX() - centerX;
            const float dy = enemy.CenterY() - centerY;

            if (dx * dx + dy * dy >
                activation.shockRadius * activation.shockRadius)
            {
                continue;
            }

            // ボスは押されにくい
            const float resist =
                (enemy.kind == EnemyKind::Boss) ? 0.25f : 1.0f;

            const float direction = (dx >= 0.0f) ? 1.0f : -1.0f;

            enemy.x += direction * activation.knockBack * resist;

            enemy.x = std::clamp(
                enemy.x,
                AreaLeft() + 8.0f,
                AreaRight() - enemy.width - 8.0f
            );

            ApplyDamageToEnemy(enemy, activation.shockDamage, false, false);
        }
    }

    //--------------------------------------------------------
    // 瞬間移動：方向キーの向きへ跳ぶ
    //--------------------------------------------------------
    if (activation.blinkDistance > 0.0f)
    {
        float direction = m_facingRight ? 1.0f : -1.0f;

        if (m_input.IsDown('A') || m_input.IsDown(VK_LEFT))
        {
            direction = -1.0f;
        }
        else if (m_input.IsDown('D') || m_input.IsDown(VK_RIGHT))
        {
            direction = 1.0f;
        }

        m_effectSystem.Spawn(EffectType::Dust, centerX, centerY, 1.4f);

        m_playerX += direction * activation.blinkDistance;

        // 上方向を押していれば少し浮く
        if (m_input.IsDown(VK_UP) || m_input.IsDown('W'))
        {
            m_playerY -= 140.0f;
            m_velocityY = 0.0f;
            m_isGrounded = false;
        }

        m_effectSystem.Spawn(
            EffectType::Dust,
            m_playerX + m_playerWidth * 0.5f,
            m_playerY + m_playerHeight * 0.5f,
            1.4f
        );

        // 短いあいだ無敵にする
        m_invincibleTimer = std::max(m_invincibleTimer, 0.25f);
    }
}

//------------------------------------------------------------
// 目標の進み具合
//------------------------------------------------------------
int GameScene::GetObjectiveProgress() const
{
    switch (m_objective.type)
    {
    case ObjectiveType::NoDamage:
        return m_tookDamageThisStage ? 1 : 0;

    case ObjectiveType::TimeLimit:
        return static_cast<int>(m_stageTime);

    case ObjectiveType::HeadShot:
        return m_score.GetHeadShotCount() - m_headShotBase;

    case ObjectiveType::EliteKill:
        return m_eliteKillCount - m_eliteBase;

    case ObjectiveType::Combo:
        return m_score.GetMaxCombo() - m_comboBase;

    case ObjectiveType::Pickup:
        return m_inventory.GetTotalPickup() - m_pickupBase;
    }

    return 0;
}

//------------------------------------------------------------
// 目標を満たしているか
//------------------------------------------------------------
bool GameScene::IsObjectiveCleared() const
{
    const int progress = GetObjectiveProgress();

    switch (m_objective.type)
    {
    case ObjectiveType::NoDamage:
        return !m_tookDamageThisStage;

    case ObjectiveType::TimeLimit:
        return progress <= m_objective.target;

    default:
        return progress >= m_objective.target;
    }
}


//------------------------------------------------------------
// リザルト画面へ渡す集計を作る
// 設計書 Ver.3 19-1.4
//------------------------------------------------------------
void GameScene::BuildResultData(bool cleared)
{
    m_result.Clear();

    m_result.stageIndex = m_stageIndex;
    m_result.livesLeft = m_lives;
    m_result.cleared = cleared;
    m_result.clearTime = m_stageTime;

    m_result.killCount = m_totalKills;
    m_result.eliteKillCount = m_eliteKillCount;
    m_result.score = m_score.GetScore();
    m_result.maxCombo = m_score.GetMaxCombo();

    m_result.pickupCount = m_inventory.GetTotalPickup();
    m_result.mythicCount = m_inventory.GetMythicCount();
    m_result.bestRarity = m_inventory.GetBestRarity();

    m_result.damageDealt = m_damageDealt;
    m_result.damageTaken = m_damageTaken;
    m_result.highestDamage = m_highestDamage;

    m_result.shotsFired = m_shotsFired;
    m_result.shotsHit = m_shotsHit;
    m_result.headShotCount = m_score.GetHeadShotCount();

    //--------------------------------------------------------
    // 発動しているシナジー
    //--------------------------------------------------------
    const SynergySystem& synergy = m_loadout.GetSynergy();

    for (int i = 0; i < SYNERGY_COUNT; ++i)
    {
        const SynergyType type = static_cast<SynergyType>(i);
        const int count = synergy.GetCount(type);

        if (count <= 0)
        {
            continue;
        }

        SynergyResult entry;

        entry.type = type;
        entry.count = count;
        entry.tier = synergy.GetTier(type);

        m_result.synergies.push_back(entry);
    }

    //--------------------------------------------------------
    // 装備一覧
    //--------------------------------------------------------
    if (m_loadout.HasMainWeapon())
    {
        m_result.equipmentNames.push_back(
            m_loadout.GetMainWeapon().data.name
        );
    }

    if (m_loadout.HasSubWeapon())
    {
        m_result.equipmentNames.push_back(
            m_loadout.GetSubWeapon().data.name
        );
    }

    for (int i = 0; i < ARMOR_SLOT_COUNT; ++i)
    {
        const ArmorSlot slot = static_cast<ArmorSlot>(i);

        if (!m_loadout.HasArmor(slot))
        {
            continue;
        }

        m_result.equipmentNames.push_back(ToString(slot));
    }
}


bool GameScene::IsBackToTitle() const
{
    return m_backToTitle;
}

void GameScene::ResetBackRequest()
{
    m_backToTitle = false;
}

//============================================================
// 描画ヘルパ
//============================================================

namespace
{
}

void GameScene::SetBrush(float r, float g, float b, float a)
{
    if (m_brush)
    {
        m_brush->SetColor(D2D1::ColorF(r, g, b, a));
        m_brush->SetOpacity(1.0f);
    }
}

void GameScene::SetRarityBrush(Rarity rarity, float alpha)
{
    const RarityColor color = GetRarityColor(rarity);

    SetBrush(color.r, color.g, color.b, alpha);
}

void GameScene::DrawTextLine(
    const std::wstring& text,
    float left,
    float top,
    float right,
    float bottom,
    IDWriteTextFormat* format
)
{
    if (!m_renderTarget || !format || text.empty())
    {
        return;
    }

    m_renderTarget->DrawText(
        text.c_str(),
        static_cast<UINT32>(text.size()),
        format,
        D2D1::RectF(left, top, right, bottom),
        m_brush.Get()
    );
}

void GameScene::DrawBar(
    float x,
    float y,
    float width,
    float height,
    float ratio,
    float r,
    float g,
    float b
)
{
    ratio = std::clamp(ratio, 0.0f, 1.0f);

    // 背景
    SetBrush(0.05f, 0.07f, 0.09f, 0.85f);

    m_renderTarget->FillRectangle(
        D2D1::RectF(x, y, x + width, y + height),
        m_brush.Get()
    );

    // 本体
    SetBrush(r, g, b, 1.0f);

    m_renderTarget->FillRectangle(
        D2D1::RectF(x, y, x + width * ratio, y + height),
        m_brush.Get()
    );

    // 枠
    SetBrush(0.35f, 0.42f, 0.50f, 0.9f);

    m_renderTarget->DrawRectangle(
        D2D1::RectF(x, y, x + width, y + height),
        m_brush.Get(),
        1.0f
    );
}

//============================================================
// 説明文の組み立て
//============================================================

std::wstring GameScene::BuildItemText(const EquipmentDrop& item) const
{
    return ItemInfoUI::BuildText(item);
}

std::wstring GameScene::BuildEquippedText(const EquipmentDrop& incoming) const
{
    return ItemInfoUI::BuildEquippedText(incoming, m_loadout);
}


//============================================================
// ワールド描画
//============================================================

void GameScene::DrawBackground()
{
    const D2D1_RECT_F screen =
        D2D1::RectF(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT);

    //--------------------------------------------------------
    // 背景が1枚も読めなかった場合の下地
    // （設計書 19-13：背景失敗時は SolidColor）
    //--------------------------------------------------------
    SetBrush(0.05f, 0.09f, 0.12f, 1.0f);
    m_renderTarget->FillRectangle(screen, m_brush.Get());

    if (m_map)
    {
        int layerIndex = 0;

        for (const BackgroundLayer& layer : m_map->GetLayers())
        {
            ID2D1Bitmap* bitmap = m_resources.GetBitmap(layer.texturePath);

            if (!bitmap)
            {
                ++layerIndex;
                continue;
            }

            const D2D1_SIZE_F size = bitmap->GetSize();

            if (size.width <= 0.0f || size.height <= 0.0f)
            {
                ++layerIndex;
                continue;
            }

            //------------------------------------------------
            // この層が受け持つ画面の帯
            //------------------------------------------------
            const float bandTop = SCREEN_HEIGHT * layer.bandTop;
            const float bandBottom = SCREEN_HEIGHT * layer.bandBottom;

            if (bandBottom - bandTop < 1.0f)
            {
                ++layerIndex;
                continue;
            }

            // 帯の外へはみ出さないようにする
            m_renderTarget->PushAxisAlignedClip(
                D2D1::RectF(0.0f, bandTop, SCREEN_WIDTH, bandBottom),
                D2D1_ANTIALIAS_MODE_ALIASED
            );

            const float drawHeight = SCREEN_HEIGHT * layer.heightScale;
            const float drawWidth = size.width * (drawHeight / size.height);

            if (drawWidth > 1.0f)
            {
                // 近景は画面の下端にそろえる
                const float top = layer.anchorBottom
                    ? (SCREEN_HEIGHT - drawHeight + layer.offsetY)
                    : layer.offsetY;
                const float bottom = top + drawHeight;

                //--------------------------------------------
                // 視差を掛けた位置から、画面を覆うぶんだけ並べる。
                // 1枚ごとに左右反転させ、つなぎ目を目立たなくする。
                //--------------------------------------------
                const float offset = m_camera.GetParallaxOffset(layer.parallax);

                const int firstTile =
                    static_cast<int>(std::floor((0.0f - offset) / drawWidth));

                const int lastTile =
                    static_cast<int>(std::floor((SCREEN_WIDTH - offset) / drawWidth));

                for (int i = firstTile; i <= lastTile; ++i)
                {
                    const float left = offset + i * drawWidth;

                    D2D1_MATRIX_3X2_F oldTransform;
                    m_renderTarget->GetTransform(&oldTransform);

                    if ((i % 2) != 0)
                    {
                        m_renderTarget->SetTransform(
                            D2D1::Matrix3x2F::Scale(
                                D2D1::SizeF(-1.0f, 1.0f),
                                D2D1::Point2F(left + drawWidth * 0.5f,
                                              top + drawHeight * 0.5f)
                            ) * oldTransform
                        );
                    }

                    m_renderTarget->DrawBitmap(
                        bitmap,
                        D2D1::RectF(left, top, left + drawWidth, bottom),
                        1.0f,
                        D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
                    );

                    m_renderTarget->SetTransform(oldTransform);
                }
            }

            m_renderTarget->PopAxisAlignedClip();

            //------------------------------------------------
            // 背景の敵軍は出さない。
            // 戻したい場合はここで DrawMarchingArmy() を呼ぶ。
            //------------------------------------------------
            ++layerIndex;
        }
    }

    //--------------------------------------------------------
    // 被弾フラッシュ
    //--------------------------------------------------------
    if (m_damageFlash > 0.0f)
    {
        SetBrush(0.85f, 0.10f, 0.12f, m_damageFlash * 0.30f);
        m_renderTarget->FillRectangle(screen, m_brush.Get());
    }
}


//------------------------------------------------------------
// 背景を進んでいく敵軍
//
// 遊びには関わらない見た目だけの演出。
// 敵のスプライトを小さく薄く描き、
// カメラより少し遅い速さで流している。
//------------------------------------------------------------
void GameScene::DrawMarchingArmy()
{
    //--------------------------------------------------------
    // 今は呼んでいない。
    // 背景に敵軍を流したくなったら、
    // DrawBackground() の中景のあとで呼べば戻る。
    //--------------------------------------------------------
    if (!m_enemyBitmap)
    {
        return;
    }

    // enemies.png：1セル 61x68、横5種×縦2フレーム
    constexpr float CELL_W = 61.0f;
    constexpr float CELL_H = 68.0f;

    constexpr int COUNT = 16;
    constexpr float SPACING = 170.0f;
    constexpr float WRAP = COUNT * SPACING;

    constexpr float DRAW_W = 38.0f;
    constexpr float DRAW_H = 42.0f;

    // 地平線のあたり。近景より奥に見えるよう高めに置く
    constexpr float BASE_Y = 374.0f;

    constexpr float PARALLAX = 0.60f;
    constexpr float MARCH_SPEED = 26.0f;

    const float scroll =
        m_camera.GetX() * PARALLAX + m_marchTime * MARCH_SPEED;

    for (int i = 0; i < COUNT; ++i)
    {
        //----------------------------------------------------
        // 折り返して並べる。
        // 1体ぶん余分に左へ出しておくと、端で消えない。
        //----------------------------------------------------
        float x = std::fmod(i * SPACING - scroll, WRAP);

        if (x < 0.0f)
        {
            x += WRAP;
        }

        x -= SPACING;

        if (x < -DRAW_W || x > SCREEN_WIDTH + DRAW_W)
        {
            continue;
        }

        //----------------------------------------------------
        // 隊列がそろいすぎないよう、少しずらす
        //----------------------------------------------------
        const float phase = static_cast<float>(i) * 0.7f;

        const float bob =
            std::sin(m_marchTime * 5.0f + phase) * 2.0f;

        const float depth =
            0.86f + std::fmod(phase, 0.28f);

        const float width = DRAW_W * depth;
        const float height = DRAW_H * depth;

        const float top = BASE_Y - height + bob;

        //----------------------------------------------------
        // 歩行の2コマ。種類は並びで変える
        //----------------------------------------------------
        const int frame =
            (static_cast<int>(m_marchTime * 6.0f + phase) % 2 == 0) ? 0 : 1;

        const int column = i % 5;

        const D2D1_RECT_F source = D2D1::RectF(
            column * CELL_W, frame * CELL_H,
            (column + 1) * CELL_W, (frame + 1) * CELL_H
        );

        //----------------------------------------------------
        // 奥にいるので薄く描く
        //----------------------------------------------------
        m_renderTarget->DrawBitmap(
            m_enemyBitmap,
            D2D1::RectF(x, top, x + width, top + height),
            0.42f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
            source
        );

        //----------------------------------------------------
        // 足元に影を落として地面に乗って見せる
        //----------------------------------------------------
        SetBrush(0.06f, 0.05f, 0.08f, 0.22f);

        m_renderTarget->FillEllipse(
            D2D1::Ellipse(
                D2D1::Point2F(x + width * 0.5f, BASE_Y + 2.0f),
                width * 0.42f, 3.0f
            ),
            m_brush.Get()
        );
    }

    //--------------------------------------------------------
    // 進軍で立つ土煙
    //--------------------------------------------------------
    for (int i = 0; i < 10; ++i)
    {
        const float seed = static_cast<float>(i) * 5.3f;

        float x = std::fmod(seed * 137.0f - scroll * 0.9f, SCREEN_WIDTH + 160.0f);

        if (x < 0.0f)
        {
            x += SCREEN_WIDTH + 160.0f;
        }

        x -= 80.0f;

        const float size =
            10.0f + std::sin(m_marchTime * 1.4f + seed) * 4.0f;

        SetBrush(0.32f, 0.28f, 0.26f, 0.10f);

        m_renderTarget->FillEllipse(
            D2D1::Ellipse(
                D2D1::Point2F(x, BASE_Y - 4.0f),
                size * 1.8f, size * 0.5f
            ),
            m_brush.Get()
        );
    }
}


//------------------------------------------------------------
// マップの飾り（木・柵・建物・街灯など）
//
// tile が指定してあれば terrain_*.png から切り出して描く。
// 指定が無い、または画像が読めなかった場合は
// kind に応じた図形で代用する（設計書 19-13）。
//
// foreground が false の飾りは地形より奥、
// true の飾りはプレイヤーより手前に描く。
// 同じ関数を2回呼び分けることで前後を作っている。
//------------------------------------------------------------
void GameScene::DrawProps(bool foreground)
{
    if (!m_map)
    {
        return;
    }

    // terrain_*.png：1マス96角
    constexpr float PROP_TILE = 96.0f;

    for (const MapProp& prop : m_map->GetProps())
    {
        if (prop.foreground != foreground)
        {
            continue;
        }

        if (!m_camera.IsVisible(prop.worldX, prop.width))
        {
            continue;
        }

        const float left = ToScreenX(prop.worldX);
        const float top = prop.baseY - prop.height;

        const D2D1_RECT_F rect =
            D2D1::RectF(left, top, left + prop.width, prop.baseY);

        //----------------------------------------------------
        // タイルから切り出して描く
        //----------------------------------------------------
        if (m_terrainBitmap && prop.tile.IsValid())
        {
            const D2D1_RECT_F source = D2D1::RectF(
                prop.tile.col * PROP_TILE,
                prop.tile.row * PROP_TILE,
                (prop.tile.col + 1) * PROP_TILE,
                (prop.tile.row + 1) * PROP_TILE
            );

            //------------------------------------------------
            // マスの下側にある透明な余白ぶんだけ下げて描く。
            //
            // 草や花のマスは絵が中ほどまでしか無いものが多く、
            // マスの下端を地面に合わせると絵が浮いて見える。
            // 絵の下端が地面に来るようにずらす。
            //------------------------------------------------
            const int opaqueBottom = TerrainTile::GetOpaqueBottom(
                m_map->GetTerrainTexture(),
                prop.tile.col,
                prop.tile.row
            );

            const float sink =
                prop.height *
                (static_cast<float>(TerrainTile::TILE - opaqueBottom) /
                 static_cast<float>(TerrainTile::TILE));

            const D2D1_RECT_F placed = D2D1::RectF(
                rect.left,
                rect.top + sink,
                rect.right,
                rect.bottom + sink
            );

            D2D1_MATRIX_3X2_F oldTransform;
            m_renderTarget->GetTransform(&oldTransform);

            if (prop.flip)
            {
                const float centerX = left + prop.width * 0.5f;

                m_renderTarget->SetTransform(
                    D2D1::Matrix3x2F::Scale(
                        D2D1::SizeF(-1.0f, 1.0f),
                        D2D1::Point2F(centerX, prop.baseY)
                    ) * oldTransform
                );
            }

            m_renderTarget->DrawBitmap(
                m_terrainBitmap,
                placed,
                1.0f,
                D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
                source
            );

            m_renderTarget->SetTransform(oldTransform);

            continue;
        }

        switch (prop.kind)
        {
        case 0:  // 木
            SetBrush(0.22f, 0.14f, 0.08f, 0.95f);
            m_renderTarget->FillRectangle(
                D2D1::RectF(
                    left + prop.width * 0.35f, top + prop.height * 0.45f,
                    left + prop.width * 0.65f, prop.baseY
                ),
                m_brush.Get()
            );
            SetBrush(0.10f, 0.32f, 0.14f, 0.95f);
            m_renderTarget->FillEllipse(
                D2D1::Ellipse(
                    D2D1::Point2F(left + prop.width * 0.5f,
                                  top + prop.height * 0.28f),
                    prop.width * 0.62f,
                    prop.height * 0.30f
                ),
                m_brush.Get()
            );
            break;

        case 1:  // 岩
            SetBrush(0.34f, 0.34f, 0.36f, 0.95f);
            m_renderTarget->FillRoundedRectangle(
                D2D1::RoundedRect(rect, 10.0f, 10.0f),
                m_brush.Get()
            );
            break;

        case 2:  // 柵
            SetBrush(0.42f, 0.28f, 0.14f, 0.9f);
            m_renderTarget->FillRectangle(
                D2D1::RectF(left, prop.baseY - prop.height * 0.55f,
                            left + prop.width, prop.baseY - prop.height * 0.40f),
                m_brush.Get()
            );
            for (int i = 0; i < 4; ++i)
            {
                const float px = left + prop.width * (0.08f + i * 0.28f);

                m_renderTarget->FillRectangle(
                    D2D1::RectF(px, top, px + 7.0f, prop.baseY),
                    m_brush.Get()
                );
            }
            break;

        case 3:  // 建物
            SetBrush(0.16f, 0.17f, 0.22f, 0.92f);
            m_renderTarget->FillRectangle(rect, m_brush.Get());
            SetBrush(0.95f, 0.85f, 0.45f, 0.55f);
            for (int wy = 0; wy < 4; ++wy)
            {
                for (int wx = 0; wx < 3; ++wx)
                {
                    const float ax = left + 18.0f + wx * 46.0f;
                    const float ay = top + 20.0f + wy * 42.0f;

                    if (ax + 22.0f > left + prop.width) continue;
                    if (ay + 26.0f > prop.baseY) continue;

                    m_renderTarget->FillRectangle(
                        D2D1::RectF(ax, ay, ax + 22.0f, ay + 26.0f),
                        m_brush.Get()
                    );
                }
            }
            break;

        default: // 街灯
            SetBrush(0.30f, 0.32f, 0.36f, 0.95f);
            m_renderTarget->FillRectangle(rect, m_brush.Get());
            SetBrush(1.0f, 0.92f, 0.60f, 0.75f);
            m_renderTarget->FillEllipse(
                D2D1::Ellipse(
                    D2D1::Point2F(left + prop.width * 0.5f, top + 6.0f),
                    16.0f, 10.0f
                ),
                m_brush.Get()
            );
            break;
        }
    }
}


//------------------------------------------------------------
// 段差と障害物
//------------------------------------------------------------
void GameScene::DrawTerrain()
{
    // terrain_*.png：1マス96角、横12列×縦9行
    //   どのマスを使うかはステージ（MapBase / MapObstacle）が持つ。
    constexpr float TILE = 96.0f;

    const TileRef topTile = m_map
        ? m_map->GetPlatformTopTile()
        : TileRef{ 0, 0 };

    const TileRef fillTile = m_map
        ? m_map->GetPlatformFillTile()
        : TileRef{ 1, 0 };

    //--------------------------------------------------------
    // 段差：地面タイルを横に並べる
    //--------------------------------------------------------
    if (m_map)
    {
        for (const MapPlatform& platform : m_map->GetPlatforms())
        {
            if (!m_camera.IsVisible(platform.worldX, platform.width))
            {
                continue;
            }

            const float left = ToScreenX(platform.worldX);
            const float right = left + platform.width;

            if (m_terrainBitmap)
            {
                //--------------------------------------------
                // 96角のタイルを縦横に敷き詰める。
                // 幅も高さも96の倍数にしてあるので隙間ができない。
                //--------------------------------------------
                const float bottom = platform.topY + platform.height;

                for (float y = platform.topY; y < bottom; y += TILE)
                {
                    // 一番上の段だけ草の乗ったタイルにする
                    const bool topRow = (y < platform.topY + 1.0f);

                    const TileRef& cell = topRow ? topTile : fillTile;

                    const float cellX = cell.col * TILE;
                    const float cellY = cell.row * TILE;

                    for (float x = left; x < right; x += TILE)
                    {
                        const float w = std::min(TILE, right - x);
                        const float h = std::min(TILE, bottom - y);

                        const D2D1_RECT_F source = D2D1::RectF(
                            cellX, cellY,
                            cellX + TILE * (w / TILE),
                            cellY + TILE * (h / TILE)
                        );

                        m_renderTarget->DrawBitmap(
                            m_terrainBitmap,
                            D2D1::RectF(x, y, x + w, y + h),
                            1.0f,
                            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                            source
                        );
                    }
                }
            }
            else
            {
                SetBrush(0.16f, 0.19f, 0.17f, 0.95f);

                m_renderTarget->FillRectangle(
                    D2D1::RectF(left, platform.topY,
                                right, platform.topY + platform.height),
                    m_brush.Get()
                );

                SetBrush(0.45f, 0.78f, 0.62f, 0.95f);

                m_renderTarget->FillRectangle(
                    D2D1::RectF(left, platform.topY, right, platform.topY + 5.0f),
                    m_brush.Get()
                );
            }
        }
    }

    //--------------------------------------------------------
    // 障害物
    //--------------------------------------------------------
    for (const ObstacleState& obstacle : m_obstacles)
    {
        if (!obstacle.alive)
        {
            continue;
        }

        if (!m_camera.IsVisible(obstacle.data.worldX, obstacle.data.width))
        {
            continue;
        }

        const float left = ToScreenX(obstacle.data.worldX);
        const float top = obstacle.TopY();
        const float right = left + obstacle.data.width;
        const float bottom = obstacle.data.baseY;

        if (m_terrainBitmap)
        {
            //------------------------------------------------
            // 使うマスは障害物ごとに持たせてある。
            // 指定が無い場合だけ kind から選ぶ。
            //   0:木箱 → 2列目 / 1:石 → 4列目 / 2:樽 → 3列目
            //------------------------------------------------
            TileRef cell = obstacle.data.tile;

            if (!cell.IsValid())
            {
                cell.row = 0;
                cell.col = 2;

                if (obstacle.data.kind == 1)      cell.col = 4;
                else if (obstacle.data.kind == 2) cell.col = 3;
            }

            const D2D1_RECT_F source = D2D1::RectF(
                cell.col * TILE, cell.row * TILE,
                (cell.col + 1) * TILE, (cell.row + 1) * TILE
            );

            m_renderTarget->DrawBitmap(
                m_terrainBitmap,
                D2D1::RectF(left, top, right, bottom),
                1.0f,
                D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
                source
            );

            // 被弾の合図は着弾点のヒットエフェクトで出す（敵と同じ）
        }
        else
        {
            float r = 0.52f, g = 0.36f, b = 0.18f;

            if (obstacle.data.kind == 1) { r = 0.42f; g = 0.44f; b = 0.47f; }
            else if (obstacle.data.kind == 2) { r = 0.30f; g = 0.46f; b = 0.34f; }

            if (obstacle.hitFlash > 0.0f)
            {
                r = std::min(1.0f, r + 0.5f);
                g = std::min(1.0f, g + 0.5f);
                b = std::min(1.0f, b + 0.5f);
            }

            SetBrush(r, g, b, 1.0f);

            m_renderTarget->FillRoundedRectangle(
                D2D1::RoundedRect(
                    D2D1::RectF(left, top, right, bottom), 5.0f, 5.0f),
                m_brush.Get()
            );
        }

        //----------------------------------------------------
        // 耐久値ゲージ
        //----------------------------------------------------
        DrawBar(
            left, top - 12.0f, obstacle.data.width, 6.0f,
            obstacle.GetHpRatio(),
            0.90f, 0.70f, 0.25f
        );
    }
}


//------------------------------------------------------------
// 進む先の線
//
// 区画の敵を全滅させたときだけ、区画の右端に線を引く。
// どこまで行けば次へ進めるのかを、その場で示す。
//
// 最後の区画ならステージの終わりなので、色を変える。
//------------------------------------------------------------
void GameScene::DrawAreaGate()
{
    if (!m_areaCleared || m_state != GameState::Playing)
    {
        return;
    }

    const float x = ToScreenX(AreaRight() - 10.0f);

    if (x < -60.0f || x > SCREEN_WIDTH + 60.0f)
    {
        return;
    }

    const float groundY = GroundY();

    const bool isLast = (m_areaIndex + 1 >= m_areaCount);

    //--------------------------------------------------------
    // ステージの終わりは金色、途中は緑
    //--------------------------------------------------------
    const float r = isLast ? 1.00f : 0.42f;
    const float g = isLast ? 0.84f : 0.95f;
    const float b = isLast ? 0.32f : 0.58f;

    const float pulse =
        0.5f + 0.5f * std::sin(m_stageTime * 3.2f);

    //--------------------------------------------------------
    // 光の帯。中心から外へ薄くする
    //--------------------------------------------------------
    for (int i = 5; i >= 1; --i)
    {
        const float t = static_cast<float>(i) / 5.0f;
        const float width = 6.0f + t * 26.0f;

        SetBrush(r, g, b, (0.05f + pulse * 0.05f) * (1.0f - t));

        m_renderTarget->FillRectangle(
            D2D1::RectF(x - width, 0.0f, x + width, groundY + 30.0f),
            m_brush.Get()
        );
    }

    //--------------------------------------------------------
    // 線そのもの
    //--------------------------------------------------------
    SetBrush(r, g, b, 0.65f + pulse * 0.30f);

    m_renderTarget->FillRectangle(
        D2D1::RectF(x - 2.0f, 0.0f, x + 2.0f, groundY + 30.0f),
        m_brush.Get()
    );

    //--------------------------------------------------------
    // 進む向きを示す矢印。上へ流れる
    //--------------------------------------------------------
    constexpr int MARKS = 6;
    constexpr float SPAN = 96.0f;

    for (int i = 0; i < MARKS; ++i)
    {
        const float offset =
            std::fmod(m_stageTime * 46.0f + i * SPAN, MARKS * SPAN);

        const float y = groundY - offset;

        if (y < -20.0f || y > groundY)
        {
            continue;
        }

        // 上へ行くほど薄くする
        const float alpha =
            std::clamp(y / groundY, 0.0f, 1.0f) * 0.85f;

        SetBrush(r, g, b, alpha);

        for (int k = 0; k < 2; ++k)
        {
            const float dx = k * 7.0f;

            m_renderTarget->DrawLine(
                D2D1::Point2F(x - 12.0f + dx, y - 9.0f),
                D2D1::Point2F(x + 2.0f + dx, y),
                m_brush.Get(),
                2.4f
            );

            m_renderTarget->DrawLine(
                D2D1::Point2F(x + 2.0f + dx, y),
                D2D1::Point2F(x - 12.0f + dx, y + 9.0f),
                m_brush.Get(),
                2.4f
            );
        }
    }

    //--------------------------------------------------------
    // 地面に足元の印
    //--------------------------------------------------------
    SetBrush(r, g, b, 0.45f + pulse * 0.25f);

    m_renderTarget->FillEllipse(
        D2D1::Ellipse(D2D1::Point2F(x, groundY + 4.0f), 34.0f, 8.0f),
        m_brush.Get()
    );

    //--------------------------------------------------------
    // 何のための線かを添える
    //--------------------------------------------------------
    SetBrush(r, g, b, 0.95f);

    DrawTextLine(
        isLast ? L"ゴール" : L"次のエリアへ",
        x - 110.0f, groundY + 14.0f, x + 110.0f, groundY + 44.0f,
        m_smallFormat.Get()
    );
}


void GameScene::DrawGround()
{
    const float groundY = GroundY();

    constexpr float TILE = 96.0f;

    //--------------------------------------------------------
    // 地面も段差と同じマスで敷き詰める。
    // 無地のままだと、タイルで描いた段差から浮いて見えるため。
    //
    // ワールド座標に合わせて並べるので、
    // 歩くと地面の模様が一緒に流れる。
    //--------------------------------------------------------
    const TileRef topTile = m_map
        ? m_map->GetPlatformTopTile()
        : TileRef{ 0, 0 };

    const TileRef fillTile = m_map
        ? m_map->GetPlatformFillTile()
        : TileRef{ 1, 0 };

    if (m_terrainBitmap && topTile.IsValid() && fillTile.IsValid())
    {
        const float startWorld =
            std::floor(m_camera.GetX() / TILE) * TILE;

        const int columns = static_cast<int>(SCREEN_WIDTH / TILE) + 2;

        for (float y = groundY; y < SCREEN_HEIGHT; y += TILE)
        {
            // 一番上の段だけ草の乗ったマスにする
            const bool topRow = (y < groundY + 1.0f);

            const TileRef& cell = topRow ? topTile : fillTile;

            const float h = std::min(TILE, SCREEN_HEIGHT - y);

            const D2D1_RECT_F source = D2D1::RectF(
                cell.col * TILE,
                cell.row * TILE,
                (cell.col + 1) * TILE,
                cell.row * TILE + TILE * (h / TILE)
            );

            for (int i = 0; i < columns; ++i)
            {
                const float x = ToScreenX(startWorld + i * TILE);

                //--------------------------------------------
                // 等倍で並べるので NEAREST でよい。
                // LINEAR だと隣のマスの色を拾い、
                // マスの境目に線が出てしまう。
                //--------------------------------------------
                m_renderTarget->DrawBitmap(
                    m_terrainBitmap,
                    D2D1::RectF(x, y, x + TILE, y + h),
                    1.0f,
                    D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                    source
                );
            }
        }

        return;
    }

    //--------------------------------------------------------
    // 画像が読めなかったときは無地で塗る（設計書 19-13）
    //--------------------------------------------------------
    SetBrush(0.09f, 0.12f, 0.10f, 0.88f);

    m_renderTarget->FillRectangle(
        D2D1::RectF(0.0f, groundY, SCREEN_WIDTH, SCREEN_HEIGHT),
        m_brush.Get()
    );

    SetBrush(0.35f, 0.80f, 0.65f, 0.55f);

    m_renderTarget->DrawLine(
        D2D1::Point2F(0.0f, groundY),
        D2D1::Point2F(SCREEN_WIDTH, groundY),
        m_brush.Get(),
        2.0f
    );

    //--------------------------------------------------------
    // スクロールしているのが分かるよう、
    // 地面に一定間隔の目印を描く
    //--------------------------------------------------------
    SetBrush(0.45f, 0.60f, 0.52f, 0.35f);

    const float step = 160.0f;
    const float startWorld =
        std::floor(m_camera.GetX() / step) * step;

    for (int i = 0; i <= static_cast<int>(SCREEN_WIDTH / step) + 1; ++i)
    {
        const float x = ToScreenX(startWorld + i * step);

        m_renderTarget->DrawLine(
            D2D1::Point2F(x, groundY + 6.0f),
            D2D1::Point2F(x, groundY + 26.0f),
            m_brush.Get(),
            2.0f
        );
    }

}


void GameScene::DrawEnemies()
{
    const bool highlightWeakPoint =
        m_loadout.GetSynergy().ShouldHighlightWeakPoint();

    const float playerCenterX = m_playerX + m_playerWidth * 0.5f;

    // enemies.png：1セル 61x68、横5種×縦2フレーム
    constexpr float ENEMY_CELL_W = 61.0f;
    constexpr float ENEMY_CELL_H = 68.0f;

    // boss.png：1セル 122x136、横2フレーム
    constexpr float BOSS_CELL_W = 122.0f;
    constexpr float BOSS_CELL_H = 136.0f;

    for (const Enemy& enemy : m_enemies)
    {
        if (!enemy.alive)
        {
            continue;
        }

        // 画面外の敵は描かない
        if (!m_camera.IsVisible(enemy.x, enemy.width))
        {
            continue;
        }

        const bool isBoss = (enemy.kind == EnemyKind::Boss);

        // ワールド座標 → スクリーン座標
        const float ex = ToScreenX(enemy.x);
        const float ecx = ex + enemy.width * 0.5f;

        //----------------------------------------------------
        // 歩行アニメーション
        //----------------------------------------------------
        const int frame =
            (static_cast<int>(enemy.actionTimer * 6.0f) % 2 == 0) ? 0 : 1;

        const D2D1_RECT_F destination = D2D1::RectF(
            ex,
            enemy.y,
            ex + enemy.width,
            enemy.y + enemy.height
        );

        ID2D1Bitmap* bitmap = isBoss
            ? m_bossBitmap
            : m_enemyBitmap;

        if (bitmap)
        {
            D2D1_RECT_F source{};

            if (isBoss)
            {
                source = D2D1::RectF(
                    frame * BOSS_CELL_W, 0.0f,
                    (frame + 1) * BOSS_CELL_W, BOSS_CELL_H
                );
            }
            else
            {
                // Grunt / Runner / Brute / Elite の順で並んでいる
                int column = 0;

                // enemies.png の並び順に対応させる
                switch (enemy.kind)
                {
                case EnemyKind::Grunt:   column = 0; break;
                case EnemyKind::Shooter: column = 1; break;
                case EnemyKind::Runner:  column = 2; break;
                case EnemyKind::Brute:   column = 3; break;
                case EnemyKind::Elite:   column = 4; break;
                default: column = 0; break;
                }

                source = D2D1::RectF(
                    column * ENEMY_CELL_W,
                    frame * ENEMY_CELL_H,
                    (column + 1) * ENEMY_CELL_W,
                    (frame + 1) * ENEMY_CELL_H
                );
            }

            //------------------------------------------------
            // プレイヤーの方を向かせる
            //------------------------------------------------
            const bool facingRight = (playerCenterX >= enemy.CenterX());

            D2D1_MATRIX_3X2_F oldTransform;
            m_renderTarget->GetTransform(&oldTransform);

            if (!facingRight)
            {
                m_renderTarget->SetTransform(
                    D2D1::Matrix3x2F::Scale(
                        D2D1::SizeF(-1.0f, 1.0f),
                        D2D1::Point2F(ecx, enemy.CenterY())
                    ) * oldTransform
                );
            }

            m_renderTarget->DrawBitmap(
                bitmap,
                destination,
                1.0f,
                D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                source
            );

            m_renderTarget->SetTransform(oldTransform);

            //------------------------------------------------
            // 被弾の合図は着弾点のヒットエフェクトで出す。
            // 以前はここで全身を半透明の長方形で覆っていたが、
            // 絵の形と関係ない四角が浮いて見えるためやめた。
            //------------------------------------------------
        }
        else
        {
            //------------------------------------------------
            // 画像が無い場合は図形で代用する
            //------------------------------------------------
            float r = 0.72f, g = 0.26f, b = 0.26f;

            switch (enemy.kind)
            {
            case EnemyKind::Grunt:   r = 0.72f; g = 0.26f; b = 0.26f; break;
            case EnemyKind::Shooter: r = 0.22f; g = 0.72f; b = 0.85f; break;
            case EnemyKind::Runner:  r = 0.90f; g = 0.55f; b = 0.18f; break;
            case EnemyKind::Brute:   r = 0.55f; g = 0.30f; b = 0.74f; break;
            case EnemyKind::Elite:   r = 1.00f; g = 0.74f; b = 0.14f; break;
            case EnemyKind::Boss:    r = 0.95f; g = 0.18f; b = 0.34f; break;
            }

            if (enemy.hitFlash > 0.0f)
            {
                r = std::min(1.0f, r + 0.5f);
                g = std::min(1.0f, g + 0.5f);
                b = std::min(1.0f, b + 0.5f);
            }

            SetBrush(r * 0.6f, g * 0.6f, b * 0.6f, 1.0f);

            m_renderTarget->FillRoundedRectangle(
                D2D1::RoundedRect(destination, 8.0f, 8.0f),
                m_brush.Get()
            );

            SetBrush(r, g, b, 1.0f);

            m_renderTarget->DrawRoundedRectangle(
                D2D1::RoundedRect(destination, 8.0f, 8.0f),
                m_brush.Get(),
                2.5f
            );
        }

        //----------------------------------------------------
        // Redeye 4：弱点表示を強調
        //----------------------------------------------------
        if (highlightWeakPoint)
        {
            const float headRadius = enemy.width * 0.26f;

            SetBrush(1.0f, 0.25f, 0.25f, 0.9f);

            m_renderTarget->DrawEllipse(
                D2D1::Ellipse(
                    D2D1::Point2F(
                        ecx,
                        enemy.y + enemy.height * 0.15f
                    ),
                    headRadius + 4.0f,
                    headRadius + 4.0f
                ),
                m_brush.Get(),
                2.0f
            );

            m_renderTarget->DrawLine(
                D2D1::Point2F(ex, enemy.GetHeadLineY()),
                D2D1::Point2F(ex + enemy.width, enemy.GetHeadLineY()),
                m_brush.Get(),
                1.0f
            );
        }

        // HPは画面下のUIへまとめて出すので、頭上には描かない
    }

    //--------------------------------------------------------
    // ボスHPバーは画面上部に大きく出す
    //--------------------------------------------------------
    for (const Enemy& enemy : m_enemies)
    {
        if (enemy.kind != EnemyKind::Boss || !enemy.alive)
        {
            continue;
        }

        DrawBar(
            360.0f, 32.0f, 580.0f, 20.0f,
            enemy.GetHpRatio(),
            0.95f, 0.16f, 0.32f
        );

        SetBrush(1.0f, 0.85f, 0.85f, 1.0f);

        // バーの左横に置いて重ならないようにする
        DrawTextLine(
            L"ボス",
            268.0f, 30.0f, 356.0f, 56.0f,
            m_normalFormat.Get()
        );

        break;
    }
}


void GameScene::DrawPlayer()
{
    // プレイヤーもワールド座標で持っているので変換する
    const float px = ToScreenX(m_playerX);

    //--------------------------------------------------------
    // 踏み切りエフェクト
    // シートの3行目に入れた3コマを足元へ出す
    //--------------------------------------------------------
    //--------------------------------------------------------
    // ジャンプした直後は、キャラクターそのものが
    // エフェクトの姿になる（色はキャラクターと同じ）
    //--------------------------------------------------------
    if (m_jumpEffectTimer > 0.0f && m_characterBitmap)
    {
        const float t = 1.0f - (m_jumpEffectTimer / 0.30f);

        int frame = static_cast<int>(t * 4.0f);
        frame = std::clamp(frame, 0, 3);

        const D2D1_RECT_F source = D2D1::RectF(
            frame * SPRITE_W, SPRITE_H * 2.0f,
            (frame + 1) * SPRITE_W, SPRITE_H * 3.0f
        );

        const float width = m_playerWidth * 1.25f;
        const float height = m_playerHeight * 1.25f;

        const float centerX = px + m_playerWidth * 0.5f;
        const float centerY = m_playerY + m_playerHeight * 0.5f;

        m_renderTarget->DrawBitmap(
            m_characterBitmap,
            D2D1::RectF(
                centerX - width * 0.5f,
                centerY - height * 0.5f,
                centerX + width * 0.5f,
                centerY + height * 0.5f
            ),
            1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
            source
        );

        // エフェクト中は本体を描かない
        return;
    }

    if (!m_characterBitmap)
    {
        // 画像が無い場合は四角で代用
        SetBrush(0.18f, 0.67f, 1.0f, 1.0f);

        m_renderTarget->FillRectangle(
            D2D1::RectF(
                px,
                m_playerY,
                px + m_playerWidth,
                m_playerY + m_playerHeight
            ),
            m_brush.Get()
        );

        return;
    }

    //--------------------------------------------------------
    // 無敵中は点滅させる
    //--------------------------------------------------------
    if (m_invincibleTimer > 0.0f)
    {
        const int blink =
            static_cast<int>(m_invincibleTimer * 20.0f) % 2;

        if (blink == 0)
        {
            return;
        }
    }

    //--------------------------------------------------------
    // 表示するフレームを選ぶ
    //   1. 空中          → 縮こまったポーズ
    //   2. 射撃直後      → マズルフラッシュ／構え
    //   3. 移動中        → 歩行5フレーム
    //   4. それ以外      → 立ち
    //--------------------------------------------------------
    D2D1_RECT_F source{};

    const bool drawingShoot =
        m_isGrounded && m_shootAnimTimer > 0.0f;

    const auto makeSource = [](int column, int row)
        {
            const float left = column * SPRITE_W;
            const float top = row * SPRITE_H;

            return D2D1::RectF(left, top, left + SPRITE_W, top + SPRITE_H);
        };

    if (!m_isGrounded)
    {
        // 空中では膝を抱えたポーズ
        source = makeSource(JUMP_FRAME_COLUMN, JUMP_FRAME_ROW);
    }
    else if (drawingShoot)
    {
        const float elapsed = SHOOT_ANIM_TIME - m_shootAnimTimer;

        // 撃った瞬間だけ銃を突き出したコマにする
        source = makeSource(
            (elapsed < 0.05f) ? SHOOT_MUZZLE_COLUMN : SHOOT_FRAME_COLUMN,
            SHOOT_FRAME_ROW
        );
    }
    else
    {
        const int frame = m_isMoving ? m_animFrame : 0;

        source = makeSource(frame, 0);
    }

    const float drawWidth = m_playerWidth;

    // 反動で少し後ろへ下がる
    const float recoilShift =
        m_facingRight ? -m_recoilOffset : m_recoilOffset;

    const D2D1_RECT_F destination = D2D1::RectF(
        px + recoilShift,
        m_playerY,
        px + recoilShift + drawWidth,
        m_playerY + m_playerHeight
    );

    const D2D1_POINT_2F center = D2D1::Point2F(
        px + m_playerWidth * 0.5f,
        m_playerY + m_playerHeight * 0.5f
    );

    D2D1_MATRIX_3X2_F oldTransform;
    m_renderTarget->GetTransform(&oldTransform);

    if (!m_facingRight)
    {
        m_renderTarget->SetTransform(
            D2D1::Matrix3x2F::Scale(
                D2D1::SizeF(-1.0f, 1.0f),
                center
            ) * oldTransform
        );
    }

    m_renderTarget->DrawBitmap(
        m_characterBitmap,
        destination,
        1.0f,
        D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
        source
    );

    m_renderTarget->SetTransform(oldTransform);

    //--------------------------------------------------------
    // ダッシュ中の残像
    //--------------------------------------------------------
    if (m_dashTimer > 0.0f)
    {
        SetBrush(0.35f, 0.85f, 1.0f, 0.30f);

        m_renderTarget->FillRoundedRectangle(
            D2D1::RoundedRect(
                D2D1::RectF(
                    px - m_dashDirection * 40.0f,
                    m_playerY + 20.0f,
                    px - m_dashDirection * 40.0f + m_playerWidth,
                    m_playerY + m_playerHeight
                ),
                10.0f, 10.0f
            ),
            m_brush.Get()
        );
    }
}

void GameScene::DrawBullets()
{
    //--------------------------------------------------------
    // bullets.png：1セル 62x40
    //   0:自機の弾  1:敵の弾  3:散弾
    //--------------------------------------------------------
    // bullets.png：1セル 62x40
    //   上段が青（Player1）、下段が緑（Player2）
    constexpr float CELL_W = 62.0f;
    constexpr float CELL_H = 40.0f;

    const float bulletRow =
        (m_playerType == PlayerType::Player1) ? 0.0f : 1.0f;

    for (const Bullet& bullet : m_bullets)
    {
        const float bx = ToScreenX(bullet.x);

        if (bx < -60.0f || bx > SCREEN_WIDTH + 60.0f)
        {
            continue;
        }

        const float speed = std::sqrt(
            bullet.vx * bullet.vx + bullet.vy * bullet.vy
        );

        if (!m_bulletBitmap)
        {
            //------------------------------------------------
            // 画像が無い場合は丸で代用する
            //------------------------------------------------
            if (bullet.fromPlayer)
            {
                SetBrush(1.0f, 0.95f, 0.70f, 1.0f);
            }
            else
            {
                SetBrush(1.0f, 0.35f, 0.45f, 1.0f);
            }

            m_renderTarget->FillEllipse(
                D2D1::Ellipse(
                    D2D1::Point2F(bx, bullet.y),
                    bullet.radius, bullet.radius
                ),
                m_brush.Get()
            );

            continue;
        }

        //----------------------------------------------------
        // 進行方向へ回転させる
        //----------------------------------------------------
        const float angle =
            std::atan2(bullet.vy, bullet.vx) * 180.0f / 3.14159265f;

        // 散弾は小さいコマを使う
        const int cell =
            bullet.fromPlayer
            ? ((bullet.radius <= 4.5f) ? 3 : 0)
            : 1;

        // 敵の弾は色を変えない
        const float row = bullet.fromPlayer ? bulletRow : 0.0f;

        const D2D1_RECT_F source = D2D1::RectF(
            cell * CELL_W, row * CELL_H,
            (cell + 1) * CELL_W, (row + 1.0f) * CELL_H
        );

        const float drawWidth = bullet.radius * 6.0f;
        const float drawHeight = drawWidth * (CELL_H / CELL_W);

        D2D1_MATRIX_3X2_F oldTransform;
        m_renderTarget->GetTransform(&oldTransform);

        m_renderTarget->SetTransform(
            D2D1::Matrix3x2F::Rotation(
                angle,
                D2D1::Point2F(bx, bullet.y)
            ) * oldTransform
        );

        m_renderTarget->DrawBitmap(
            m_bulletBitmap,
            D2D1::RectF(
                bx - drawWidth * 0.5f,
                bullet.y - drawHeight * 0.5f,
                bx + drawWidth * 0.5f,
                bullet.y + drawHeight * 0.5f
            ),
            1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
            source
        );

        m_renderTarget->SetTransform(oldTransform);

        (void)speed;
    }
}


void GameScene::DrawExplosions()
{
    //--------------------------------------------------------
    // 炎そのものは EffectSystem が描く。
    // ここでは効果範囲が分かるよう、輪だけを重ねる。
    //--------------------------------------------------------
    for (const Explosion& explosion : m_explosions)
    {
        const float t = std::clamp(explosion.timer / 0.32f, 0.0f, 1.0f);

        const float ex = ToScreenX(explosion.x);
        const float radius = explosion.radius * (1.25f - t * 0.35f);

        SetBrush(1.0f, 0.85f, 0.35f, t * 0.5f);

        m_renderTarget->DrawEllipse(
            D2D1::Ellipse(D2D1::Point2F(ex, explosion.y), radius, radius),
            m_brush.Get(),
            2.5f
        );
    }
}


//------------------------------------------------------------
// items.png から1セルを取り出して描く
//   0:宝箱(閉) 1:宝箱(開) 2:弾薬箱 3:武器 4:防具
//------------------------------------------------------------
int GameScene::GetItemRow() const
{
    // 装備の絵は操作キャラクターと同じ色にする
    return (m_playerType == PlayerType::Player1) ? 0 : 1;
}

void GameScene::DrawItemSprite(
    int cellIndex,
    float centerX,
    float centerY,
    float size
)
{
    if (!m_itemBitmap)
    {
        return;
    }

    constexpr float CELL = 64.0f;   // items.png は 1セル64・横10列

    const float row = static_cast<float>(GetItemRow());

    const D2D1_RECT_F source = D2D1::RectF(
        cellIndex * CELL,
        row * CELL,
        (cellIndex + 1) * CELL,
        (row + 1.0f) * CELL
    );

    const float half = size * 0.5f;

    m_renderTarget->DrawBitmap(
        m_itemBitmap,
        D2D1::RectF(
            centerX - half,
            centerY - half,
            centerX + half,
            centerY + half
        ),
        1.0f,
        D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
        source
    );
}


void GameScene::DrawChest()
{
    //--------------------------------------------------------
    // chests.png
    //   1セル 96x96、横7コマ × 縦6色
    //
    //   横：0～2 閉じている（ゆっくり切り替えて置物に見せる）
    //       3～5 開く途中
    //       6    開ききって中身が見えている
    //
    //   縦：中身のレアリティ
    //       0 茶 / 1 緑 / 2 青 / 3 赤 / 4 銀 / 5 金
    //--------------------------------------------------------
    constexpr float CHEST_CELL = 96.0f;
    constexpr int CHEST_COLUMNS = 7;

    constexpr float IDLE_FRAME_TIME = 0.45f;
    constexpr float OPEN_FRAME_TIME = 0.10f;

    // 絵を作り直して中身が大きくなったぶん、表示は少し小さくする
    constexpr float DRAW_SIZE = 80.0f;

    for (const TreasureChest& chest : m_chests)
    {
        if (!chest.active)
        {
            continue;
        }

        if (!m_camera.IsVisible(chest.x - 40.0f, 80.0f))
        {
            continue;
        }

        const float cx = ToScreenX(chest.x);

        //----------------------------------------------------
        // 表示するコマを決める
        //----------------------------------------------------
        int column = 0;

        if (chest.opened)
        {
            // 3 → 4 → 5 → 6 と開いて、最後で止まる
            const int step =
                static_cast<int>(chest.animTimer / OPEN_FRAME_TIME);

            column = std::min(3 + step, CHEST_COLUMNS - 1);
        }
        else
        {
            const int step =
                static_cast<int>(chest.animTimer / IDLE_FRAME_TIME) % 3;

            column = step;
        }

        const int row = std::clamp(
            static_cast<int>(chest.displayRarity), 0, 5
        );

        if (m_chestBitmap)
        {
            const D2D1_RECT_F source = D2D1::RectF(
                column * CHEST_CELL, row * CHEST_CELL,
                (column + 1) * CHEST_CELL, (row + 1) * CHEST_CELL
            );

            //------------------------------------------------
            // 絵はセルの下端に合わせてあるので、
            // 描くときも足元を地面に置く
            //------------------------------------------------
            //------------------------------------------------
            // 消える直前は点滅させる
            //------------------------------------------------
            float opacity = 1.0f;

            if (chest.opened && chest.animTimer > 0.6f)
            {
                const int phase =
                    static_cast<int>((chest.animTimer - 0.6f) / 0.10f);

                opacity = (phase % 2 == 0) ? 1.0f : 0.25f;
            }

            m_renderTarget->DrawBitmap(
                m_chestBitmap,
                D2D1::RectF(
                    cx - DRAW_SIZE * 0.5f,
                    chest.y + 26.0f - DRAW_SIZE,
                    cx + DRAW_SIZE * 0.5f,
                    chest.y + 26.0f
                ),
                opacity,
                D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                source
            );

            //------------------------------------------------
            // 開ける前だけ、レアリティ色の光を足す
            //------------------------------------------------
            if (!chest.opened)
            {
                SetRarityBrush(chest.displayRarity, 0.20f);

                m_renderTarget->FillEllipse(
                    D2D1::Ellipse(
                        D2D1::Point2F(cx, chest.y + 8.0f),
                        46.0f, 26.0f
                    ),
                    m_brush.Get()
                );
            }
        }
        else if (m_itemBitmap)
        {
            DrawItemSprite(chest.opened ? 1 : 0, cx, chest.y, 72.0f);
        }

        //----------------------------------------------------
        // 近づいたら操作を出す
        //----------------------------------------------------
        if (chest.opened)
        {
            continue;
        }

        const bool touching = RectsOverlap(
            chest.x - 34.0f, chest.y - 34.0f, 68.0f, 68.0f,
            m_playerX, m_playerY, m_playerWidth, m_playerHeight
        );

        if (touching)
        {
            SetBrush(1.0f, 1.0f, 1.0f, 1.0f);

            DrawTextLine(
                L"F : 開ける",
                cx - 70.0f, chest.y - 78.0f,
                cx + 110.0f, chest.y - 48.0f,
                m_normalFormat.Get()
            );
        }
    }
}


void GameScene::DrawDrops()
{
    for (size_t i = 0; i < m_drops.size(); ++i)
    {
        const WorldDrop& drop = m_drops[i];

        if (!m_camera.IsVisible(drop.x - 34.0f, 68.0f))
        {
            continue;
        }

        const float dx = ToScreenX(drop.x);

        const float bob = std::sin(drop.bobTimer) * 6.0f;

        const float centerY = drop.y + bob;

        const Rarity rarity = drop.item.GetRarity();

        //----------------------------------------------------
        // レアリティ色の光
        //----------------------------------------------------
        SetRarityBrush(rarity, 0.22f);

        m_renderTarget->FillEllipse(
            D2D1::Ellipse(
                D2D1::Point2F(dx, centerY),
                34.0f,
                34.0f
            ),
            m_brush.Get()
        );

        //----------------------------------------------------
        // 本体（武器は四角、防具はひし形っぽく傾けた四角）
        //----------------------------------------------------
        const D2D1_RECT_F box = D2D1::RectF(
            dx - 16.0f,
            centerY - 16.0f,
            dx + 16.0f,
            centerY + 16.0f
        );

        SetRarityBrush(rarity, 1.0f);

        if (m_itemBitmap)
        {
            // レアリティ色の枠を描いてからアイコンを重ねる
            m_renderTarget->DrawRoundedRectangle(
                D2D1::RoundedRect(
                    D2D1::RectF(
                        dx - 24.0f, centerY - 24.0f,
                        dx + 24.0f, centerY + 24.0f
                    ),
                    4.0f, 4.0f
                ),
                m_brush.Get(),
                2.0f
            );

            //--------------------------------------------
            // 武器はメイン／サブ、防具は部位ごとに絵を変える
            //--------------------------------------------
            int cell = 3;

            if (drop.item.isWeapon)
            {
                cell = (drop.item.weapon.data.category == WeaponCategory::Main)
                    ? 3 : 4;
            }
            else
            {
                cell = 5 + static_cast<int>(drop.item.armor.data.slot);
            }

            DrawItemSprite(cell, dx, centerY, 46.0f);
        }
        else if (drop.item.isWeapon)
        {
            m_renderTarget->FillRoundedRectangle(
                D2D1::RoundedRect(box, 3.0f, 3.0f),
                m_brush.Get()
            );
        }
        else
        {
            m_renderTarget->FillRectangle(box, m_brush.Get());
        }

        //----------------------------------------------------
        // レアリティは色だけで示す
        //----------------------------------------------------
        SetRarityBrush(rarity, 0.95f);

        m_renderTarget->FillRoundedRectangle(
            D2D1::RoundedRect(
                D2D1::RectF(dx - 20.0f, centerY - 36.0f,
                            dx + 20.0f, centerY - 30.0f),
                3.0f, 3.0f
            ),
            m_brush.Get()
        );

        //----------------------------------------------------
        // 拾える状態なら操作を出す
        //----------------------------------------------------
        if (static_cast<int>(i) == m_hoveredDrop)
        {
            SetBrush(1.0f, 1.0f, 1.0f, 1.0f);

            DrawTextLine(
                L"F : 装備   G : 捨てる",
                dx - 90.0f,
                centerY + 24.0f,
                dx + 190.0f,
                centerY + 50.0f,
                m_smallFormat.Get()
            );
        }
    }
}

void GameScene::DrawAmmoPickups()
{
    for (const AmmoPickup& pickup : m_ammoPickups)
    {
        if (!m_camera.IsVisible(pickup.x - 24.0f, 48.0f))
        {
            continue;
        }

        const float ax = ToScreenX(pickup.x);

        const float bob = std::sin(pickup.bobTimer) * 5.0f;

        const D2D1_RECT_F box = D2D1::RectF(
            ax - 13.0f,
            pickup.y - 9.0f + bob,
            ax + 13.0f,
            pickup.y + 9.0f + bob
        );

        SetBrush(0.95f, 0.80f, 0.25f, 0.25f);

        m_renderTarget->FillEllipse(
            D2D1::Ellipse(
                D2D1::Point2F(ax, pickup.y + bob),
                22.0f, 22.0f
            ),
            m_brush.Get()
        );

        if (m_itemBitmap)
        {
            DrawItemSprite(2, ax, pickup.y + bob, 44.0f);
        }
        else
        {
            SetBrush(0.85f, 0.68f, 0.20f, 1.0f);

            m_renderTarget->FillRoundedRectangle(
                D2D1::RoundedRect(box, 3.0f, 3.0f),
                m_brush.Get()
            );
        }
    }
}

//------------------------------------------------------------
// HP回復アイテム
//   赤い光を敷いてから救急箱を描く。
//   弾薬（黄色）と一目で見分けられるようにしている。
//------------------------------------------------------------
void GameScene::DrawHealthPickups()
{
    for (const HealthPickup& pickup : m_healthPickups)
    {
        if (!m_camera.IsVisible(pickup.x - 24.0f, 48.0f))
        {
            continue;
        }

        const float ax = ToScreenX(pickup.x);

        const float bob = std::sin(pickup.bobTimer) * 5.0f;
        const float cy = pickup.y + bob;

        //----------------------------------------------------
        // ゆっくり明滅させて目に留まりやすくする
        //----------------------------------------------------
        const float pulse = 0.5f + 0.5f * std::sin(pickup.bobTimer * 1.6f);

        SetBrush(1.0f, 0.30f, 0.32f, 0.16f + pulse * 0.14f);

        m_renderTarget->FillEllipse(
            D2D1::Ellipse(D2D1::Point2F(ax, cy), 26.0f, 26.0f),
            m_brush.Get()
        );

        if (m_healthBitmap)
        {
            const float half = 22.0f;

            m_renderTarget->DrawBitmap(
                m_healthBitmap,
                D2D1::RectF(ax - half, cy - half, ax + half, cy + half),
                1.0f,
                D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR
            );
        }
        else
        {
            //------------------------------------------------
            // 画像が読めないときは赤十字を図形で描く
            //------------------------------------------------
            SetBrush(0.78f, 0.20f, 0.22f, 1.0f);

            m_renderTarget->FillRoundedRectangle(
                D2D1::RoundedRect(
                    D2D1::RectF(ax - 18.0f, cy - 14.0f,
                                ax + 18.0f, cy + 14.0f),
                    4.0f, 4.0f
                ),
                m_brush.Get()
            );

            SetBrush(0.96f, 0.95f, 0.92f, 1.0f);

            m_renderTarget->FillRectangle(
                D2D1::RectF(ax - 4.0f, cy - 10.0f, ax + 4.0f, cy + 10.0f),
                m_brush.Get()
            );

            m_renderTarget->FillRectangle(
                D2D1::RectF(ax - 11.0f, cy - 4.0f, ax + 11.0f, cy + 4.0f),
                m_brush.Get()
            );
        }
    }
}


void GameScene::DrawDamageTexts()
{
    for (const DamageText& text : m_damageTexts)
    {
        const float tx = ToScreenX(text.x);

        if (tx < -80.0f || tx > SCREEN_WIDTH + 80.0f)
        {
            continue;
        }

        const float alpha = std::clamp(text.timer / 0.75f, 0.0f, 1.0f);

        if (text.isHeadShot)
        {
            SetBrush(1.0f, 0.35f, 0.30f, alpha);
        }
        else if (text.isCrit)
        {
            SetBrush(1.0f, 0.85f, 0.25f, alpha);
        }
        else
        {
            SetBrush(1.0f, 1.0f, 1.0f, alpha);
        }

        std::wstring label = FormatFloat(text.value, 0);

        if (text.isHeadShot)
        {
            label += L" ヘッド";
        }
        else if (text.isCrit)
        {
            label += L" 会心";
        }

        DrawTextLine(
            label,
            tx - 60.0f,
            text.y - 14.0f,
            tx + 120.0f,
            text.y + 14.0f,
            text.isCrit || text.isHeadShot
            ? m_normalFormat.Get()
            : m_smallFormat.Get()
        );
    }
}

//============================================================
// UI
//============================================================

//------------------------------------------------------------
// 頭上のお知らせ
//------------------------------------------------------------
void GameScene::DrawWorldMessages()
{
    const float centerX = ToScreenX(m_playerX) + m_playerWidth * 0.5f;

    float y = m_playerY - 34.0f;

    for (size_t i = m_worldMessages.size(); i-- > 0; )
    {
        const WorldMessage& message = m_worldMessages[i];

        const float alpha = std::clamp(message.timer / 0.6f, 0.0f, 1.0f);

        const float top = y + message.offsetY;

        // 背板を敷いて読みやすくする
        SetBrush(0.02f, 0.03f, 0.05f, 0.62f * alpha);

        m_renderTarget->FillRoundedRectangle(
            D2D1::RoundedRect(
                D2D1::RectF(centerX - 190.0f, top, centerX + 190.0f, top + 28.0f),
                6.0f, 6.0f
            ),
            m_brush.Get()
        );

        SetRarityBrush(message.rarity, alpha);

        m_renderTarget->DrawRoundedRectangle(
            D2D1::RoundedRect(
                D2D1::RectF(centerX - 190.0f, top, centerX + 190.0f, top + 28.0f),
                6.0f, 6.0f
            ),
            m_brush.Get(),
            1.4f
        );

        DrawTextLine(
            message.text,
            centerX - 180.0f, top + 3.0f, centerX + 180.0f, top + 27.0f,
            m_smallFormat.Get()
        );

        y -= 32.0f;
    }
}


void GameScene::DrawSynergyPanel()
{
    m_synergyTooltip.clear();

    //--------------------------------------------------------
    // 装備一覧を開いている間だけ、そちらの説明を優先する。
    // 比較パネルを見ている間もシナジーは出したままにする。
    //--------------------------------------------------------
    if (m_showInventory)
    {
        return;
    }

    //--------------------------------------------------------
    // 画面左上には目標の欄があるので、
    // シナジーはその下から始める。
    //--------------------------------------------------------
    m_synergyUI.Draw(
        m_ui,
        m_loadout.GetSynergy(),
        m_loadout.GetMythic(),
        18.0f,
        90.0f,
        m_uiMouseX,
        m_uiMouseY,
        &m_synergyTooltip
    );
}


void GameScene::DrawHUD()
{
    const StageConfig& config = CurrentStage();
    const PlayerStats& stats = m_loadout.GetStats();

    const int index = m_usingMain ? 0 : 1;

    HudData data;

    data.hp = m_hp;
    data.maxHp = stats.maxHP;
    data.shield = m_shield;
    data.defense = stats.defense;

    data.weapon = m_usingMain ? &m_mainEffective : &m_subEffective;
    data.usingMain = m_usingMain;

    data.ammoInMagazine = m_ammoInMagazine[index];
    data.ammoReserve = m_ammoReserve[index];
    data.infiniteReserve = !m_usingMain;   // サブ武器は無限

    data.reloading = m_isReloading;
    data.reloadTimer = m_reloadTimer;

    data.reloadTime =
        (data.weapon && data.weapon->valid) ? data.weapon->reloadTime : 1.0f;

    data.dashCooldown = m_dashCooldown;
    data.dashCooldownMax = DASH_COOLDOWN;

    for (int i = 0; i < 2; ++i)
    {
        if (!m_skills[i])
        {
            continue;
        }

        data.skills[i].name = m_skills[i]->GetName();
        data.skills[i].cooldown = m_skills[i]->GetCooldown();
        data.skills[i].cooldownMax = m_skills[i]->GetCooldownMax();
        data.skills[i].active = m_skills[i]->IsActive();
        data.skills[i].iconIndex = m_skills[i]->GetIconIndex();
        data.skills[i].key = (i == 0) ? L"K" : L"L";
    }

    data.stageIndex = m_stageIndex;
    data.killCount = m_killCount;
    data.killQuota = config.killQuota;
    data.bossStage = config.bossStage;

    data.areaIndex = m_areaIndex;
    data.areaCount = m_areaCount;
    data.areaCleared = m_areaCleared;

    {
        const int limit = GetLevelSetting(m_stageIndex).targetSeconds;

        data.timeLimit = static_cast<float>(limit);
        data.timeLeft = std::max(0.0f, limit - m_stageTime);
    }

    data.objectiveText = m_objective.text;
    data.objectiveProgress =
        FormatObjectiveProgress(m_objective, GetObjectiveProgress());
    data.objectiveCleared = IsObjectiveCleared();

    data.enemyCount = std::min<int>(
        static_cast<int>(m_enemyHpTracks.size()), 5
    );

    for (int i = 0; i < data.enemyCount; ++i)
    {
        data.enemies[i].name = m_enemyHpTracks[i].name;
        data.enemies[i].hp = m_enemyHpTracks[i].hp;
        data.enemies[i].maxHp = m_enemyHpTracks[i].maxHp;
        data.enemies[i].elite = m_enemyHpTracks[i].elite;
    }

    data.score = m_score.GetScore();
    data.combo = m_score.GetCombo();
    data.stageTime = m_stageTime;

    //--------------------------------------------------------
    // マップのどこまで進んだかを記憶
    //--------------------------------------------------------
    data.progress =
        static_cast<float>(m_areaIndex + (m_areaCleared ? 1 : 0)) /
        static_cast<float>(std::max(m_areaCount, 1));

    data.toast = m_toastText;
    data.toastTimer = m_toastTimer;

    data.aimX = m_mouseX;
    data.aimY = m_mouseY;
    data.playerScreenX = ToScreenX(m_playerX) + m_playerWidth * 0.5f;
    data.playerScreenY = m_playerY + m_playerHeight * 0.42f;

    m_gameUI.Draw(m_ui, data, SCREEN_WIDTH, SCREEN_HEIGHT);
}


//------------------------------------------------------------
// 拾える装備の比較パネル
//------------------------------------------------------------
void GameScene::DrawDropPanel()
{
    if (m_hoveredDrop < 0 ||
        m_hoveredDrop >= static_cast<int>(m_drops.size()))
    {
        return;
    }

    m_itemInfoUI.DrawCompare(
        m_ui,
        m_drops[m_hoveredDrop].item,
        m_loadout,
        !m_inventory.IsKnown(m_drops[m_hoveredDrop].item)
    );
}


void GameScene::DrawInventoryPanel()
{
    if (!m_showInventory)
    {
        return;
    }

    m_itemInfoUI.DrawLoadout(
        m_ui, m_loadout, SCREEN_WIDTH, SCREEN_HEIGHT,
        m_uiMouseX, m_uiMouseY, &m_synergyTooltip
    );
}


//------------------------------------------------------------
// ステージ開始／クリア表示
//------------------------------------------------------------
void GameScene::DrawStageBanner(IDWriteTextFormat* titleFormat)
{
    if (!titleFormat)
    {
        return;
    }

    //--------------------------------------------------------
    // ステージ開始
    //--------------------------------------------------------
    if (m_state == GameState::StageIntro)
    {
        const float total = 2.4f;
        const float elapsed = total - m_stateTimer;

        float opacity = 1.0f;

        if (elapsed < 0.4f)
        {
            opacity = elapsed / 0.4f;
        }
        else if (m_stateTimer < 0.5f)
        {
            opacity = m_stateTimer / 0.5f;
        }

        opacity = std::clamp(opacity, 0.0f, 1.0f);

        SetBrush(0.95f, 0.97f, 1.0f, opacity);

        m_renderTarget->DrawText(
            GetStageName(CurrentStage().type),
            static_cast<UINT32>(wcslen(GetStageName(CurrentStage().type))),
            m_bigFormat.Get(),
            D2D1::RectF(0.0f, 250.0f, SCREEN_WIDTH, 400.0f),
            m_brush.Get()
        );

        return;
    }

    //--------------------------------------------------------
    // ステージクリア
    //--------------------------------------------------------
    if (m_state == GameState::StageClear)
    {
        SetBrush(0.0f, 0.0f, 0.0f, 0.45f);

        m_renderTarget->FillRectangle(
            D2D1::RectF(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT),
            m_brush.Get()
        );

        SetBrush(0.45f, 0.95f, 0.60f, 1.0f);

        const wchar_t* text = L"ステージ クリア";

        m_renderTarget->DrawText(
            text,
            static_cast<UINT32>(wcslen(text)),
            m_bigFormat.Get(),
            D2D1::RectF(0.0f, 250.0f, SCREEN_WIDTH, 380.0f),
            m_brush.Get()
        );

        SetBrush(0.90f, 0.95f, 1.0f, 1.0f);

       
    }
}

//------------------------------------------------------------
// リザルト
//------------------------------------------------------------
void GameScene::DrawResultScreen(IDWriteTextFormat* titleFormat)
{
    (void)titleFormat;

    const bool victory = (m_state == GameState::Victory);

    SetBrush(0.0f, 0.0f, 0.0f, 0.80f);

    m_renderTarget->FillRectangle(
        D2D1::RectF(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT),
        m_brush.Get()
    );

    if (victory)
    {
        SetBrush(1.0f, 0.82f, 0.30f, 1.0f);
    }
    else
    {
        SetBrush(0.95f, 0.35f, 0.35f, 1.0f);
    }

    const wchar_t* title = victory ? L"ミッション完了" : L"GAME OVER";

    m_renderTarget->DrawText(
        title,
        static_cast<UINT32>(wcslen(title)),
        m_bigFormat.Get(),
        D2D1::RectF(0.0f, 170.0f, SCREEN_WIDTH, 260.0f),
        m_brush.Get()
    );

    //--------------------------------------------------------
    // 最終ビルドを表示する
    //--------------------------------------------------------
    const SynergySystem& synergy = m_loadout.GetSynergy();

    std::wstring summary;

    {
        wchar_t buffer[160];

        swprintf_s(
            buffer,
            L"到達ステージ %d        撃破数 %d\n\n最終シナジー\n",
            m_stageIndex + 1,
            m_totalKills
        );

        summary = buffer;
    }

    bool anySynergy = false;

    for (int i = 0; i < SYNERGY_COUNT; ++i)
    {
        const SynergyType type = static_cast<SynergyType>(i);
        const int tier = synergy.GetTier(type);

        if (tier <= 0)
        {
            continue;
        }

        anySynergy = true;

        summary += ToString(type);
        summary += L" ";
        summary += std::to_wstring(tier);
        summary += L"        ";
    }

    if (!anySynergy)
    {
        summary += L"（発動なし）";
    }

    SetBrush(0.90f, 0.94f, 1.0f, 1.0f);

    m_headerFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);

    m_renderTarget->DrawText(
        summary.c_str(),
        static_cast<UINT32>(summary.size()),
        m_headerFormat.Get(),
        D2D1::RectF(140.0f, 300.0f, 1140.0f, 500.0f),
        m_brush.Get()
    );

    const wchar_t* hint = L"ENTER : タイトルへ戻る";

    SetBrush(0.60f, 0.70f, 0.80f, 1.0f);

    m_renderTarget->DrawText(
        hint,
        static_cast<UINT32>(wcslen(hint)),
        m_headerFormat.Get(),
        D2D1::RectF(0.0f, 540.0f, SCREEN_WIDTH, 580.0f),
        m_brush.Get()
    );

    m_headerFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
}

//============================================================
// 描画本体
//============================================================

void GameScene::Draw(
    IDWriteTextFormat* titleFormat,
    IDWriteTextFormat* menuFormat
)
{
    (void)menuFormat;

    if (!m_renderTarget)
    {
        return;
    }

    DrawBackground();

    // 奥の飾り → 地面 → 段差と障害物
    DrawProps(false);
    DrawGround();
    DrawTerrain();
    DrawAreaGate();

    DrawChest();
    DrawDrops();
    DrawAmmoPickups();
    DrawHealthPickups();
    DrawEnemies();
    DrawPlayer();
    DrawBullets();
    DrawExplosions();

    // 手前の下草はプレイヤーより後に描く
    DrawProps(true);

    m_effectSystem.Draw(m_renderTarget.Get(), m_camera);
    DrawDamageTexts();
    DrawWorldMessages();

    DrawSynergyPanel();
    DrawHUD();

    // スコアは画面右上
    m_scoreDisplay.Draw(m_ui, m_score, SCREEN_WIDTH);
    DrawDropPanel();

    DrawStageBanner(titleFormat);

    if (m_state == GameState::GameOver ||
        m_state == GameState::Victory)
    {
        DrawResultScreen(titleFormat);
    }

    DrawInventoryPanel();

    //--------------------------------------------------------
    // 説明は最後に描いて、装備一覧の上へ出す
    //--------------------------------------------------------
    if (!m_synergyTooltip.empty())
    {
        //----------------------------------------------------
        // 基本説明のあとに 2 / 4 / 6 の効果が続くので、
        // 5行ぶんの高さを取っている
        //----------------------------------------------------
        constexpr float TIP_TOP = 452.0f;
        constexpr float TIP_BOTTOM = 616.0f;

        m_ui.FillPanel(18.0f, TIP_TOP, SCREEN_WIDTH - 18.0f, TIP_BOTTOM, 8.0f);

        SetBrush(1.0f, 0.90f, 0.45f, 0.85f);

        m_ui.DrawRound(18.0f, TIP_TOP, SCREEN_WIDTH - 18.0f, TIP_BOTTOM,
                       8.0f, 1.4f);

        SetBrush(1.0f, 0.97f, 0.86f, 1.0f);

        m_ui.DrawText(m_synergyTooltip,
                      32.0f, TIP_TOP + 6.0f,
                      SCREEN_WIDTH - 32.0f, TIP_BOTTOM - 6.0f,
                      m_smallFormat.Get());
    }

    m_pauseUI.Draw(m_ui, SCREEN_WIDTH, SCREEN_HEIGHT);
}

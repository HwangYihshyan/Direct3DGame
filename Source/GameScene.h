/*--------------------------------------------------------------------

ファイル名：GameScene.h
お仕事　　：BULLET DRAFT のゲーム本体

・STAGE 1 森林 → 2 郊外 → 3 町 → BOSS の4段構成
・敵を倒す → 装備がドロップ → 拾って装備 → シナジー発動
・設計書 12. のシナジー効果がすべて戦闘へ反映される

----------------------------------------------------------------------*/

#ifndef GAMESCENE_H
#define GAMESCENE_H

#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <chrono>
#include <string>
#include <vector>

#include <memory>

#include "Camera.h"
#include "CommonLevel.h"
#include "DropSystem.h"
#include "Enemy.h"
#include "EffectSystem.h"
#include "EnemySpawner.h"
#include "InputTracker.h"
#include "Inventory.h"
#include "PlayerLoadout.h"
#include "PlayerSkill.h"
#include "ResourceManager.h"
#include "ResultData.h"
#include "GameUI.h"
#include "ItemInfoUI.h"
#include "PauseUI.h"
#include "ScoreDisplay.h"
#include "ShowScore.h"
#include "StageObjective.h"
#include "SynergyUI.h"
#include "UIRenderer.h"
#include "StageMaps.h"

//------------------------------------------------------------
// ゲーム進行状態
//------------------------------------------------------------
enum class GameState
{
    StageIntro,
    Playing,
    StageClear,
    GameOver,
    Victory
};

//------------------------------------------------------------
// 弾
//------------------------------------------------------------
struct Bullet
{
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;

    float traveled = 0.0f;
    float range = 1200.0f;
    float radius = 5.0f;

    bool fromPlayer = true;
    bool alive = true;

    //--------------------------------------------------------
    // 命中時の計算に必要な情報を弾が持ち歩く
    //--------------------------------------------------------
    float baseDamage = 0.0f;

    float critRate = 0.0f;
    float critDamage = 0.0f;

    float headShotMultiplier = 2.0f;
    float headShotBonus = 0.0f;

    float armorPenetration = 0.0f;

    float falloffStart = 0.0f;
    float falloffEnd = 0.0f;
    float falloffMinScale = 1.0f;

    int pierceLeft = 0;

    // 命中時に敵を押し戻す距離（px）。0 なら押さない
    float knockBack = 0.0f;

    //--------------------------------------------------------
    // 何回目の射撃で出た弾か。
    // 散弾は1回の射撃でまとめて出るので、全弾が同じ番号を持つ。
    // ヘッドショットを1射撃につき1回だけ数えるために使う。
    //--------------------------------------------------------
    int shotId = 0;

    // 貫通中に同じ敵へ二重命中しないよう記録
    std::vector<int> hitEnemyIds;
};

//------------------------------------------------------------
// Demolitionの爆発
//------------------------------------------------------------
struct Explosion
{
    float x = 0.0f;
    float y = 0.0f;
    float radius = 90.0f;
    float damage = 0.0f;
    float timer = 0.30f;
    bool damageApplied = false;
};

//------------------------------------------------------------
// ダメージ表示
//------------------------------------------------------------
//------------------------------------------------------------
// 頭上へ出す短いお知らせ（装備した、など）
//------------------------------------------------------------
//------------------------------------------------------------
// 直前に撃った敵のHP表示
//------------------------------------------------------------
struct EnemyHpTrack
{
    int id = 0;
    const wchar_t* name = L"";
    float hp = 0.0f;
    float maxHp = 1.0f;
    bool elite = false;
    float timer = 0.0f;
};

struct WorldMessage
{
    std::wstring text;
    float timer = 0.0f;
    float offsetY = 0.0f;
    Rarity rarity = Rarity::Common;
};

struct DamageText
{
    float x = 0.0f;
    float y = 0.0f;
    float velocityY = -70.0f;
    float timer = 0.8f;
    float value = 0.0f;
    bool isCrit = false;
    bool isHeadShot = false;
};

//------------------------------------------------------------
// 落ちている装備
//------------------------------------------------------------
struct WorldDrop
{
    float x = 0.0f;
    float y = 0.0f;
    float bobTimer = 0.0f;
    bool fromChest = false;
    EquipmentDrop item;
};

//------------------------------------------------------------
// 弾薬ピックアップ
// 設計書 10. LEG の「Ammo Drop」に対応する
//------------------------------------------------------------
struct AmmoPickup
{
    float x = 0.0f;
    float y = 0.0f;
    float bobTimer = 0.0f;
};

//------------------------------------------------------------
// HP回復ピックアップ
//
// 敵を倒すと一定確率で落ちる救急箱。
// 触れるだけで拾える（弾薬と同じ扱い）。
//------------------------------------------------------------
struct HealthPickup
{
    float x = 0.0f;
    float y = 0.0f;
    float bobTimer = 0.0f;

    // 回復量（実HP量）
    float amount = 0.0f;
};

//------------------------------------------------------------
// 障害物の状態
//------------------------------------------------------------
struct ObstacleState
{
    MapObstacle data;

    float hp = 0.0f;
    bool alive = true;
    float hitFlash = 0.0f;

    float GetHpRatio() const
    {
        return (data.maxHp > 0.0f) ? (hp / data.maxHp) : 0.0f;
    }

    float TopY() const { return data.baseY - data.height; }
};

//------------------------------------------------------------
// 宝箱
//------------------------------------------------------------
struct TreasureChest
{
    float x = 0.0f;
    float y = 0.0f;
    bool opened = false;
    bool active = false;

    //--------------------------------------------------------
    // 中身は開ける前に決めておく。
    // 見た目の色を中身のレアリティに合わせるため。
    //--------------------------------------------------------
    std::vector<EquipmentDrop> contents;
    Rarity displayRarity = Rarity::Common;

    //--------------------------------------------------------
    // 開閉アニメーション用の経過時間
    //--------------------------------------------------------
    float animTimer = 0.0f;
};

//============================================================

class GameScene
{
public:
    bool Initialize(
        ID2D1RenderTarget* renderTarget,
        IDWriteFactory* writeFactory
    );

    // タイトルから開始するたびに呼ぶ
    void Reset();

    // ステージと難易度を指定して開始する（LevelSelectSceneから）
    void Reset(int stageIndex, DifficultyType difficulty);

    // キャラクターも指定して開始する
    void Reset(int stageIndex, DifficultyType difficulty, PlayerType player);

    // リザルト用の集計を取り出す
    const ResultData& GetResultData() const { return m_result; }

    bool IsGameOver() const { return m_state == GameState::GameOver; }
    bool IsVictory() const { return m_state == GameState::Victory; }

    // ステージクリアで停止しているか（ResultSceneへ移る合図）
    bool IsStageClearPending() const { return m_stageClearPending; }

    // 次のステージへ進む / 同じステージをやり直す
    void AdvanceStage();
    void RetryStage();

    int GetStageIndex() const { return m_stageIndex; }

    // 残りリトライ回数
    int GetLives() const { return m_lives; }
    bool CanRetry() const { return m_lives > 0; }

    void Update(HWND hwnd);

    void Draw(
        IDWriteTextFormat* titleFormat,
        IDWriteTextFormat* menuFormat
    );

    //--------------------------------------------------------
    // ポーズ画面で「設定」が選ばれたか。
    //
    // ポーズは開いたままなので、設定から戻ると
    // ふたたびポーズ画面が表示される。
    //--------------------------------------------------------
    bool IsOptionRequested() const { return m_optionRequested; }
    void ResetOptionRequest() { m_optionRequested = false; }

    // 設定画面から戻ってきたときに呼ぶ
    void ResumeFromOption();

    //--------------------------------------------------------
    // 被弾直後の強さ（1.0 → 0.0）。
    // 画面全体へ掛ける後処理が使う。
    //--------------------------------------------------------
    float GetDamageFlash() const { return m_damageFlash; }

    bool IsBackToTitle() const;
    void ResetBackRequest();

private:
    //--------------------------------------------------------
    // 初期化
    //--------------------------------------------------------
    bool CreateBrushes();
    bool CreateTextFormats(IDWriteFactory* writeFactory);
    void LoadTextures();

    //--------------------------------------------------------
    // マップ依存の値
    //--------------------------------------------------------
    float GroundY() const;
    float LeftLimit() const;
    float RightLimit() const;

    // ワールド座標 → スクリーン座標
    float ToScreenX(float worldX) const;

    void BuildResultData(bool cleared);

    // スキルの発動と、その結果の適用
    void UseSkill(int slot);
    void ApplySkillActivation(const SkillActivation& activation);

    // 発動中のスキル効果をまとめたもの
    SkillBuff GetSkillBuff() const;

    //--------------------------------------------------------
    // 更新
    //--------------------------------------------------------
    void UpdateMouse(HWND hwnd);

    //--------------------------------------------------------
    // 向きと照準を決める。
    //
    // ポーズ中や装備一覧を開いている間は呼ばない。
    // 呼んでしまうと、メニューの上下移動（W / S）に
    // 照準が反応して動いてしまう。
    //--------------------------------------------------------
    void UpdateAim();

    void UpdatePlayer(float deltaTime);
    void UpdateWeapon(float deltaTime);
    void UpdateBullets(float deltaTime);
    void UpdateEnemies(float deltaTime);
    void UpdateExplosions(float deltaTime);
    void UpdateDrops(float deltaTime);
    void UpdateSpawning(float deltaTime);
    void UpdateEffects(float deltaTime);
    void UpdateStageFlow(float deltaTime);

    //--------------------------------------------------------
    // 戦闘
    //--------------------------------------------------------
    void FireCurrentWeapon();

    void SpawnBullet(
        float angleRadian,
        const EffectiveWeapon& weapon,
        float damageScale
    );

    void StartReload();

    void ApplyDamageToEnemy(
        Enemy& enemy,
        float damage,
        bool isCrit,
        bool isHeadShot
    );

    void OnEnemyKilled(const Enemy& enemy);

    void DamagePlayer(float damage);
    void HealPlayer(float amount);

    void SpawnExplosion(float x, float y, float damage, float radiusScale);
    void SpawnDamageText(float x, float y, float value, bool crit, bool head);

    // 頭上へ短いお知らせを出す
    void ShowWorldMessage(const std::wstring& text, Rarity rarity);

    //--------------------------------------------------------
    // ステージ
    //--------------------------------------------------------
    void BeginStage(int stageIndex);
    void SpawnEnemyWave();

    //--------------------------------------------------------
    // 区画（1画面ぶん）ごとの進行
    //--------------------------------------------------------
    void UpdateArea(float deltaTime);

    float AreaLeft() const;
    float AreaRight() const;
    void SpawnChest();
    void SpawnDropAt(float x, float y, const EquipmentDrop& item, bool fromChest);
    void SpawnAmmoPickup(float x, float y);
    void SpawnHealthPickup(float x, float y, float amount);
    void RefillReserve(float ratio);

    const StageConfig& CurrentStage() const;

    //--------------------------------------------------------
    // 装備
    //--------------------------------------------------------
    int FindTouchingDrop() const;
    void EquipDrop(int dropIndex);
    void RefreshWeapons(bool refillAmmo);

    //--------------------------------------------------------
    // 弾倉を満タンにする。予備弾は減らさない。
    // 制圧射撃の発動時に使う。
    //--------------------------------------------------------
    void RefillMagazine();

    //--------------------------------------------------------
    // 描画
    //--------------------------------------------------------
    void DrawBackground();
    void DrawProps(bool foreground);

    // 背景を進んでいく敵軍
    void DrawMarchingArmy();
    void DrawTerrain();

    // 制圧したときに出す、進む先の線
    void DrawAreaGate();

    // 段差・障害物の上に乗る処理
    void ResolveStanding();

    // 弾が障害物に当たったか
    bool HitObstacle(struct Bullet& bullet);
    // items.png から1セル取り出して描く
    void DrawItemSprite(int cellIndex, float centerX, float centerY, float size);

    // items.png の行（0:Player1の青 1:Player2の緑）
    int GetItemRow() const;
    void DrawGround();
    void DrawEnemies();
    void DrawPlayer();
    void DrawBullets();
    void DrawExplosions();
    void DrawDrops();
    void DrawAmmoPickups();
    void DrawHealthPickups();
    void DrawChest();
    void DrawDamageTexts();
    void DrawWorldMessages();

    void DrawHUD();
    void DrawSynergyPanel();
    void DrawDropPanel();
    void DrawInventoryPanel();
    void DrawStageBanner(IDWriteTextFormat* titleFormat);
    void DrawResultScreen(IDWriteTextFormat* titleFormat);

    // 補助
    void SetBrush(float r, float g, float b, float a = 1.0f);
    void SetRarityBrush(Rarity rarity, float alpha = 1.0f);

    void DrawTextLine(
        const std::wstring& text,
        float left,
        float top,
        float right,
        float bottom,
        IDWriteTextFormat* format
    );

    void DrawBar(
        float x,
        float y,
        float width,
        float height,
        float ratio,
        float r,
        float g,
        float b
    );

    std::wstring BuildItemText(const EquipmentDrop& item) const;
    std::wstring BuildEquippedText(const EquipmentDrop& incoming) const;

private:
    //--------------------------------------------------------
    // 描画リソース
    //--------------------------------------------------------
    Microsoft::WRL::ComPtr<ID2D1RenderTarget> m_renderTarget;
    // 画像は ResourceManager がキャッシュする（設計書 19-11.1）
    ResourceManager m_resources;

    ID2D1Bitmap* m_characterBitmap = nullptr;
    ID2D1Bitmap* m_enemyBitmap = nullptr;
    ID2D1Bitmap* m_bossBitmap = nullptr;
    ID2D1Bitmap* m_itemBitmap = nullptr;
    ID2D1Bitmap* m_bulletBitmap = nullptr;
    ID2D1Bitmap* m_iconBitmap = nullptr;
    ID2D1Bitmap* m_terrainBitmap = nullptr;
    ID2D1Bitmap* m_chestBitmap = nullptr;
    ID2D1Bitmap* m_healthBitmap = nullptr;

    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_brush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_panelBrush;

    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_smallFormat;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_normalFormat;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_headerFormat;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_bigFormat;

    //--------------------------------------------------------
    // 入力
    //--------------------------------------------------------
    InputTracker m_input;

    float m_mouseX = 640.0f;
    float m_mouseY = 360.0f;

    //--------------------------------------------------------
    // プレイヤー
    //--------------------------------------------------------
    float m_playerX = 200.0f;
    float m_playerY = 0.0f;
    float m_playerWidth = 82.0f;
    float m_playerHeight = 92.0f;

    float m_velocityY = 0.0f;
    float m_previousBottom = 0.0f;
    bool m_isGrounded = true;
    bool m_facingRight = true;

    float m_hp = 1000.0f;
    float m_shield = 0.0f;

    float m_invincibleTimer = 0.0f;
    float m_damageFlash = 0.0f;

    // ダッシュ
    float m_dashTimer = 0.0f;
    float m_dashCooldown = 0.0f;
    float m_dashDirection = 1.0f;

    // アニメーション
    float m_animTimer = 0.0f;
    int m_animFrame = 0;
    bool m_isMoving = false;
    float m_shootAnimTimer = 0.0f;

    // 踏み切りエフェクトの経過時間
    float m_jumpEffectTimer = 0.0f;

    float m_aimAngle = 0.0f;
    float m_recoilOffset = 0.0f;

    //--------------------------------------------------------
    // 武器
    //--------------------------------------------------------
    PlayerLoadout m_loadout;
    std::unique_ptr<PlayerSkill> m_skills[2];
    PlayerType m_playerType = PlayerType::Player1;

    bool m_usingMain = true;

    EffectiveWeapon m_mainEffective;
    EffectiveWeapon m_subEffective;

    int m_ammoInMagazine[2] = { 0, 0 };
    int m_ammoReserve[2] = { 0, 0 };

    float m_shotTimer = 0.0f;
    float m_reloadTimer = 0.0f;
    bool m_isReloading = false;

    // バーストライフル用
    int m_burstRemaining = 0;
    float m_burstTimer = 0.0f;

    //--------------------------------------------------------
    // ワールド
    //--------------------------------------------------------
    std::vector<Bullet> m_bullets;
    std::vector<Enemy> m_enemies;
    std::vector<Explosion> m_explosions;
    std::vector<DamageText> m_damageTexts;
    std::vector<WorldMessage> m_worldMessages;
    std::vector<EnemyHpTrack> m_enemyHpTracks;
    std::vector<WorldDrop> m_drops;
    std::vector<AmmoPickup> m_ammoPickups;
    std::vector<HealthPickup> m_healthPickups;
    std::vector<ObstacleState> m_obstacles;

    static constexpr int CHEST_MAX = 3;
    TreasureChest m_chests[CHEST_MAX];

    DropSystem m_dropSystem;

    //--------------------------------------------------------
    // Ver.3 で分離したシステム
    //--------------------------------------------------------
    Camera m_camera;
    std::unique_ptr<MapBase> m_map;
    EnemySpawner m_spawner;
    ShowScore m_score;
    ScoreDisplay m_scoreDisplay;
    EffectSystem m_effectSystem;

    //--------------------------------------------------------
    // UI（設計書 19-6）
    //--------------------------------------------------------
    UIRenderer m_ui;
    GameUI m_gameUI;
    SynergyUI m_synergyUI;
    ItemInfoUI m_itemInfoUI;
    PauseUI m_pauseUI;
    Inventory m_inventory;

    DifficultyType m_difficulty = DifficultyType::Normal;

    // マウスのワールド座標（照準の先）
    float m_mouseWorldX = 640.0f;

    // UI のあたり判定に使う実際のマウス位置
    float m_uiMouseX = -1000.0f;
    float m_uiMouseY = -1000.0f;

    //--------------------------------------------------------
    // リザルト集計
    //--------------------------------------------------------
    ResultData m_result;

    float m_stageTime = 0.0f;

    // 背景の敵軍を動かすための時間
    float m_marchTime = 0.0f;
    float m_damageDealt = 0.0f;
    float m_damageTaken = 0.0f;
    float m_highestDamage = 0.0f;
    int m_shotsFired = 0;
    int m_shotsHit = 0;
    int m_eliteKillCount = 0;
    bool m_tookDamageThisStage = false;

    //--------------------------------------------------------
    // ステージ目標
    //   基準値との差でそのステージぶんだけ数える
    //--------------------------------------------------------
    StageObjective m_objective;

    int m_headShotBase = 0;
    int m_eliteBase = 0;
    int m_pickupBase = 0;
    int m_comboBase = 0;

    int GetObjectiveProgress() const;
    bool IsObjectiveCleared() const;

    int m_nextEnemyId = 1;

    //--------------------------------------------------------
    // ステージ
    //--------------------------------------------------------
    GameState m_state = GameState::StageIntro;

    int m_stageIndex = 0;
    int m_killCount = 0;
    int m_totalKills = 0;

    float m_spawnTimer = 0.0f;
    float m_stateTimer = 0.0f;

    bool m_bossSpawned = false;
    bool m_stageClearPending = false;

    // リトライできる残り回数（3ライフ制）
    static constexpr int LIFE_MAX = 3;
    int m_lives = LIFE_MAX;

    //--------------------------------------------------------
    // 区画進行
    //   敵出現 → 全滅 → 右へ進む → 次の敵出現
    //--------------------------------------------------------
    //--------------------------------------------------------
    // 射撃1回ごとに増やす番号。弾へ配って同じ射撃かどうかを見る。
    //--------------------------------------------------------
    int m_nextShotId = 1;

    //--------------------------------------------------------
    // このフレームで既にヘッドショットを数えた射撃の番号。
    // 散弾が同時に頭へ当たっても、数えるのは1回だけにする。
    //--------------------------------------------------------
    int m_headShotShotId = 0;

    //--------------------------------------------------------
    // ステージクリアの演出を始めてからの秒数。
    // ファンファーレを鳴らし切るまで待つが、
    // 曲が長すぎた場合に備えて上限も見る。
    //--------------------------------------------------------
    float m_stageClearWait = 0.0f;

    //--------------------------------------------------------
    // 段差をすり抜けて下へ降りている最中の残り時間。
    // 0より大きい間は段差と障害物に着地しない。
    //--------------------------------------------------------
    float m_dropThroughTimer = 0.0f;

    int m_areaIndex = 0;
    int m_areaCount = 1;
    bool m_areaCleared = false;
    bool m_waveSpawned = false;
    float m_advanceHintTimer = 0.0f;

    //--------------------------------------------------------
    // UI
    //--------------------------------------------------------
    bool m_showInventory = false;
    int m_hoveredDrop = -1;

    std::wstring m_synergyTooltip;
    std::wstring m_toastText;
    float m_toastTimer = 0.0f;

    bool m_backToTitle = false;
    bool m_optionRequested = false;

    std::chrono::steady_clock::time_point m_lastUpdate;
};

#endif

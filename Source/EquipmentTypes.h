/*--------------------------------------------------------------------

ファイル名：EquipmentTypes.h
お仕事　　：装備システム共通の列挙型定義

設計書　　：Ver.2 「1. 装備システム全体」

----------------------------------------------------------------------*/

#ifndef EQUIPMENTTYPES_H
#define EQUIPMENTTYPES_H

#include <string>

//------------------------------------------------------------
// レアリティ D / C / B / A / S / SS / SSS
// 設計書 5. レアリティ倍率
//------------------------------------------------------------
enum class Rarity
{
    Common,     // D
    Uncommon,   // C
    Rare,       // B
    Special,    // A
    Epic,       // S
    Legendary,  // SS
    Mythic      // SSS
};

constexpr int RARITY_COUNT = 7;

//------------------------------------------------------------
// シナジー10種類
// 設計書 12. シナジー10種類 再調整版
//------------------------------------------------------------
enum class SynergyType
{
    Assault,        // 連続攻撃／攻撃速度
    Hunter,         // Crit
    Demolition,     // 追加爆発
    Guardian,       // 防御／Shield
    Velocity,       // 移動／回避
    Gunner,         // 弾薬／Reload
    Executioner,    // 瀕死の敵への火力
    Vampire,        // HP回復／継戦
    Redeye,         // 弱点／HeadShot
    Fortune         // Drop／高レア装備
};

constexpr int SYNERGY_COUNT = 10;

//------------------------------------------------------------
// ランダムOP
// 設計書 9. 武器ランダムOP / 10. 防具ランダムOP
//------------------------------------------------------------
enum class RandomOptionType
{
    // 武器OP
    AttackPercent,
    CritRate,
    CritDamage,
    FireRate,
    ReloadSpeed,
    MagazineSize,
    HeadShotDamage,
    ArmorPenetration,

    // BODY
    MaxHP,
    Defense,
    DamageReduction,
    Shield,

    // LEG
    ReserveAmmo,
    AmmoDrop,

    // ARM
    RecoilControl,
    WeaponHandling,

    // BOOTS
    MoveSpeed,
    Dodge,
    DashCooldown,
    DashDistance,

    // HEAD
    DetectionRange
};

enum class WeaponCategory
{
    Main,
    Sub
};

//------------------------------------------------------------
// ステージ
// 設計書 Ver.3 19-3.3「System間で共通使用する型を定義」
//------------------------------------------------------------
enum class StageType
{
    Forest = 1,   // STAGE 1 森林
    Suburb = 2,   // STAGE 2 平原
    Town = 3,     // STAGE 3 郊外
    Boss = 4      // BOSS
};

//------------------------------------------------------------
// ドロップ元
//------------------------------------------------------------
enum class DropSource
{
    NormalEnemy,
    EliteEnemy,
    TreasureChest,
    Boss
};

//------------------------------------------------------------
// 武器種
// 設計書 2-1 メイン武器 / 2-2 サブ武器
//------------------------------------------------------------
enum class WeaponType
{
    // MAIN
    AssaultRifle,
    BurstRifle,
    BattleRifle,
    PumpShotgun,
    AutoShotgun,
    ScoutRifle,
    BoltActionRifle,
    AntiMaterialRifle,
    LightMachineGun,
    HeavyMachineGun,

    // SUB
    CombatPistol,
    AutoPistol,
    HeavyRevolver,
    MagnumRevolver,
    CompactSMG,
    TacticalSMG,
    VampireSMG
};

//------------------------------------------------------------
// 発射方式
//------------------------------------------------------------
enum class FireMode
{
    FullAuto,   // 押しっぱなしで連射
    SemiAuto,   // 1クリック1発
    Burst,      // 3点バースト
    Shotgun     // 散弾（1トリガーで複数ペレット）
};

enum class ArmorSlot
{
    Head,
    Body,
    Leg,
    Arm,
    Boots
};

constexpr int ARMOR_SLOT_COUNT = 5;

//------------------------------------------------------------
// 表示用文字列
//------------------------------------------------------------
const wchar_t* ToString(Rarity value);
const wchar_t* ToString(SynergyType value);
const wchar_t* ToString(RandomOptionType value);
const wchar_t* ToString(WeaponType value);
const wchar_t* ToString(ArmorSlot value);
const wchar_t* ToString(StageType value);
const wchar_t* ToString(DropSource value);

// ステージ名（画面表示用）
const wchar_t* GetStageName(StageType stage);

// レアリティの短縮表記（D / C / B / A / S / SS / SSS）
const wchar_t* ToShortString(Rarity value);

// シナジー説明（HUD用の1行）
const wchar_t* GetSynergyThemeText(SynergyType value);

// 発動段階ごとの効果（0なら2発動時の内容を返す）
const wchar_t* GetSynergyTierText(SynergyType value, int tier);

//------------------------------------------------------------
// 説明文をまとめて作る
//   1行目 名前と個数
//   2行目 何を伸ばすシナジーか
//   3行目以降 2 / 4 / 6 発動時の効果
//------------------------------------------------------------
std::wstring BuildSynergyDescription(
    SynergyType value,
    int count,
    int tier
);

//------------------------------------------------------------
// レアリティごとの表示色（0.0f～1.0f）
//------------------------------------------------------------
struct RarityColor
{
    float r;
    float g;
    float b;
};

RarityColor GetRarityColor(Rarity value);

// そのOPが「マイナスの方が良い」OPかどうか（Dash Cooldownのみ）
bool IsReductionOption(RandomOptionType value);

#endif

/*--------------------------------------------------------------------

ファイル名：EquipmentDatabase.cpp
お仕事　　：武器17種／防具5部位のマスターデータ

設計書 2. の★評価を実数値へ変換したもの。
DPSがおおよそ揃うように baseAttack と roundsPerMinute を調整し、
「威力★が高い＝1発が重い代わりに連射が遅い」という関係を作っている。

例：
    Assault Rifle    100 × (600/60) = 1000 DPS
    Bolt Action     1000 × ( 55/60) =  917 DPS ＋ HeadShot ×3.0
    Heavy MG          68 × (900/60) = 1020 DPS ＋ 移動速度 ×0.75

サブ武器はメインの約5割のDPSに抑えている。

----------------------------------------------------------------------*/

#include "EquipmentDatabase.h"

namespace
{
    //--------------------------------------------------------
    // メイン武器
    //--------------------------------------------------------
    void AddMainWeapons(std::vector<WeaponData>& list)
    {
        {
            WeaponData w;
            w.type = WeaponType::AssaultRifle;
            w.category = WeaponCategory::Main;
            w.name = L"アサルトライフル";
            w.feature = L"バランス型";
            w.basicEffect = L"安定した連射";
            w.powerRating = 2; w.fireRateRating = 3; w.reloadRating = 3; w.recoilRating = 3;
            w.baseAttack = 100.0f;
            w.fireMode = FireMode::FullAuto;
            w.roundsPerMinute = 600.0f;
            w.magazineSize = 30;
            w.reserveAmmo = 300;
            w.reloadTime = 2.0f;
            w.spreadDegree = 2.0f;
            w.bulletSpeed = 1500.0f;
            w.bulletRange = 1400.0f;
            w.headShotMultiplier = 2.0f;
            w.recoilKick = 1.0f;
            list.push_back(w);
        }
        {
            WeaponData w;
            w.type = WeaponType::BurstRifle;
            w.category = WeaponCategory::Main;
            w.name = L"バーストライフル";
            w.feature = L"3点バースト";
            w.basicEffect = L"3発目のダメージ +40%";
            w.powerRating = 3; w.fireRateRating = 2; w.reloadRating = 3; w.recoilRating = 3;
            w.baseAttack = 115.0f;
            w.fireMode = FireMode::Burst;
            w.roundsPerMinute = 700.0f;   // バースト内の連射速度
            w.magazineSize = 30;
            w.reserveAmmo = 270;
            w.reloadTime = 2.0f;
            w.spreadDegree = 1.5f;
            w.bulletSpeed = 1550.0f;
            w.bulletRange = 1500.0f;
            w.headShotMultiplier = 2.2f;
            w.recoilKick = 1.1f;
            list.push_back(w);
        }
        {
            WeaponData w;
            w.type = WeaponType::BattleRifle;
            w.category = WeaponCategory::Main;
            w.name = L"バトルライフル";
            w.feature = L"高威力・低連射";
            w.basicEffect = L"攻撃アップ / 敵を1体貫通";
            w.powerRating = 4; w.fireRateRating = 1; w.reloadRating = 3; w.recoilRating = 3;
            w.baseAttack = 230.0f;
            w.fireMode = FireMode::FullAuto;
            w.roundsPerMinute = 260.0f;
            w.magazineSize = 20;
            w.reserveAmmo = 180;
            w.reloadTime = 2.2f;
            w.spreadDegree = 1.2f;
            w.bulletSpeed = 1700.0f;
            w.bulletRange = 1600.0f;
            w.pierceCount = 1;
            w.headShotMultiplier = 2.3f;
            w.recoilKick = 1.6f;
            list.push_back(w);
        }
        {
            WeaponData w;
            w.type = WeaponType::PumpShotgun;
            w.category = WeaponCategory::Main;
            w.name = L"ポンプ式ショットガン";
            w.feature = L"一撃型";
            w.basicEffect = L"近距離ほど高火力";
            w.powerRating = 5; w.fireRateRating = 1; w.reloadRating = 2; w.recoilRating = 4;
            w.baseAttack = 110.0f;        // 1ペレットあたり
            w.fireMode = FireMode::Shotgun;
            w.roundsPerMinute = 70.0f;
            w.magazineSize = 6;
            w.reserveAmmo = 60;
            w.reloadTime = 3.0f;
            w.pelletCount = 8;
            w.spreadDegree = 9.0f;
            w.bulletSpeed = 1200.0f;
            w.bulletRange = 700.0f;
            w.headShotMultiplier = 1.5f;
            w.falloffStart = 200.0f;
            w.falloffEnd = 650.0f;
            w.falloffMinScale = 0.35f;
            w.recoilKick = 3.4f;

            // 1ペレットぶん。8発当たれば約24px押す
            w.knockBack = 3.0f;

            list.push_back(w);
        }
        {
            WeaponData w;
            w.type = WeaponType::AutoShotgun;
            w.category = WeaponCategory::Main;
            w.name = L"オートショットガン";
            w.feature = L"連射型";
            w.basicEffect = L"連続散弾";
            w.powerRating = 4; w.fireRateRating = 1; w.reloadRating = 2; w.recoilRating = 4;
            w.baseAttack = 42.0f;
            w.fireMode = FireMode::Shotgun;
            w.roundsPerMinute = 160.0f;
            w.magazineSize = 8;
            w.reserveAmmo = 80;
            w.reloadTime = 3.2f;
            w.pelletCount = 8;
            w.spreadDegree = 11.0f;
            w.bulletSpeed = 1150.0f;
            w.bulletRange = 640.0f;
            w.headShotMultiplier = 1.5f;
            w.falloffStart = 180.0f;
            w.falloffEnd = 600.0f;
            w.falloffMinScale = 0.35f;
            w.recoilKick = 2.6f;

            // 連射できるぶん、1発あたりは控えめにする
            w.knockBack = 1.8f;

            list.push_back(w);
        }
        {
            WeaponData w;
            w.type = WeaponType::ScoutRifle;
            w.category = WeaponCategory::Main;
            w.name = L"スカウトライフル";
            w.feature = L"軽量";
            w.basicEffect = L"移動速度を落とさない";
            w.powerRating = 4; w.fireRateRating = 1; w.reloadRating = 3; w.recoilRating = 4;
            w.baseAttack = 620.0f;
            w.fireMode = FireMode::SemiAuto;
            w.roundsPerMinute = 90.0f;
            w.magazineSize = 8;
            w.reserveAmmo = 72;
            w.reloadTime = 2.4f;
            w.spreadDegree = 0.6f;
            w.bulletSpeed = 2100.0f;
            w.bulletRange = 1900.0f;
            w.pierceCount = 1;
            w.headShotMultiplier = 2.6f;
            w.moveSpeedScale = 1.0f;      // 軽量：ペナルティなし
            w.recoilKick = 2.4f;
            list.push_back(w);
        }
        {
            WeaponData w;
            w.type = WeaponType::BoltActionRifle;
            w.category = WeaponCategory::Main;
            w.name = L"ボルトアクションライフル";
            w.feature = L"超高威力";
            w.basicEffect = L"ヘッドショット倍率 ×3.0";
            w.powerRating = 4; w.fireRateRating = 2; w.reloadRating = 2; w.recoilRating = 5;
            w.baseAttack = 1000.0f;
            w.fireMode = FireMode::SemiAuto;
            w.roundsPerMinute = 55.0f;
            w.magazineSize = 5;
            w.reserveAmmo = 50;
            w.reloadTime = 2.8f;
            w.spreadDegree = 0.3f;
            w.bulletSpeed = 2300.0f;
            w.bulletRange = 2000.0f;
            w.pierceCount = 1;
            w.headShotMultiplier = 3.0f;
            w.moveSpeedScale = 0.94f;
            w.recoilKick = 4.0f;
            list.push_back(w);
        }
        {
            WeaponData w;
            w.type = WeaponType::AntiMaterialRifle;
            w.category = WeaponCategory::Main;
            w.name = L"対物ライフル";
            w.feature = L"超重量";
            w.basicEffect = L"敵を貫通";
            w.powerRating = 5; w.fireRateRating = 1; w.reloadRating = 2; w.recoilRating = 5;
            w.baseAttack = 1600.0f;
            w.fireMode = FireMode::SemiAuto;
            w.roundsPerMinute = 40.0f;
            w.magazineSize = 4;
            w.reserveAmmo = 36;
            w.reloadTime = 3.4f;
            w.spreadDegree = 0.4f;
            w.bulletSpeed = 2600.0f;
            w.bulletRange = 2200.0f;
            w.pierceCount = 99;           // 全貫通
            w.headShotMultiplier = 2.5f;
            w.moveSpeedScale = 0.85f;
            w.recoilKick = 5.0f;
            list.push_back(w);
        }
        {
            WeaponData w;
            w.type = WeaponType::LightMachineGun;
            w.category = WeaponCategory::Main;
            w.name = L"軽機関銃";
            w.feature = L"高連射";
            w.basicEffect = L"長時間射撃";
            w.powerRating = 4; w.fireRateRating = 4; w.reloadRating = 2; w.recoilRating = 4;
            w.baseAttack = 72.0f;
            w.fireMode = FireMode::FullAuto;
            w.roundsPerMinute = 750.0f;
            w.magazineSize = 75;
            w.reserveAmmo = 450;
            w.reloadTime = 4.2f;
            w.spreadDegree = 3.2f;
            w.bulletSpeed = 1500.0f;
            w.bulletRange = 1400.0f;
            w.headShotMultiplier = 1.8f;
            w.moveSpeedScale = 0.92f;
            w.recoilKick = 1.2f;
            list.push_back(w);
        }
        {
            WeaponData w;
            w.type = WeaponType::HeavyMachineGun;
            w.category = WeaponCategory::Main;
            w.name = L"重機関銃";
            w.feature = L"重火力";
            w.basicEffect = L"移動速度ダウン / 火力アップ";
            w.powerRating = 5; w.fireRateRating = 4; w.reloadRating = 1; w.recoilRating = 5;
            w.baseAttack = 68.0f;
            w.fireMode = FireMode::FullAuto;
            w.roundsPerMinute = 900.0f;
            w.magazineSize = 120;
            w.reserveAmmo = 600;
            w.reloadTime = 5.5f;
            w.spreadDegree = 4.0f;
            w.bulletSpeed = 1450.0f;
            w.bulletRange = 1350.0f;
            w.pierceCount = 1;
            w.headShotMultiplier = 1.7f;
            w.moveSpeedScale = 0.75f;     // 設計書：移動速度DOWN
            w.recoilKick = 1.5f;
            list.push_back(w);
        }
    }

    //--------------------------------------------------------
    // サブ武器
    //--------------------------------------------------------
    void AddSubWeapons(std::vector<WeaponData>& list)
    {
        {
            WeaponData w;
            w.type = WeaponType::CombatPistol;
            w.category = WeaponCategory::Sub;
            w.name = L"コンバットピストル";
            w.feature = L"ハンドガン";
            w.basicEffect = L"取り回しが良い";
            w.powerRating = 1; w.fireRateRating = 1; w.reloadRating = 3; w.recoilRating = 1;
            w.baseAttack = 90.0f;
            w.fireMode = FireMode::SemiAuto;
            w.roundsPerMinute = 320.0f;
            w.magazineSize = 15;
            w.reserveAmmo = 180;
            w.reloadTime = 1.4f;
            w.spreadDegree = 1.6f;
            w.bulletSpeed = 1400.0f;
            w.bulletRange = 1100.0f;
            w.headShotMultiplier = 2.2f;
            w.recoilKick = 0.8f;
            list.push_back(w);
        }
        {
            WeaponData w;
            w.type = WeaponType::AutoPistol;
            w.category = WeaponCategory::Sub;
            w.name = L"オートピストル";
            w.feature = L"高連射ピストル";
            w.basicEffect = L"フルオート射撃";
            w.powerRating = 1; w.fireRateRating = 3; w.reloadRating = 4; w.recoilRating = 2;
            w.baseAttack = 55.0f;
            w.fireMode = FireMode::FullAuto;
            w.roundsPerMinute = 550.0f;
            w.magazineSize = 20;
            w.reserveAmmo = 240;
            w.reloadTime = 1.3f;
            w.spreadDegree = 2.4f;
            w.bulletSpeed = 1350.0f;
            w.bulletRange = 1000.0f;
            w.headShotMultiplier = 2.0f;
            w.recoilKick = 0.7f;
            list.push_back(w);
        }
        {
            WeaponData w;
            w.type = WeaponType::HeavyRevolver;
            w.category = WeaponCategory::Sub;
            w.name = L"リボルバー";
            w.feature = L"高威力リボルバー";
            w.basicEffect = L"一発の威力を重視";
            w.powerRating = 2; w.fireRateRating = 1; w.reloadRating = 3; w.recoilRating = 2;
            w.baseAttack = 190.0f;
            w.fireMode = FireMode::SemiAuto;
            w.roundsPerMinute = 180.0f;
            w.magazineSize = 6;
            w.reserveAmmo = 72;
            w.reloadTime = 1.9f;
            w.spreadDegree = 1.2f;
            w.bulletSpeed = 1600.0f;
            w.bulletRange = 1300.0f;
            w.headShotMultiplier = 2.4f;
            w.recoilKick = 1.8f;
            list.push_back(w);
        }
        {
            WeaponData w;
            w.type = WeaponType::MagnumRevolver;
            w.category = WeaponCategory::Sub;
            w.name = L"マグナム";
            w.feature = L"超高威力リボルバー";
            w.basicEffect = L"高威力・低連射 / 1体貫通";
            w.powerRating = 3; w.fireRateRating = 1; w.reloadRating = 1; w.recoilRating = 3;
            w.baseAttack = 340.0f;
            w.fireMode = FireMode::SemiAuto;
            w.roundsPerMinute = 110.0f;
            w.magazineSize = 6;
            w.reserveAmmo = 60;
            w.reloadTime = 2.6f;
            w.spreadDegree = 1.0f;
            w.bulletSpeed = 1800.0f;
            w.bulletRange = 1500.0f;
            w.pierceCount = 1;
            w.headShotMultiplier = 2.6f;
            w.recoilKick = 2.6f;
            list.push_back(w);
        }
        {
            WeaponData w;
            w.type = WeaponType::CompactSMG;
            w.category = WeaponCategory::Sub;
            w.name = L"コンパクト短機関銃";
            w.feature = L"高速連射";
            w.basicEffect = L"軽量・高機動";
            w.powerRating = 1; w.fireRateRating = 5; w.reloadRating = 5; w.recoilRating = 2;
            w.baseAttack = 32.0f;
            w.fireMode = FireMode::FullAuto;
            w.roundsPerMinute = 1000.0f;
            w.magazineSize = 32;
            w.reserveAmmo = 384;
            w.reloadTime = 1.2f;
            w.spreadDegree = 3.4f;
            w.bulletSpeed = 1300.0f;
            w.bulletRange = 900.0f;
            w.headShotMultiplier = 1.8f;
            w.recoilKick = 0.6f;
            list.push_back(w);
        }
        {
            WeaponData w;
            w.type = WeaponType::TacticalSMG;
            w.category = WeaponCategory::Sub;
            w.name = L"タクティカル短機関銃";
            w.feature = L"安定した短機関銃";
            w.basicEffect = L"連射と精度の両立";
            w.powerRating = 2; w.fireRateRating = 4; w.reloadRating = 5; w.recoilRating = 2;
            w.baseAttack = 42.0f;
            w.fireMode = FireMode::FullAuto;
            w.roundsPerMinute = 850.0f;
            w.magazineSize = 35;
            w.reserveAmmo = 350;
            w.reloadTime = 1.2f;
            w.spreadDegree = 2.6f;
            w.bulletSpeed = 1380.0f;
            w.bulletRange = 1000.0f;
            w.headShotMultiplier = 1.9f;
            w.recoilKick = 0.7f;
            list.push_back(w);
        }
        {
            WeaponData w;
            w.type = WeaponType::VampireSMG;
            w.category = WeaponCategory::Sub;
            w.name = L"ヴァンパイア短機関銃";
            w.feature = L"高速連射";
            w.basicEffect = L"高連射型短機関銃";
            w.powerRating = 2; w.fireRateRating = 5; w.reloadRating = 4; w.recoilRating = 2;
            w.baseAttack = 34.0f;
            w.fireMode = FireMode::FullAuto;
            w.roundsPerMinute = 1000.0f;
            w.magazineSize = 30;
            w.reserveAmmo = 360;
            w.reloadTime = 1.4f;
            w.spreadDegree = 3.0f;
            w.bulletSpeed = 1300.0f;
            w.bulletRange = 900.0f;
            w.headShotMultiplier = 1.8f;
            w.recoilKick = 0.6f;
            list.push_back(w);
        }
    }

    std::vector<WeaponData> CreateWeapons()
    {
        std::vector<WeaponData> list;
        list.reserve(17);

        AddMainWeapons(list);
        AddSubWeapons(list);

        return list;
    }

    //--------------------------------------------------------
    // 防具
    // 設計書 4. 防具シナジー仕様 / 10. 防具ランダムOP
    //--------------------------------------------------------
    std::vector<ArmorData> CreateArmors()
    {
        std::vector<ArmorData> list;

        {
            ArmorData a;
            a.slot = ArmorSlot::Head;
            a.name = L"頭部";
            a.optionPool =
            {
                RandomOptionType::CritRate,
                RandomOptionType::CritDamage,
                RandomOptionType::HeadShotDamage,
                RandomOptionType::DetectionRange
            };
            a.preferredSynergies =
            {
                SynergyType::Redeye,
                SynergyType::Hunter,
                SynergyType::Executioner,
                SynergyType::Fortune
            };
            a.mainStatType = RandomOptionType::CritRate;
            a.mainStatBase = 5.0f;
            a.baseDefense = 10.0f;
            list.push_back(a);
        }
        {
            ArmorData a;
            a.slot = ArmorSlot::Body;
            a.name = L"胴体";
            a.optionPool =
            {
                RandomOptionType::MaxHP,
                RandomOptionType::Defense,
                RandomOptionType::DamageReduction,
                RandomOptionType::Shield
            };
            a.preferredSynergies =
            {
                SynergyType::Guardian,
                SynergyType::Vampire,
                SynergyType::Demolition
            };
            a.mainStatType = RandomOptionType::MaxHP;
            a.mainStatBase = 220.0f;      // 実HP加算
            a.baseDefense = 40.0f;
            list.push_back(a);
        }
        {
            ArmorData a;
            a.slot = ArmorSlot::Leg;
            a.name = L"脚部";
            a.optionPool =
            {
                RandomOptionType::ReserveAmmo,
                RandomOptionType::MagazineSize,
                RandomOptionType::ReloadSpeed,
                RandomOptionType::AmmoDrop
            };
            a.preferredSynergies =
            {
                SynergyType::Gunner,
                SynergyType::Assault,
                SynergyType::Fortune
            };
            a.mainStatType = RandomOptionType::ReserveAmmo;
            a.mainStatBase = 30.0f;       // %
            a.baseDefense = 16.0f;
            list.push_back(a);
        }
        {
            ArmorData a;
            a.slot = ArmorSlot::Arm;
            a.name = L"腕部";
            a.optionPool =
            {
                RandomOptionType::AttackPercent,
                RandomOptionType::FireRate,
                RandomOptionType::RecoilControl,
                RandomOptionType::WeaponHandling
            };
            a.preferredSynergies =
            {
                SynergyType::Assault,
                SynergyType::Gunner,
                SynergyType::Executioner
            };
            a.mainStatType = RandomOptionType::AttackPercent;
            a.mainStatBase = 6.0f;        // %
            a.baseDefense = 14.0f;
            list.push_back(a);
        }
        {
            ArmorData a;
            a.slot = ArmorSlot::Boots;
            a.name = L"ブーツ";
            a.optionPool =
            {
                RandomOptionType::MoveSpeed,
                RandomOptionType::Dodge,
                RandomOptionType::DashCooldown,
                RandomOptionType::DashDistance
            };
            a.preferredSynergies =
            {
                SynergyType::Velocity,
                SynergyType::Hunter,
                SynergyType::Fortune
            };
            a.mainStatType = RandomOptionType::MoveSpeed;
            a.mainStatBase = 5.0f;        // %
            a.baseDefense = 10.0f;
            list.push_back(a);
        }

        return list;
    }

    const std::vector<WeaponData> WEAPONS = CreateWeapons();
    const std::vector<ArmorData>  ARMORS = CreateArmors();
}

const std::vector<WeaponData>& EquipmentDatabase::GetWeapons()
{
    return WEAPONS;
}

const std::vector<ArmorData>& EquipmentDatabase::GetArmorTemplates()
{
    return ARMORS;
}

const WeaponData* EquipmentDatabase::FindWeapon(WeaponType type)
{
    for (const auto& w : WEAPONS)
    {
        if (w.type == type)
        {
            return &w;
        }
    }

    return nullptr;
}

const ArmorData* EquipmentDatabase::FindArmor(ArmorSlot slot)
{
    for (const auto& a : ARMORS)
    {
        if (a.slot == slot)
        {
            return &a;
        }
    }

    return nullptr;
}

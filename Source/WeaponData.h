/*--------------------------------------------------------------------

ファイル名：WeaponData.h
お仕事　　：武器1本分のデータ

設計書 2. 銃のステータス仕様 の★評価を、
実際に戦闘計算で使える数値へ落とし込んだもの。

★評価は powerRating などにそのまま残し、
実数値（baseAttack / roundsPerMinute など）を別に持つ。

----------------------------------------------------------------------*/

#ifndef WEAPONDATA_H
#define WEAPONDATA_H

#include <string>
#include <vector>

#include "EquipmentTypes.h"
#include "RandomOption.h"

struct WeaponData
{
    //--------------------------------------------------------
    // 識別
    //--------------------------------------------------------
    WeaponType type = WeaponType::AssaultRifle;
    WeaponCategory category = WeaponCategory::Main;

    std::wstring name;
    std::wstring feature;       // 特徴
    std::wstring basicEffect;   // 基本効果

    //--------------------------------------------------------
    // 設計書の★評価（1～5）
    //--------------------------------------------------------
    int powerRating = 1;
    int fireRateRating = 1;
    int reloadRating = 1;
    int recoilRating = 1;

    //--------------------------------------------------------
    // 実数値
    //--------------------------------------------------------

    // 1発（散弾なら1ペレット）の基礎攻撃力
    // ここにだけレアリティ倍率を掛ける（設計書 5.）
    float baseAttack = 0.0f;

    FireMode fireMode = FireMode::FullAuto;

    float roundsPerMinute = 600.0f;
    int magazineSize = 30;
    int reserveAmmo = 240;
    float reloadTime = 2.0f;    // 秒

    int pelletCount = 1;        // 散弾数
    float spreadDegree = 2.0f;  // 拡散角
    float bulletSpeed = 1500.0f;
    float bulletRange = 1400.0f;

    // 貫通できる敵の数（0＝貫通なし、99＝全貫通）
    int pierceCount = 0;

    // HeadShot倍率（Bolt Actionなど精密武器は高い）
    float headShotMultiplier = 2.0f;

    // 装備中の移動速度倍率（Heavy Machine Gunは遅い）
    float moveSpeedScale = 1.0f;

    // 距離減衰（ショットガン用）。0で減衰なし
    float falloffStart = 0.0f;
    float falloffEnd = 0.0f;
    float falloffMinScale = 1.0f;

    // 発射ごとの画面反動（描画・エイムブレ用）
    float recoilKick = 1.0f;

    //--------------------------------------------------------
    // 命中したときに敵を押し戻す距離（px）
    //
    // 0 なら押さない。ショットガンにだけ持たせている。
    // 散弾は1ペレットぶんの値なので、
    // 近距離で全弾当たれば ×ペレット数ぶん押される。
    // 離れるほど当たるペレットが減り、押しも弱くなる。
    //--------------------------------------------------------
    float knockBack = 0.0f;

    //--------------------------------------------------------
    // ドロップ時に決まる部分
    //--------------------------------------------------------
    Rarity rarity = Rarity::Common;
    std::vector<RandomOption> randomOptions;
    std::vector<SynergyType> synergies;
};

#endif

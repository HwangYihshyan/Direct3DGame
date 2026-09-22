/*--------------------------------------------------------------------

ファイル名：GameUI.h
お仕事　　：プレイ中のHUD

設計書 Ver.3 19-6.1
    Player HP / Shield / Main・Sub Weapon / Ammo / Reload /
    Score / Stage Progress / Boss HP / Item Popup

UIは表示のみ。値は GameScene から HudData で受け取る。

----------------------------------------------------------------------*/

#ifndef GAMEUI_H
#define GAMEUI_H

#include <string>

#include "PlayerLoadout.h"
#include "UIRenderer.h"

struct HudData
{
    //--------------------------------------------------------
    // 直
    // に撃った敵のHP（最大5体・上から順に並べる）
    //--------------------------------------------------------
    struct EnemyHpView
    {
        const wchar_t* name = L"";
        float hp = 0.0f;
        float maxHp = 1.0f;
        bool elite = false;
    };

    EnemyHpView enemies[5];
    int enemyCount = 0;

    float hp = 0.0f;
    float maxHp = 1.0f;
    float shield = 0.0f;
    float defense = 0.0f;

    // 構えている武器（valid が false なら未装備）
    const EffectiveWeapon* weapon = nullptr;
    bool usingMain = true;

    int ammoInMagazine = 0;
    int ammoReserve = 0;
    bool infiniteReserve = false;

    bool reloading = false;
    float reloadTimer = 0.0f;
    float reloadTime = 1.0f;

    float dashCooldown = 0.0f;
    float dashCooldownMax = 1.0f;

    //--------------------------------------------------------
    // 固有スキル2つ（0:K キー 1:L キー）
    //--------------------------------------------------------
    struct SkillView
    {
        const wchar_t* name = L"";
        float cooldown = 0.0f;
        float cooldownMax = 1.0f;
        bool active = false;
        int iconIndex = 0;
        const wchar_t* key = L"K";
    };

    SkillView skills[2];

    int stageIndex = 0;
    int killCount = 0;
    int killQuota = 1;
    bool bossStage = false;

    // 区画進行
    int areaIndex = 0;
    int areaCount = 1;
    bool areaCleared = false;

    int score = 0;
    int combo = 0;

    float stageTime = 0.0f;

    // 制限時間
    float timeLimit = 0.0f;
    float timeLeft = 0.0f;

    // 進行度（0.0～1.0）
    float progress = 0.0f;

    // ステージ目標
    std::wstring objectiveText;
    std::wstring objectiveProgress;
    bool objectiveCleared = false;

    std::wstring toast;
    float toastTimer = 0.0f;

    // 狙っている先（スクリーン座標）
    float aimX = 640.0f;
    float aimY = 360.0f;
    float playerScreenX = 640.0f;
    float playerScreenY = 360.0f;
};

class GameUI
{
public:
    void Draw(
        const UIRenderer& ui,
        const HudData& data,
        float screenWidth,
        float screenHeight
    ) const;

private:
    void DrawAimMarker(const UIRenderer& ui, const HudData& data) const;
};

#endif

/*--------------------------------------------------------------------

ファイル名：GameUI.cpp
お仕事　　：プレイ中のHUD

配置（設計書 19-6.1 の配置案）
    左下：HP / Shield
    中央下：武器 / Ammo
    右下：Stage Progress / Score

----------------------------------------------------------------------*/
#define NOMINMAX
#include "GameUI.h"

#include <algorithm>
#include <cstdio>

namespace
{
    std::wstring Format(const wchar_t* pattern, float a, float b = 0.0f)
    {
        wchar_t buffer[160];
        swprintf_s(buffer, pattern, a, b);
        return buffer;
    }
}

void GameUI::Draw(
    const UIRenderer& ui,
    const HudData& data,
    float screenWidth,
    float screenHeight
) const
{
    if (!ui.IsReady())
    {
        return;
    }

    (void)screenHeight;

    //--------------------------------------------------------
    // 下部パネル
    //
    //   左        HP / Shield
    //   その右    直前に撃った敵のHP
    //   中央      武器 / 弾数
    //   その右    スキル / ダッシュ
    //   右        ステージと操作
    //--------------------------------------------------------
    ui.FillPanel(18.0f, 626.0f, screenWidth - 18.0f, 710.0f, 10.0f);

    //--------------------------------------------------------
    // HP / Shield
    //--------------------------------------------------------
    ui.DrawBar(
        30.0f, 638.0f, 250.0f, 18.0f,
        data.hp / std::max(data.maxHp, 1.0f),
        0.30f, 0.85f, 0.42f
    );

    if (data.shield > 0.0f)
    {
        ui.DrawBar(
            30.0f, 658.0f, 250.0f, 8.0f,
            data.shield / std::max(data.maxHp, 1.0f),
            0.35f, 0.72f, 1.0f
        );
    }

    ui.SetColor(0.90f, 0.95f, 1.0f, 1.0f);

    {
        wchar_t buffer[128];

        swprintf_s(
            buffer, L"HP %.0f / %.0f",
            data.hp, data.maxHp
        );

        ui.DrawText(buffer, 30.0f, 670.0f, 180.0f, 692.0f, ui.Small());
    }

    //--------------------------------------------------------
    // 防御
    //   武器欄から移してきた。HPの隣のほうが意味も近い。
    //--------------------------------------------------------
    {
        wchar_t buffer[64];

        swprintf_s(buffer, L"防御 %.0f", data.defense);

        ui.SetColor(0.72f, 0.80f, 0.90f, 1.0f);

        ui.DrawText(buffer, 190.0f, 670.0f, 290.0f, 692.0f, ui.Small());
    }

    //--------------------------------------------------------
    // 敵のHP
    // 当ててから3秒だけ出る。複数いれば上から順に並べる。
    //--------------------------------------------------------
    {
        constexpr float ENEMY_LEFT = 300.0f;
        constexpr float ENEMY_WIDTH = 210.0f;
        constexpr float ROW = 15.0f;

        float y = 632.0f;

        for (int i = 0; i < data.enemyCount && i < 5; ++i)
        {
            const HudData::EnemyHpView& enemy = data.enemies[i];

            if (enemy.elite)
            {
                ui.SetColor(1.00f, 0.80f, 0.20f, 1.0f);
            }
            else
            {
                ui.SetColor(0.82f, 0.86f, 0.92f, 1.0f);
            }

            ui.DrawText(
                enemy.name,
                ENEMY_LEFT, y - 1.0f, ENEMY_LEFT + 84.0f, y + 14.0f,
                ui.Small()
            );

            ui.DrawBar(
                ENEMY_LEFT + 86.0f, y + 2.0f, ENEMY_WIDTH - 86.0f, 9.0f,
                enemy.hp / std::max(enemy.maxHp, 1.0f),
                0.95f, 0.30f, 0.30f
            );

            y += ROW;
        }
    }

    //--------------------------------------------------------
    // 武器
    //
    //   使える横幅は 522 ～ 836。右はスキルの枠が始まる。
    //   文字は折り返しが有効なので、はみ出すと勝手に2行になり、
    //   下の行と重なってしまう。
    //   3行それぞれが1行で収まる幅に収めている。
    //
    //     1行目  武器名だけ（22px・最長264px）
    //            「ボルトアクションライフル」でも折り返さない
    //     2行目  どちらの枠か ＋ 弾数（18px・最長288px）
    //     3行目  攻撃力とクリティカル率（18px・最長279px）
    //
    //   防御は武器ではなく体の値なので、左のHP欄へ移した。
    //   3行目へ入れると横幅が足りず、折り返してしまう。
    //--------------------------------------------------------
    if (data.weapon && data.weapon->valid)
    {
        constexpr float WEAPON_LEFT = 522.0f;
        constexpr float WEAPON_RIGHT = 836.0f;

        const EffectiveWeapon& weapon = *data.weapon;

        //----------------------------------------------------
        // 1行目：武器名。レアリティの色で出す。
        //----------------------------------------------------
        ui.SetRarityColor(weapon.rarity, 1.0f);

        ui.DrawText(weapon.name,
                    WEAPON_LEFT, 628.0f, WEAPON_RIGHT, 654.0f,
                    ui.Normal());

        //----------------------------------------------------
        // 2行目：どちらの枠か ＋ 弾数
        //----------------------------------------------------
        wchar_t buffer[160];

        const wchar_t* slot = data.usingMain ? L"[1] メイン" : L"[2] サブ";

        if (data.reloading)
        {
            swprintf_s(buffer, L"%s  リロード中...  %.1f秒",
                       slot, data.reloadTimer);
        }
        else if (data.infiniteReserve)
        {
            swprintf_s(buffer, L"%s  %d / %d  予備 ∞",
                       slot, data.ammoInMagazine, weapon.magazineSize);
        }
        else
        {
            swprintf_s(buffer, L"%s  %d / %d  予備 %d",
                       slot, data.ammoInMagazine, weapon.magazineSize,
                       data.ammoReserve);
        }

        ui.SetColor(0.88f, 0.92f, 0.98f, 1.0f);

        ui.DrawText(buffer,
                    WEAPON_LEFT, 656.0f, WEAPON_RIGHT, 678.0f,
                    ui.Small());

        //----------------------------------------------------
        // 3行目：リロード中はゲージ、そうでなければ性能
        //----------------------------------------------------
        if (data.reloading && data.reloadTime > 0.0f)
        {
            ui.DrawBar(
                WEAPON_LEFT, 686.0f, 220.0f, 6.0f,
                1.0f - data.reloadTimer / data.reloadTime,
                1.0f, 0.75f, 0.25f
            );
        }
        else
        {
            ui.SetColor(0.70f, 0.78f, 0.85f, 1.0f);

            swprintf_s(buffer, L"攻撃 %.0f   クリティカル率 %.0f%%",
                       weapon.attack,
                       weapon.critRate * 100.0f);

            ui.DrawText(buffer,
                        WEAPON_LEFT, 680.0f, WEAPON_RIGHT, 702.0f,
                        ui.Small());
        }
    }

    //--------------------------------------------------------
    // 固有スキル（K / L）
    //--------------------------------------------------------
    for (int i = 0; i < 2; ++i)
    {
        const HudData::SkillView& skill = data.skills[i];

        if (skill.name == nullptr || skill.name[0] == L'\0')
        {
            continue;
        }

        const float x = 846.0f + i * 122.0f;
        const bool ready = (skill.cooldown <= 0.0f);

        ui.DrawSkillIcon(skill.iconIndex, x, 632.0f, 36.0f,
                         ready ? 1.0f : 0.4f);

        if (skill.active)
        {
            ui.SetColor(1.0f, 0.88f, 0.35f, 0.95f);
            ui.DrawRound(x - 3.0f, 629.0f, x + 39.0f, 671.0f, 8.0f, 2.0f);
        }

        if (skill.active)      ui.SetColor(1.00f, 0.88f, 0.35f, 1.0f);
        else if (ready)        ui.SetColor(0.60f, 0.96f, 0.75f, 1.0f);
        else                   ui.SetColor(0.55f, 0.60f, 0.66f, 1.0f);

        std::wstring label = L"[";
        label+= skill.key;
        label+= L"]";

        if (!ready)
        {
            wchar_t buffer[24];
            swprintf_s(buffer, L" %.0f秒", skill.cooldown);
            label += buffer;
        }

        ui.DrawText(label, x + 40.0f, 634.0f, x + 118.0f, 656.0f, ui.Small());

        ui.DrawBar(
            x + 40.0f, 658.0f, 74.0f, 6.0f,
            ready ? 1.0f
                  : 1.0f - skill.cooldown / std::max(skill.cooldownMax, 0.01f),
            skill.active ? 1.0f : 0.45f, 0.88f, 0.45f
        );
    }

    //--------------------------------------------------------
    // ダッシュ
    //--------------------------------------------------------
    ui.SetColor(0.70f, 0.78f, 0.85f, 1.0f);
    ui.DrawText(L"[Shift]", 846.0f, 676.0f, 908.0f, 700.0f, ui.Small());

    ui.DrawBar(
        910.0f, 682.0f, 140.0f, 8.0f,
        (data.dashCooldown <= 0.0f)
            ? 1.0f
            : 1.0f - data.dashCooldown / std::max(data.dashCooldownMax, 0.01f),
        0.35f, 0.80f, 1.0f
    );

    //--------------------------------------------------------
    // ステージと操作
    //--------------------------------------------------------
    ui.SetColor(0.95f, 0.95f, 1.0f, 1.0f);

    {
        wchar_t buffer[128];

        if (data.bossStage)
        {
            swprintf_s(buffer, L"最終 - ボス");
        }
        else
        {
            swprintf_s(buffer, L"ステージ %d  エリア %d / %d",
                       data.stageIndex + 1, data.areaIndex + 1, data.areaCount);
        }

        ui.DrawText(buffer, 1090.0f, 632.0f, screenWidth - 24.0f, 656.0f,
                    ui.Small());
    }

    ui.SetColor(0.55f, 0.62f, 0.70f, 1.0f);

    ui.DrawText(L"TAB : 装備\nEsc : メニュー",
                1090.0f, 658.0f, screenWidth - 24.0f, 706.0f, ui.Small());

    //--------------------------------------------------------
    // ステージ目標
    //--------------------------------------------------------
    if (!data.objectiveText.empty())
    {
        //----------------------------------------------------
        // 枠と文字の置き場所
        //
        // 文字は 18px。全角は約18px、半角は約9px 送る。
        // 折り返しが有効なので、枠が狭いと勝手に2行になって
        // 下の行と重なってしまう。1行ぶんの幅を必ず確保する。
        //
        //   ラベル「ボーナス」  全角4文字 ＝ 約72px
        //   目標の文言          最長で約235px
        //     （「ヘッドショットを24回決める」）
        //   進捗                最長で約101px
        //----------------------------------------------------
        constexpr float PANEL_LEFT = 18.0f;
        constexpr float PANEL_RIGHT = 500.0f;
        constexpr float PANEL_TOP = 28.0f;
        constexpr float PANEL_BOTTOM = 82.0f;

        // ラベルと目標の文言を置く1行目
        constexpr float LINE1_TOP = 31.0f;
        constexpr float LINE1_BOTTOM = 55.0f;

        // 進捗を置く2行目
        constexpr float LINE2_TOP = 55.0f;
        constexpr float LINE2_BOTTOM = 79.0f;

        constexpr float LABEL_LEFT = 30.0f;
        constexpr float LABEL_RIGHT = 118.0f;   // 72px の文字に対して余裕を持たせる

        constexpr float TEXT_LEFT = 126.0f;
        constexpr float TEXT_RIGHT = 488.0f;    // 362px。最長の文言でも折り返さない

        ui.FillPanel(PANEL_LEFT, PANEL_TOP, PANEL_RIGHT, PANEL_BOTTOM, 8.0f);

        if (data.objectiveCleared)
        {
            ui.SetColor(0.45f, 0.95f, 0.60f, 0.9f);
        }
        else
        {
            ui.SetColor(0.85f, 0.72f, 0.30f, 0.7f);
        }

        ui.DrawRound(PANEL_LEFT, PANEL_TOP, PANEL_RIGHT, PANEL_BOTTOM, 8.0f, 1.3f);

        ui.SetColor(0.72f, 0.78f, 0.86f, 1.0f);

        ui.DrawText(L"ボーナス：",
                    LABEL_LEFT, LINE1_TOP, LABEL_RIGHT, LINE1_BOTTOM,
                    ui.Small());

        ui.SetColor(0.92f, 0.95f, 1.0f, 1.0f);

        ui.DrawText(data.objectiveText,
                    TEXT_LEFT, LINE1_TOP, TEXT_RIGHT, LINE1_BOTTOM,
                    ui.Small());

        if (data.objectiveCleared)
        {
            ui.SetColor(0.55f, 1.00f, 0.70f, 1.0f);
        }
        else
        {
            ui.SetColor(0.80f, 0.84f, 0.90f, 1.0f);
        }

        ui.DrawText(data.objectiveProgress,
                    LABEL_LEFT, LINE2_TOP, TEXT_RIGHT, LINE2_BOTTOM,
                    ui.Small());
    }

    //--------------------------------------------------------
    // 残り時間
    //   少なくなると赤くなる
    //--------------------------------------------------------
    if (data.timeLimit > 0.0f)
    {
        const bool hurry = (data.timeLeft <= 30.0f);

        ui.FillPanel(540.0f, 28.0f, 740.0f, 66.0f, 8.0f);

        if (hurry)
        {
            ui.SetColor(1.00f, 0.35f, 0.32f, 0.9f);
        }
        else
        {
            ui.SetColor(0.35f, 0.62f, 0.88f, 0.7f);
        }

        ui.DrawRound(540.0f, 28.0f, 740.0f, 66.0f, 8.0f, 1.3f);

        wchar_t buffer[48];

        const int left = static_cast<int>(data.timeLeft);

        swprintf_s(buffer, L"TIME  %d:%02d", left / 60, left % 60);

        if (hurry)
        {
            ui.SetColor(1.00f, 0.72f, 0.66f, 1.0f);
        }
        else
        {
            ui.SetColor(0.90f, 0.94f, 1.0f, 1.0f);
        }

        ui.DrawText(buffer, 552.0f, 32.0f, 732.0f, 62.0f, ui.Normal());
    }

    //--------------------------------------------------------
    // ステージ進行バー
    //--------------------------------------------------------
    ui.DrawBar(430.0f, 10.0f, 420.0f, 8.0f, data.progress,
               0.45f, 0.75f, 0.95f);

    //--------------------------------------------------------
    // 区画を制圧したら、右へ進むよう促す
    //--------------------------------------------------------
    if (data.areaCleared)
    {
        ui.SetColor(0.55f, 1.00f, 0.70f, 0.95f);

        ui.DrawText(
            L"  　    　　　→　右へ進め  →",
            screenWidth * 0.5f - 260.0f, 78.0f,
            screenWidth * 0.5f + 260.0f, 116.0f,
            ui.Header()
        );
    }

    //--------------------------------------------------------
    // 通知
    //--------------------------------------------------------
    if (data.toastTimer > 0.0f && !data.toast.empty())
    {
        ui.SetColor(1.0f, 0.90f, 0.45f, std::min(data.toastTimer, 1.0f));
        ui.DrawText(data.toast, 340.0f, 548.0f, 940.0f, 582.0f, ui.Normal());
    }

    DrawAimMarker(ui, data);
}

//------------------------------------------------------------
// 照準
//
// マウスを使わないので、プレイヤーから狙う方向へ
// 点線を伸ばして示す。
//------------------------------------------------------------
void GameUI::DrawAimMarker(
    const UIRenderer& ui,
    const HudData& data
) const
{
    ui.SetColor(1.0f, 0.95f, 0.55f, 0.55f);

    const float dx = data.aimX - data.playerScreenX;
    const float dy = data.aimY - data.playerScreenY;

    constexpr int DOTS = 7;

    for (int i = 2; i <= DOTS; ++i)
    {
        const float t = static_cast<float>(i) / DOTS;

        const float x = data.playerScreenX + dx * t;
        const float y = data.playerScreenY + dy * t;

        ui.FillRect(x - 2.0f, y - 2.0f, x + 2.0f, y + 2.0f);
    }

    ui.SetColor(1.0f, 0.90f, 0.35f, 0.9f);

    ui.DrawRound(
        data.aimX - 9.0f, data.aimY - 9.0f,
        data.aimX + 9.0f, data.aimY + 9.0f,
        9.0f, 2.0f
    );
}

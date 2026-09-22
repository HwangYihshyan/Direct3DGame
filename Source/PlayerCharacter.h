/*--------------------------------------------------------------------

ファイル名：PlayerCharacter.h
お仕事　　：操作キャラクター4種類の基礎データ

設計書 Ver.3 19-2.3 ～ 19-2.6
    Player1 近距離向け  HP120 Speed80  Attack90  Defense90
    Player2 遠距離向け  HP90  Speed100 Attack120 Defense70
    Player3 / Player4   将来追加枠（共通Interfaceを維持する）

設計書の数値は相対値なので、
実際のゲーム内数値へ換算した値をここへ持たせる。

----------------------------------------------------------------------*/

#ifndef PLAYERCHARACTER_H
#define PLAYERCHARACTER_H

#include <string>

enum class PlayerType
{
    Player1,
    Player2,
    Player3,
    Player4
};

constexpr int PLAYER_TYPE_COUNT = 4;

struct PlayerCharacterData
{
    PlayerType type = PlayerType::Player1;

    const wchar_t* name = L"";
    const wchar_t* role = L"";
    const wchar_t* description = L"";

    //--------------------------------------------------------
    // 基礎ステータス
    //--------------------------------------------------------
    float baseHP = 1000.0f;
    float baseDefense = 100.0f;

    // 全武器へ乗る攻撃力の下駄
    float baseAttackPercent = 0.0f;

    // 移動速度の倍率
    float baseMoveSpeedPercent = 0.0f;

    float baseCritRate = 0.05f;

    //--------------------------------------------------------
    // 見た目
    //--------------------------------------------------------
    const wchar_t* spriteSheet = L"Assets\\texture\\player_sheet.png";

    //--------------------------------------------------------
    // 固有スキル
    //--------------------------------------------------------
    // K キー（1つ目）
    const wchar_t* skillName = L"";
    const wchar_t* skillDescription = L"";
    float skillCooldown = 14.0f;
    float skillDuration = 5.0f;

    // L キー（2つ目・クールダウンは長め）
    const wchar_t* skillName2 = L"";
    const wchar_t* skillDescription2 = L"";
    float skillCooldown2 = 22.0f;
    float skillDuration2 = 5.0f;

    // 実装済みかどうか（Player3 / 4 は将来枠）
    bool available = false;
};

const PlayerCharacterData& GetPlayerCharacter(PlayerType type);

const wchar_t* ToString(PlayerType type);

#endif

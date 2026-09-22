/*--------------------------------------------------------------------

ファイル名：PlayerCharacter.cpp
お仕事　　：操作キャラクター4種類の基礎データ

設計書の相対値（HP120 / Speed80 など）を、
基準を 100 = ゲーム内 1000HP として換算している。

    Player1  HP120 → 1200
    Player2  HP 90 →  900

Defense も同様に 100 = 100 として扱う。

----------------------------------------------------------------------*/

#include "PlayerCharacter.h"

namespace
{
    PlayerCharacterData MakePlayer1()
    {
        PlayerCharacterData d;

        d.type = PlayerType::Player1;
        d.name = L"重装兵";
        d.role = L"防御特化型";
        d.description =
            L"HPと防御が高く、前に出て戦える。\n"
            L"ショットガンやマシンガンと相性が良い。";

        d.baseHP = 1200.0f;
        d.baseDefense = 90.0f;
        d.baseAttackPercent = -0.10f;   // 攻撃はやや低い
        d.baseMoveSpeedPercent = -0.10f;
        d.baseCritRate = 0.05f;

        d.spriteSheet = L"Assets\\texture\\player_sheet.png";

        d.skillName = L"衝撃波";
        d.skillDescription =
            L"周囲へ衝撃波。敵をノックバックさせダメージを与える。\n"; 
        d.skillCooldown = 9.0f;
        d.skillDuration = 0.4f;

        d.skillName2 = L"弾幕障壁";
        d.skillDescription2 =
            L"5秒間 無敵になる。あらゆるダメージを受けない。\n";
        d.skillCooldown2 = 34.0f;
        d.skillDuration2 = 5.0f;

        d.available = true;

        return d;
    }

    PlayerCharacterData MakePlayer2()
    {
        PlayerCharacterData d;

        d.type = PlayerType::Player2;
        d.name = L"突撃兵";
        d.role = L"攻撃特化型";
        d.description =
            L"攻撃力と機動力が高いが打たれ弱い。\n"
            L"スナイパーライフルやライフルと相性が良い。";

        d.baseHP = 900.0f;
        d.baseDefense = 70.0f;
        d.baseAttackPercent = 0.20f;
        d.baseMoveSpeedPercent = 0.10f;
        d.baseCritRate = 0.08f;

        d.spriteSheet = L"Assets\\texture\\player2_sheet.png";

        d.skillName = L"制圧射撃";
        d.skillDescription =
            L"発動と同時に弾倉を満タンにする。\n"
            L"10秒間、攻撃力 +30% ／ 連射速度 +45%。\n"
            L"そのあいだ弾薬を消費しない。\n"
            L"上昇量は強襲シナジーの数だけ伸びる。\n";
        d.skillCooldown = 26.0f;
        d.skillDuration = 10.0f;

        d.skillName2 = L"瞬間移動";
        d.skillDescription2 =
            L"L ＋ 方向キーで瞬間移動する。\n";
        d.skillCooldown2 = 20.0f;
        d.skillDuration2 = 0.2f;

        d.available = true;

        return d;
    }

    PlayerCharacterData MakePlayer3()
    {
        PlayerCharacterData d;

        d.type = PlayerType::Player3;
        d.name = L"防壁兵";
        d.role = L"防衛型（将来追加）";
        d.description = L"未実装のキャラクター枠です。";

        d.baseHP = 1400.0f;
        d.baseDefense = 120.0f;
        d.baseAttackPercent = -0.20f;
        d.baseMoveSpeedPercent = -0.18f;

        d.skillName = L"守護領域";
        d.skillDescription = L"最大HPの25%のシールドを張る。";
        d.skillCooldown = 18.0f;
        d.skillDuration = 6.0f;
        d.skillName2 = L"要塞";
        d.skillDescription2 = L"未実装。";

        d.available = false;

        return d;
    }

    PlayerCharacterData MakePlayer4()
    {
        PlayerCharacterData d;

        d.type = PlayerType::Player4;
        d.name = L"死神兵";
        d.role = L"吸収型（将来追加）";
        d.description = L"未実装のキャラクター枠です。";

        d.baseHP = 1000.0f;
        d.baseDefense = 80.0f;
        d.baseAttackPercent = 0.10f;
        d.baseMoveSpeedPercent = 0.05f;

        d.skillName = L"血の契約";
        d.skillDescription = L"8秒間、与ダメージの8%をHPへ変換する。";
        d.skillCooldown = 20.0f;
        d.skillDuration = 8.0f;
        d.skillName2 = L"紅の儀";
        d.skillDescription2 = L"未実装。";

        d.available = false;

        return d;
    }

    const PlayerCharacterData PLAYERS[PLAYER_TYPE_COUNT] =
    {
        MakePlayer1(),
        MakePlayer2(),
        MakePlayer3(),
        MakePlayer4()
    };
}

const PlayerCharacterData& GetPlayerCharacter(PlayerType type)
{
    int index = static_cast<int>(type);

    if (index < 0 || index >= PLAYER_TYPE_COUNT)
    {
        index = 0;
    }

    return PLAYERS[index];
}

const wchar_t* ToString(PlayerType type)
{
    return GetPlayerCharacter(type).name;
}

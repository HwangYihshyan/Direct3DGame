/*--------------------------------------------------------------------

ファイル名：AudioManager.h
お仕事　　：BGM と SE の再生（設計書 19-2.12 Sound）

XAudio2 で鳴らし、mp3 の展開は Media Foundation に任せている。
外部ライブラリは使わないので、追加の DLL は要らない。

方針（19-13 共通エラー処理と同じ考え方）：
    ・初期化に失敗しても、すべての関数が「何もしない」だけになる。
    ・音声ファイルが1つも無くてもゲームは普通に動く。
      （BGM は後から Assets\sound へ置けばそのまま鳴る）

BGM はステージごとに MapBase が持っている。
    StageMaps.cpp の m_bgmPath を参照。

----------------------------------------------------------------------*/

#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <string>

//------------------------------------------------------------
// 効果音の種類
//   実ファイルは AudioManager.cpp の SE_PATH 表で対応付ける。
//------------------------------------------------------------
enum class SeId
{
    Shot,            // 弾発射
    HeadShot,        // ヘッドショット
    Dash,            // ダッシュ
    EnemyHit,        // 敵とぶつかる
    Break,           // 障害物を壊す
    Chest,           // 宝箱を開ける
    Heal,            // HP回復アイテムを拾う
    SkillAssault,    // 突撃兵の K スキル
    SkillAssaultL,   // 突撃兵の L スキル
    SkillHeavy,      // 重装兵の K スキル
    SkillHeavyL,     // 重装兵の L スキル
    MenuMove,        // 選択項目を動かす
    MenuDecide,      // 選択項目の決定
    MenuBack,        // 戻る
    SceneChange,     // 画面（背景）の切り替え

    Count
};

//------------------------------------------------------------
// BGM のファイルパス
//   ステージ用は StageMaps.cpp で設定している。
//------------------------------------------------------------
namespace BgmPath
{
    inline const wchar_t* Title = L"Assets\\sound\\bgm_title.mp3";
    inline const wchar_t* Stage1 = L"Assets\\sound\\bgm_stage1.mp3";
    inline const wchar_t* Stage2 = L"Assets\\sound\\bgm_stage2.mp3";
    inline const wchar_t* Stage3 = L"Assets\\sound\\bgm_stage3.mp3";
    inline const wchar_t* Boss = L"Assets\\sound\\bgm_boss.mp3";

    //--------------------------------------------------------
    // ここから下は1回だけ鳴らすもの。
    // 短い曲なので、繰り返すとしつこくなる。
    // Audio::PlayJingle で鳴らすこと。
    //--------------------------------------------------------
    // ゴールラインを越えた瞬間。
    // リザルトへ移る直前に Audio::StopBgm() で止める。
    inline const wchar_t* StageClear = L"Assets\\sound\\bgm_stageclear.mp3";

    inline const wchar_t* Result = L"Assets\\sound\\bgm_result.mp3";
    inline const wchar_t* GameOver = L"Assets\\sound\\bgm_gameover.mp3";
    inline const wchar_t* GameClear = L"Assets\\sound\\bgm_gameclear.mp3";
}

namespace Audio
{
    // 失敗しても false を返すだけで、以降の呼び出しは無視される
    bool Initialize();

    void Shutdown();

    //--------------------------------------------------------
    // 音量（0～100）。OptionScene の値をそのまま渡す。
    //--------------------------------------------------------
    void SetVolume(int master, int bgm, int se);

    //--------------------------------------------------------
    // BGM（ループ再生）
    //   同じパスを渡したときは鳴らし直さず、そのまま流し続ける。
    //--------------------------------------------------------
    void PlayBgm(const std::wstring& path);

    //--------------------------------------------------------
    // 1回だけ鳴らす短い曲
    //
    //   ステージクリアやゲームオーバーのファンファーレ用。
    //   繰り返さず、最後まで鳴ったらそのまま無音になる。
    //   音量は BGM 側の設定に従う。
    //
    //   PlayBgm と同じ枠を使うので、鳴らすと今のBGMは止まる。
    //   同じパスでも、呼ぶたびに頭から鳴らし直す。
    //--------------------------------------------------------
    void PlayJingle(const std::wstring& path);

    //--------------------------------------------------------
    // PlayJingle で鳴らした曲が、まだ鳴っている最中か。
    //
    //   ループ中のBGMは対象外なので false を返す。
    //   曲が読めなかったときも false になるので、
    //   「鳴り終わるまで待つ」処理が止まらない。
    //--------------------------------------------------------
    bool IsJinglePlaying();

    void StopBgm();

    //--------------------------------------------------------
    // SE
    //   同じ音が重なっても良いように、1音ごとに数本の再生枠を持つ。
    //--------------------------------------------------------
    void PlaySe(SeId id);

    void PlaySe(const std::wstring& path);
}

#endif

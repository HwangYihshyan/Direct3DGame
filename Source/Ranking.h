/*--------------------------------------------------------------------

ファイル名：Ranking.h
お仕事　　：スコアランキングの保存と読み込み

ランキングは ranking.txt へ保存する。
1行につき「名前,スコア,到達ステージ」の形式。

----------------------------------------------------------------------*/

#ifndef RANKING_H
#define RANKING_H

#include <string>
#include <vector>

struct RankingEntry
{
    std::wstring name;
    int score = 0;
    int stageIndex = 0;
    bool cleared = false;

    // 直前に登録したものかどうか（表示で目立たせる）
    bool highlight = false;
};

class Ranking
{
public:
    // 名前に使える最大文字数
    static constexpr int NAME_MAX = 16;

    // 残す件数
    static constexpr int ENTRY_MAX = 10;

    void Load(const std::string& path = "ranking.txt");
    bool Save(const std::string& path = "ranking.txt") const;

    // 追加して並べ替える。入った順位を返す（入らなければ -1）
    int Add(const RankingEntry& entry);

    const std::vector<RankingEntry>& GetEntries() const { return m_entries; }

    // そのスコアが載るかどうか
    bool WillEnter(int score) const;

    void ClearHighlight();

private:
    std::vector<RankingEntry> m_entries;
};

#endif

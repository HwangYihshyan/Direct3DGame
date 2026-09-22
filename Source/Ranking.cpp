/*--------------------------------------------------------------------

ファイル名：Ranking.cpp
お仕事　　：スコアランキングの保存と読み込み

名前に「,」や改行が入ると行が壊れるので、
保存する前に取り除いている。

----------------------------------------------------------------------*/

#include "Ranking.h"

#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <fstream>
#include <sstream>

namespace
{
    //--------------------------------------------------------
    // ファイルは UTF-8 で読み書きする。
    //
    // 以前は 128 未満の文字だけを通していたが、
    // 名前の既定値を日本語にしたため
    // そのままでは保存した時点で消えてしまう。
    // Windows の変換を使って UTF-8 と往復させる。
    //--------------------------------------------------------
    std::string ToNarrow(const std::wstring& text)
    {
        // 行が壊れるので「,」と改行だけ先に落とす
        std::wstring cleaned;

        for (wchar_t c : text)
        {
            if (c == L',' || c == L'\n' || c == L'\r')
            {
                continue;
            }

            cleaned += c;
        }

        if (cleaned.empty())
        {
            return std::string();
        }

        const int size = WideCharToMultiByte(
            CP_UTF8, 0,
            cleaned.c_str(), static_cast<int>(cleaned.size()),
            nullptr, 0, nullptr, nullptr
        );

        if (size <= 0)
        {
            return std::string();
        }

        std::string out(static_cast<size_t>(size), '\0');

        WideCharToMultiByte(
            CP_UTF8, 0,
            cleaned.c_str(), static_cast<int>(cleaned.size()),
            out.data(), size, nullptr, nullptr
        );

        return out;
    }

    std::wstring ToWide(const std::string& text)
    {
        if (text.empty())
        {
            return std::wstring();
        }

        const int size = MultiByteToWideChar(
            CP_UTF8, 0,
            text.c_str(), static_cast<int>(text.size()),
            nullptr, 0
        );

        if (size <= 0)
        {
            return std::wstring();
        }

        std::wstring out(static_cast<size_t>(size), L'\0');

        MultiByteToWideChar(
            CP_UTF8, 0,
            text.c_str(), static_cast<int>(text.size()),
            out.data(), size
        );

        return out;
    }
}

void Ranking::Load(const std::string& path)
{
    m_entries.clear();

    std::ifstream file(path);

    if (!file)
    {
        // まだ1度も遊んでいない場合はここに来る
        return;
    }

    std::string line;

    while (std::getline(file, line) &&
           static_cast<int>(m_entries.size()) < ENTRY_MAX)
    {
        std::istringstream stream(line);

        std::string name;
        std::string score;
        std::string stage;
        std::string cleared;

        if (!std::getline(stream, name, ',')) continue;
        if (!std::getline(stream, score, ',')) continue;
        if (!std::getline(stream, stage, ',')) stage = "0";
        if (!std::getline(stream, cleared, ',')) cleared = "0";

        RankingEntry entry;

        entry.name = ToWide(name);

        try
        {
            entry.score = std::stoi(score);
            entry.stageIndex = std::stoi(stage);
            entry.cleared = (std::stoi(cleared) != 0);
        }
        catch (...)
        {
            continue;
        }

        m_entries.push_back(entry);
    }

    std::sort(
        m_entries.begin(), m_entries.end(),
        [](const RankingEntry& a, const RankingEntry& b)
        {
            return a.score > b.score;
        }
    );
}

bool Ranking::Save(const std::string& path) const
{
    std::ofstream file(path, std::ios::trunc);

    if (!file)
    {
        return false;
    }

    for (const RankingEntry& entry : m_entries)
    {
        file << ToNarrow(entry.name) << ','
             << entry.score << ','
             << entry.stageIndex << ','
             << (entry.cleared ? 1 : 0) << '\n';
    }

    return true;
}

bool Ranking::WillEnter(int score) const
{
    if (static_cast<int>(m_entries.size()) < ENTRY_MAX)
    {
        return true;
    }

    return score > m_entries.back().score;
}

void Ranking::ClearHighlight()
{
    for (RankingEntry& entry : m_entries)
    {
        entry.highlight = false;
    }
}

int Ranking::Add(const RankingEntry& entry)
{
    ClearHighlight();

    RankingEntry added = entry;

    added.highlight = true;

    if (added.name.empty())
    {
        added.name = L"プレイヤー";
    }

    if (static_cast<int>(added.name.size()) > NAME_MAX)
    {
        added.name.resize(NAME_MAX);
    }

    m_entries.push_back(added);

    std::stable_sort(
        m_entries.begin(), m_entries.end(),
        [](const RankingEntry& a, const RankingEntry& b)
        {
            return a.score > b.score;
        }
    );

    if (static_cast<int>(m_entries.size()) > ENTRY_MAX)
    {
        m_entries.resize(ENTRY_MAX);
    }

    for (size_t i = 0; i < m_entries.size(); ++i)
    {
        if (m_entries[i].highlight)
        {
            return static_cast<int>(i);
        }
    }

    // 上位から押し出された
    return -1;
}

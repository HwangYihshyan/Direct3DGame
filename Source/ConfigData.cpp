/*--------------------------------------------------------------------

ファイル名：ConfigData.cpp
お仕事　　：設定ファイルの入出力

「key=value」を1行ずつ並べただけの単純な形式にしている。
壊れた行は読み飛ばし、既定値のまま進む。

----------------------------------------------------------------------*/

#include "ConfigData.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

namespace
{
    int ClampVolume(int value)
    {
        return std::clamp(value, 0, 100);
    }
}

float OptionData::GetEffectiveBgm() const
{
    return (masterVolume / 100.0f) * (bgmVolume / 100.0f);
}

float OptionData::GetEffectiveSe() const
{
    return (masterVolume / 100.0f) * (seVolume / 100.0f);
}

bool ConfigData::Load(OptionData& out, const std::string& path)
{
    std::ifstream file(path);

    if (!file)
    {
        // ファイルが無いのは異常ではない。既定値のまま進む
        return false;
    }

    std::string line;

    while (std::getline(file, line))
    {
        const size_t separator = line.find('=');

        if (separator == std::string::npos)
        {
            continue;
        }

        const std::string key = line.substr(0, separator);
        const std::string value = line.substr(separator + 1);

        try
        {
            if (key == "master")          out.masterVolume = ClampVolume(std::stoi(value));
            else if (key == "bgm")        out.bgmVolume = ClampVolume(std::stoi(value));
            else if (key == "se")         out.seVolume = ClampVolume(std::stoi(value));
            else if (key == "fullscreen") out.fullscreen = (std::stoi(value) != 0);
            else if (key == "width")      out.screenWidth = std::stoi(value);
            else if (key == "height")     out.screenHeight = std::stoi(value);
        }
        catch (...)
        {
            // 数値でない行は読み飛ばす
            continue;
        }
    }

    return true;
}

bool ConfigData::Save(const OptionData& data, const std::string& path)
{
    std::ofstream file(path, std::ios::trunc);

    if (!file)
    {
        return false;
    }

    file << "master=" << data.masterVolume << "\n";
    file << "bgm=" << data.bgmVolume << "\n";
    file << "se=" << data.seVolume << "\n";
    file << "fullscreen=" << (data.fullscreen ? 1 : 0) << "\n";
    file << "width=" << data.screenWidth << "\n";
    file << "height=" << data.screenHeight << "\n";

    return true;
}

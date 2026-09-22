/*--------------------------------------------------------------------

ファイル名：ConfigData.h
お仕事　　：音量・画面設定の保存と読み込み

設計書 Ver.3 19-10.1
    Master / BGM / SE Volume、Fullscreen、Resolution
    形式は初期段階では txt で可

----------------------------------------------------------------------*/

#ifndef CONFIGDATA_H
#define CONFIGDATA_H

#include <string>

struct OptionData
{
    int masterVolume = 80;
    int bgmVolume = 80;
    int seVolume = 80;

    bool fullscreen = false;

    int screenWidth = 1280;
    int screenHeight = 720;

    // Master を掛けた実効音量（0.0～1.0）
    float GetEffectiveBgm() const;
    float GetEffectiveSe() const;
};

class ConfigData
{
public:
    // 読み込みに失敗しても既定値で動く
    static bool Load(OptionData& out, const std::string& path = "config.txt");

    static bool Save(const OptionData& data, const std::string& path = "config.txt");
};

#endif

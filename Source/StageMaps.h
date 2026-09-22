/*--------------------------------------------------------------------

ファイル名：Map1.h / Map2.h / Map3.h 相当
お仕事　　：3ステージ分のマップ定義

設計書 Ver.3 19-2.14 ～ 19-2.16
    Map1 森林（Forest.png）
    Map2 平原（Plain.png）
    Map3 郊外（Surberb.png）

3ファイルに分けても中身がほぼ同じ構造になるため、
1つのヘッダへまとめてクラスだけ分けている。

----------------------------------------------------------------------*/

#ifndef STAGEMAPS_H
#define STAGEMAPS_H

#include <memory>

#include "MapBase.h"

//------------------------------------------------------------
// STAGE 1 森林
//------------------------------------------------------------
class Map1 : public MapBase
{
public:
    void Build() override;
    StageType GetStageType() const override { return StageType::Forest; }
    const wchar_t* GetDisplayName() const override { return L"ステージ1 - 森林"; }
};

//------------------------------------------------------------
// STAGE 2 平原
//------------------------------------------------------------
class Map2 : public MapBase
{
public:
    void Build() override;
    StageType GetStageType() const override { return StageType::Suburb; }
    const wchar_t* GetDisplayName() const override { return L"ステージ2 - 平原"; }
};

//------------------------------------------------------------
// STAGE 3 郊外
//------------------------------------------------------------
class Map3 : public MapBase
{
public:
    void Build() override;
    StageType GetStageType() const override { return StageType::Town; }
    const wchar_t* GetDisplayName() const override { return L"ステージ3 - 郊外"; }
};

//------------------------------------------------------------
// ボスステージ（郊外の最奥）
//------------------------------------------------------------
class MapBoss : public MapBase
{
public:
    void Build() override;
    StageType GetStageType() const override { return StageType::Boss; }
    const wchar_t* GetDisplayName() const override { return L"最終 - ボス"; }
};

// ステージ番号（0～3）からマップを作る
std::unique_ptr<MapBase> CreateMap(int stageIndex);

#endif

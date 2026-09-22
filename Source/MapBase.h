/*--------------------------------------------------------------------

ファイル名：MapBase.h
お仕事　　：ステージマップの共通定義

設計書 Ver.3 19-2.14 / 19-2.15 / 19-2.16
    Map1 森林 / Map2 平原 / Map3 郊外

各マップは
    ・ワールド幅
    ・地面の高さ
    ・背景画像と視差
    ・敵の湧き位置
    ・宝箱の設置位置
    ・ゴール位置
を持つ。描画APIには依存しない。

----------------------------------------------------------------------*/

#ifndef MAPBASE_H
#define MAPBASE_H

#include <string>
#include <vector>

#include "EquipmentTypes.h"
#include "EquipmentGenerator.h"

//------------------------------------------------------------
// 背景1層ぶんの情報
//------------------------------------------------------------
struct BackgroundLayer
{
    std::wstring texturePath;

    // 0.0 で固定、1.0 で地面と同速
    float parallax = 0.5f;

    // 画像を上下いっぱいに引き伸ばすときの高さ倍率
    float heightScale = 1.0f;

    // 縦方向のずらし（負で上へ）
    float offsetY = 0.0f;

    //--------------------------------------------------------
    // この層を描く画面上の範囲（0.0＝画面上端、1.0＝下端）
    // 上下で別々の速度にするために使う。
    //--------------------------------------------------------
    float bandTop = 0.0f;
    float bandBottom = 1.0f;

    //--------------------------------------------------------
    // true なら画面の下端に合わせて置く（近景用）
    //--------------------------------------------------------
    bool anchorBottom = false;
};

//------------------------------------------------------------
// terrain_*.png の何番目のマスを使うか
//   1マス96角、横12列×縦9行。
//   col / row が負なら「画像を使わない」の意味。
//------------------------------------------------------------
struct TileRef
{
    int col = -1;
    int row = -1;

    bool IsValid() const { return col >= 0 && row >= 0; }
};

//------------------------------------------------------------
// マップに置く飾り（木・柵・建物など）
//
// tile を指定すると terrain_*.png から切り出して描く。
// 指定しなかった場合は kind に応じた図形で描く（保険）。
//------------------------------------------------------------
struct MapProp
{
    float worldX = 0.0f;
    float baseY = 0.0f;
    float width = 60.0f;
    float height = 120.0f;

    // 見た目の種類（0:木 1:岩 2:柵 3:建物 4:街灯）
    int kind = 0;

    float parallax = 1.0f;

    TileRef tile;

    // 左右反転して置く（同じ絵を並べても単調にならない）
    bool flip = false;

    //--------------------------------------------------------
    // true ならプレイヤーより手前に描く。
    // 足元の草などを手前に置くと奥行きが出る。
    //--------------------------------------------------------
    bool foreground = false;
};

//------------------------------------------------------------
// 段差（乗れる足場）
//------------------------------------------------------------
struct MapPlatform
{
    float worldX = 0.0f;
    float topY = 0.0f;     // この高さに乗れる
    float width = 200.0f;
    float height = 40.0f;
};

//------------------------------------------------------------
// 障害物（耐久値あり）
//   壊れるまで弾を止める。上に乗ることもできる。
//------------------------------------------------------------
struct MapObstacle
{
    float worldX = 0.0f;
    float baseY = 0.0f;    // 下端
    float width = 90.0f;
    float height = 90.0f;
    float maxHp = 800.0f;
    int kind = 0;          // 0:木箱 1:コンクリート 2:ドラム缶

    // 使う絵。未指定ならステージ既定の絵になる。
    TileRef tile;
};

//------------------------------------------------------------
// 宝箱を置く位置
//   足場の上へ置けるよう、高さも持たせている。
//------------------------------------------------------------
struct MapChestPoint
{
    float worldX = 0.0f;

    // 箱の足元。0 以下ならステージの地面へ置く。
    float baseY = 0.0f;
};

class MapBase
{
public:
    virtual ~MapBase() = default;

    // マップを構築する
    virtual void Build() = 0;

    virtual StageType GetStageType() const = 0;
    virtual const wchar_t* GetDisplayName() const = 0;

    float GetWorldWidth() const { return m_worldWidth; }
    float GetGroundY() const { return m_groundY; }

    float GetLeftLimit() const { return m_leftLimit; }
    float GetRightLimit() const { return m_worldWidth - m_leftLimit; }

    // ゴール地点（ワールド座標）
    float GetGoalX() const { return m_goalX; }

    // 段差・障害物・飾りに使うアトラス（1セル96・横12列×縦9行）
    const std::wstring& GetTerrainTexture() const { return m_terrainTexture; }

    // 足場の天板と中詰めに使うマス
    const TileRef& GetPlatformTopTile() const { return m_platformTopTile; }
    const TileRef& GetPlatformFillTile() const { return m_platformFillTile; }

    // このステージで流す BGM（設計書 19-2.12）
    const std::wstring& GetBgmPath() const { return m_bgmPath; }

    const std::vector<BackgroundLayer>& GetLayers() const { return m_layers; }
    const std::vector<MapProp>& GetProps() const { return m_props; }

    const std::vector<MapPlatform>& GetPlatforms() const { return m_platforms; }
    const std::vector<MapObstacle>& GetObstacles() const { return m_obstacles; }

    const std::vector<float>& GetSpawnPoints() const { return m_spawnPoints; }
    const std::vector<MapChestPoint>& GetChestPoints() const { return m_chestPoints; }

    // ワールド内へ収める
    float ClampX(float x, float width) const;

protected:
    float m_worldWidth = 3840.0f;
    float m_groundY = 620.0f;
    float m_leftLimit = 40.0f;
    float m_goalX = 3700.0f;

    std::wstring m_terrainTexture;
    std::wstring m_bgmPath;

    // 既定は 0列目＝草の乗った天板 / 1列目＝土の中詰め
    TileRef m_platformTopTile{ 0, 0 };
    TileRef m_platformFillTile{ 1, 0 };

    std::vector<BackgroundLayer> m_layers;
    std::vector<MapProp> m_props;
    std::vector<MapPlatform> m_platforms;
    std::vector<MapObstacle> m_obstacles;
    std::vector<float> m_spawnPoints;
    std::vector<MapChestPoint> m_chestPoints;
};

#endif

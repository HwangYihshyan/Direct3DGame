/*--------------------------------------------------------------------

ファイル名：StageMaps.cpp
お仕事　　：Map1 森林 / Map2 平原 / Map3 郊外 / Boss の中身

背景は遠景・中景・近景の3層。
それぞれ速度を変えることで奥行きを出している。

    遠景  parallax 0.12  かすませた空と山
    中景  parallax 0.42  ステージの主となる絵
    近景  parallax 0.88  下端に置く手前の草木

背景と BGM はステージごとに切り替える。

    STAGE 1 森林  bg_forest_*  / bgm_stage1.mp3
    STAGE 2 平原  bg_plain_*   / bgm_stage2.mp3
    STAGE 3 郊外  bg_suburb_*  / bgm_stage3.mp3
    FINAL   BOSS  bg_suburb_*  / bgm_boss.mp3

BOSS は「郊外の最奥」という位置づけなので郊外の背景を使う。

近景は上端が透明になっているので、
中景との境目が線として見えない。
どの層も左右反転しながら並べるので、途切れずに続く。


----------------------------------------------------------------------
レベルデザインの決まりごと
----------------------------------------------------------------------

・地形も飾りも terrain_*.png（96角・横12列×縦9行）から切り出す。
  マスの指定は AddProp / AddObstacle の col / row。

・段差は必ず96の倍数。AddPlatform が段数から高さを計算するので、
  中途半端な高さは作れないようにしてある。

・ジャンプで登れる高さは約129。
  つまり1段（96）は登れるが、2段（192）は一度に登れない。
  高い足場へは必ず1段ずつの踏み台を隣に置くこと。

・1区画（1280＝1画面）ごとに役割を持たせている。

    ひらけた場所   撃ち合う
    遮蔽物の集まり 隠れて撃ち返す
    登れる段差     高所を取る。上に宝箱を置いて登る理由を作る
    見せ場         そのステージらしい大道具を置く

----------------------------------------------------------------------*/

#include "StageMaps.h"

#include <algorithm>

#include "AudioManager.h"

namespace
{
    constexpr float GROUND_Y = 620.0f;

    // 1マスの大きさ
    constexpr float TILE = 96.0f;

    // 敵の湧き位置を等間隔で並べる
    void MakeSpawnPoints(
        std::vector<float>& out,
        float from,
        float to,
        int count
    )
    {
        out.clear();

        if (count <= 1)
        {
            out.push_back((from + to) * 0.5f);
            return;
        }

        const float step = (to - from) / static_cast<float>(count - 1);

        for (int i = 0; i < count; ++i)
        {
            out.push_back(from + step * i);
        }
    }

    //--------------------------------------------------------
    // 段差を1つ置く
    //   steps は96を何段積むか。地面まで必ず届く。
    //--------------------------------------------------------
    void AddPlatform(
        std::vector<MapPlatform>& out,
        float x,
        float width,
        int steps
    )
    {
        MapPlatform platform;

        platform.worldX = x;
        platform.width = width;
        platform.height = steps * TILE;
        platform.topY = GROUND_Y - steps * TILE;

        out.push_back(platform);
    }

    //--------------------------------------------------------
    // 障害物を1つ置く
    //   kind は画像が読めなかったときの色分けにだけ使う。
    //--------------------------------------------------------
    void AddObstacle(
        std::vector<MapObstacle>& out,
        float x,
        float width,
        float height,
        int col,
        int row,
        float hp,
        int kind = 0,
        float baseY = GROUND_Y
    )
    {
        MapObstacle obstacle;

        obstacle.worldX = x;
        obstacle.baseY = baseY;
        obstacle.width = width;
        obstacle.height = height;
        obstacle.maxHp = hp;
        obstacle.kind = kind;
        obstacle.tile = TileRef{ col, row };

        out.push_back(obstacle);
    }

    //--------------------------------------------------------
    // 飾りを1つ置く
    //--------------------------------------------------------
    void AddProp(
        std::vector<MapProp>& out,
        float x,
        float width,
        float height,
        int col,
        int row,
        bool flip = false,
        bool foreground = false,
        float baseY = GROUND_Y,
        int kind = 0
    )
    {
        MapProp prop;

        prop.worldX = x;
        prop.baseY = baseY;
        prop.width = width;
        prop.height = height;
        prop.kind = kind;
        prop.parallax = 1.0f;
        prop.tile = TileRef{ col, row };
        prop.flip = flip;
        prop.foreground = foreground;

        out.push_back(prop);
    }
}

float MapBase::ClampX(float x, float width) const
{
    return std::clamp(
        x,
        GetLeftLimit(),
        GetRightLimit() - width
    );
}

//============================================================
// STAGE 1 森林
//
//   木立の中を進み、奥の岩棚を3段登って抜ける。
//   遮蔽物が多く、近づいて撃ち合う形になりやすい。
//============================================================
void Map1::Build()
{
    m_worldWidth = 3840.0f;   // 1280 x 3 区画
    m_groundY = GROUND_Y;
    m_leftLimit = 40.0f;
    m_goalX = m_worldWidth - 220.0f;

    m_terrainTexture = L"Assets\\texture\\terrain_forest.png";

    m_bgmPath = BgmPath::Stage1;

    // 草の乗った土 ＋ 土の中詰め
    m_platformTopTile = TileRef{ 0, 0 };
    m_platformFillTile = TileRef{ 1, 0 };

    m_layers.clear();

    // 遠景：ほとんど動かない
    {
        BackgroundLayer layer;
        layer.texturePath = L"Assets\\texture\\bg_forest_far.png";
        layer.parallax = 0.12f;
        layer.heightScale = 1.00f;
        m_layers.push_back(layer);
    }

    // 中景：ステージの主な絵
    {
        BackgroundLayer layer;
        layer.texturePath = L"Assets\\texture\\bg_forest_mid.png";
        layer.parallax = 0.42f;
        layer.heightScale = 1.00f;
        m_layers.push_back(layer);
    }

    // 近景：下端に置き、一番速く流す
    {
        BackgroundLayer layer;
        layer.texturePath = L"Assets\\texture\\bg_forest_near.png";
        layer.parallax = 0.88f;
        layer.heightScale = 0.46f;
        layer.anchorBottom = true;
        layer.offsetY = 24.0f;
        m_layers.push_back(layer);
    }

    //--------------------------------------------------------
    // 段差
    //--------------------------------------------------------
    m_platforms.clear();

    // 区画1：低い岩棚をひとつだけ。操作に慣れてもらう。
    AddPlatform(m_platforms, 420.0f, 288.0f, 1);
    AddPlatform(m_platforms, 770.0f, 192.0f, 2);

    // 区画2：左右に分かれた足場。上を取ると撃ち下ろせる。
    AddPlatform(m_platforms, 1500.0f, 240.0f, 1);
    AddPlatform(m_platforms, 1820.0f, 192.0f, 1);
    AddPlatform(m_platforms, 2120.0f, 288.0f, 2);

    // 区画3：3段の階段。登り切った先に宝箱を置く。
    AddPlatform(m_platforms, 2700.0f, 192.0f, 1);
    AddPlatform(m_platforms, 2930.0f, 192.0f, 2);
    AddPlatform(m_platforms, 3170.0f, 240.0f, 3);
    AddPlatform(m_platforms, 3520.0f, 240.0f, 1);

    //--------------------------------------------------------
    // 障害物（遮蔽物）
    //   木箱 2,0 ／ 樽 3,0 ／ 石積み 4,0
    //--------------------------------------------------------
    m_obstacles.clear();

    AddObstacle(m_obstacles, 950.0f, 96.0f, 96.0f, 2, 0, 700.0f, 0);
    AddObstacle(m_obstacles, 1090.0f, 76.0f, 96.0f, 3, 0, 490.0f, 2);

    AddObstacle(m_obstacles, 1330.0f, 96.0f, 96.0f, 4, 0, 980.0f, 1);
    AddObstacle(m_obstacles, 1680.0f, 96.0f, 96.0f, 2, 0, 700.0f, 0);
    AddObstacle(m_obstacles, 1980.0f, 76.0f, 96.0f, 3, 0, 490.0f, 2);
    AddObstacle(m_obstacles, 2460.0f, 96.0f, 96.0f, 2, 0, 700.0f, 0);

    AddObstacle(m_obstacles, 2600.0f, 96.0f, 96.0f, 4, 0, 980.0f, 1);
    AddObstacle(m_obstacles, 3420.0f, 96.0f, 96.0f, 2, 0, 700.0f, 0);
    AddObstacle(m_obstacles, 3620.0f, 76.0f, 96.0f, 3, 0, 490.0f, 2);

    // 高い足場の上にも1つ置いて、上でも隠れられるようにする
    AddObstacle(
        m_obstacles, 2300.0f, 76.0f, 96.0f, 3, 0, 490.0f, 2,
        GROUND_Y - 2 * TILE
    );

    //--------------------------------------------------------
    // 飾り
    //--------------------------------------------------------
    m_props.clear();

    // 奥に大きな木を並べる。1本ごとに反転して同じ顔にしない。
    for (int i = 0; i < 13; ++i)
    {
        const float x = 90.0f + i * 300.0f;

        AddProp(m_props, x, 200.0f, 236.0f, 0, 2, (i % 2 == 1));
        AddProp(m_props, x + 170.0f, 150.0f, 138.0f, 6, 2, (i % 3 == 0));
    }

    // 低い茂み
    for (int i = 0; i < 15; ++i)
    {
        AddProp(m_props, 210.0f + i * 262.0f, 118.0f, 84.0f, 3, 3, (i % 2 == 1));
    }

    // 手前の下草。プレイヤーより前に出して奥行きを作る。
    for (int i = 0; i < 23; ++i)
    {
        const float x = 40.0f + i * 172.0f;

        AddProp(m_props, x, 88.0f, 62.0f, 7, 7, (i % 2 == 0), true);
    }

    for (int i = 0; i < 11; ++i)
    {
        AddProp(m_props, 320.0f + i * 356.0f, 96.0f, 66.0f, 9, 7, (i % 2 == 1), true);
    }

    // 見せ場：苔むした遺跡の柱と倒木
    AddProp(m_props, 250.0f, 74.0f, 208.0f, 0, 6);
    AddProp(m_props, 640.0f, 64.0f, 158.0f, 2, 6, true);
    AddProp(m_props, 1180.0f, 148.0f, 74.0f, 6, 6);
    AddProp(m_props, 1420.0f, 106.0f, 96.0f, 8, 5);
    AddProp(m_props, 1760.0f, 74.0f, 208.0f, 0, 6, true);
    AddProp(m_props, 2020.0f, 140.0f, 66.0f, 3, 5);
    AddProp(m_props, 2350.0f, 118.0f, 98.0f, 8, 3);
    AddProp(m_props, 2560.0f, 64.0f, 158.0f, 2, 6);
    AddProp(m_props, 3040.0f, 148.0f, 74.0f, 6, 6, true);
    AddProp(m_props, 3300.0f, 92.0f, 168.0f, 2, 7);
    AddProp(m_props, 3700.0f, 136.0f, 148.0f, 0, 8);

    // シダと花を散らす
    for (int i = 0; i < 9; ++i)
    {
        AddProp(m_props, 420.0f + i * 398.0f, 104.0f, 92.0f, 2, 3, (i % 2 == 0));
        AddProp(m_props, 560.0f + i * 398.0f, 92.0f, 58.0f, 4, 5, false, true);
    }

    //--------------------------------------------------------
    // 湧き位置と宝箱
    //--------------------------------------------------------
    MakeSpawnPoints(m_spawnPoints, 500.0f, m_worldWidth - 300.0f, 9);

    m_chestPoints = {
        { 1200.0f, GROUND_Y },              // 通り道
        { 2200.0f, GROUND_Y - 2 * TILE },   // 中段の足場の上
        { 3230.0f, GROUND_Y - 3 * TILE }    // 階段を登り切った所
    };
}

//============================================================
// STAGE 2 平原
//
//   ひらけた農地。遮蔽物は低く、間隔も広い。
//   遠くから撃ち合う形になりやすいステージ。
//============================================================
void Map2::Build()
{
    m_worldWidth = 5120.0f;   // 1280 x 4 区画
    m_groundY = GROUND_Y;
    m_leftLimit = 40.0f;
    m_goalX = m_worldWidth - 220.0f;

    m_terrainTexture = L"Assets\\texture\\terrain_plain.png";

    m_bgmPath = BgmPath::Stage2;

    m_platformTopTile = TileRef{ 0, 0 };
    m_platformFillTile = TileRef{ 1, 0 };

    m_layers.clear();

    // 遠景：ほとんど動かない
    {
        BackgroundLayer layer;
        layer.texturePath = L"Assets\\texture\\bg_plain_far.png";
        layer.parallax = 0.12f;
        layer.heightScale = 1.00f;
        m_layers.push_back(layer);
    }

    // 中景：ステージの主な絵
    {
        BackgroundLayer layer;
        layer.texturePath = L"Assets\\texture\\bg_plain_mid.png";
        layer.parallax = 0.42f;
        layer.heightScale = 1.00f;
        m_layers.push_back(layer);
    }

    // 近景：下端に置き、一番速く流す
    {
        BackgroundLayer layer;
        layer.texturePath = L"Assets\\texture\\bg_plain_near.png";
        layer.parallax = 0.88f;
        layer.heightScale = 0.46f;
        layer.anchorBottom = true;
        layer.offsetY = 24.0f;
        m_layers.push_back(layer);
    }

    //--------------------------------------------------------
    // 段差
    //   森林より数を減らし、1つ1つを広くしている。
    //   走り回れる平地を残すため。
    //--------------------------------------------------------
    m_platforms.clear();

    AddPlatform(m_platforms, 700.0f, 384.0f, 1);

    AddPlatform(m_platforms, 1450.0f, 288.0f, 1);
    AddPlatform(m_platforms, 1800.0f, 192.0f, 2);

    AddPlatform(m_platforms, 2600.0f, 384.0f, 1);

    AddPlatform(m_platforms, 3300.0f, 192.0f, 1);
    AddPlatform(m_platforms, 3550.0f, 288.0f, 2);

    AddPlatform(m_platforms, 4400.0f, 384.0f, 1);

    //--------------------------------------------------------
    // 障害物
    //   樽 3,0 ／ 木箱 2,0 ／ 大岩 5,4 ／ 荷車 7,5
    //   （絵が地面まで届いているマスだけを使う）
    //--------------------------------------------------------
    m_obstacles.clear();

    AddObstacle(m_obstacles, 1080.0f, 76.0f, 96.0f, 3, 0, 620.0f, 2);
    AddObstacle(m_obstacles, 1230.0f, 104.0f, 96.0f, 2, 0, 820.0f, 0);

    AddObstacle(m_obstacles, 2080.0f, 110.0f, 96.0f, 5, 4, 1400.0f, 1);
    AddObstacle(m_obstacles, 2320.0f, 104.0f, 96.0f, 2, 0, 820.0f, 0);
    AddObstacle(m_obstacles, 3040.0f, 128.0f, 96.0f, 7, 5, 900.0f, 0);

    AddObstacle(m_obstacles, 3940.0f, 110.0f, 96.0f, 5, 4, 1400.0f, 1);
    AddObstacle(m_obstacles, 4180.0f, 76.0f, 96.0f, 3, 0, 620.0f, 2);
    AddObstacle(m_obstacles, 4860.0f, 104.0f, 96.0f, 2, 0, 820.0f, 0);

    // 高台の上の遮蔽物
    AddObstacle(
        m_obstacles, 1880.0f, 104.0f, 96.0f, 2, 0, 820.0f, 0,
        GROUND_Y - 2 * TILE
    );

    //--------------------------------------------------------
    // 飾り
    //--------------------------------------------------------
    m_props.clear();

    // 木は森林より少なく、間隔をあけて「ひらけた」感じを出す
    for (int i = 0; i < 9; ++i)
    {
        AddProp(m_props, 160.0f + i * 580.0f, 190.0f, 176.0f, 0, 2, (i % 2 == 1));
        AddProp(m_props, 420.0f + i * 580.0f, 150.0f, 168.0f, 3, 2, (i % 3 == 0));
    }

    // 農地を区切る木柵。同じ絵を横に continue して並べる。
    for (int i = 0; i < 26; ++i)
    {
        const float x = 120.0f + i * 190.0f;

        // 3本並べて途切れさせ、柵が続きすぎないようにする
        if (i % 4 == 3)
        {
            continue;
        }

        AddProp(m_props, x, 190.0f, 92.0f, 6, 4);
    }

    // 花畑（手前）
    for (int i = 0; i < 30; ++i)
    {
        const float x = 60.0f + i * 172.0f;

        AddProp(m_props, x, 96.0f, 58.0f, 3, 4, (i % 2 == 0), true);
    }

    for (int i = 0; i < 16; ++i)
    {
        AddProp(m_props, 240.0f + i * 322.0f, 70.0f, 64.0f, 0, 4, (i % 2 == 1), true);
    }

    // 麦・とうもろこし畑
    for (int i = 0; i < 14; ++i)
    {
        const float x = 1520.0f + i * 240.0f;

        AddProp(m_props, x, 100.0f, 122.0f, 5, 6, (i % 2 == 0));
        AddProp(m_props, x + 110.0f, 96.0f, 106.0f, 5, 7, (i % 3 == 0));
    }

    // 見せ場：野営地・荷車・道しるべ・旗
    AddProp(m_props, 520.0f, 168.0f, 118.0f, 8, 6);
    AddProp(m_props, 900.0f, 90.0f, 176.0f, 9, 1);
    AddProp(m_props, 1330.0f, 92.0f, 150.0f, 9, 4);
    AddProp(m_props, 1980.0f, 148.0f, 82.0f, 4, 5);
    AddProp(m_props, 2500.0f, 168.0f, 118.0f, 8, 6, true);
    AddProp(m_props, 2900.0f, 90.0f, 176.0f, 1, 7);
    AddProp(m_props, 3200.0f, 92.0f, 150.0f, 5, 5, true);
    AddProp(m_props, 3860.0f, 148.0f, 82.0f, 11, 7);
    AddProp(m_props, 4320.0f, 118.0f, 96.0f, 5, 3);
    AddProp(m_props, 4680.0f, 172.0f, 96.0f, 9, 6);
    AddProp(m_props, 5000.0f, 118.0f, 90.0f, 10, 7, true);

    // 低木で足元を埋める
    for (int i = 0; i < 12; ++i)
    {
        AddProp(m_props, 340.0f + i * 412.0f, 112.0f, 84.0f, 6, 3, (i % 2 == 1));
    }

    //--------------------------------------------------------
    // 湧き位置と宝箱
    //--------------------------------------------------------
    MakeSpawnPoints(m_spawnPoints, 520.0f, m_worldWidth - 320.0f, 11);

    m_chestPoints = {
        { 820.0f,  GROUND_Y - TILE },       // 最初の広い段の上
        { 1806.0f, GROUND_Y - 2 * TILE },   // 高台
        { 3640.0f, GROUND_Y - 2 * TILE }    // 奥の高台
    };
}

//============================================================
// STAGE 3 郊外
//
//   村のはずれ。建物と石壁で高低差が一番大きい。
//   屋根や石垣の上を渡っていくステージ。
//============================================================
void Map3::Build()
{
    m_worldWidth = 6400.0f;   // 1280 x 5 区画
    m_groundY = GROUND_Y;
    m_leftLimit = 40.0f;
    m_goalX = m_worldWidth - 220.0f;

    m_terrainTexture = L"Assets\\texture\\terrain_suburb.png";

    m_bgmPath = BgmPath::Stage3;

    // 草の乗った石垣 ＋ 石積みの中詰め（村の石段に見せる）
    m_platformTopTile = TileRef{ 8, 0 };
    m_platformFillTile = TileRef{ 3, 6 };

    m_layers.clear();

    // 遠景：ほとんど動かない
    {
        BackgroundLayer layer;
        layer.texturePath = L"Assets\\texture\\bg_suburb_far.png";
        layer.parallax = 0.12f;
        layer.heightScale = 1.00f;
        m_layers.push_back(layer);
    }

    // 中景：ステージの主な絵
    {
        BackgroundLayer layer;
        layer.texturePath = L"Assets\\texture\\bg_suburb_mid.png";
        layer.parallax = 0.42f;
        layer.heightScale = 1.00f;
        m_layers.push_back(layer);
    }

    // 近景：下端に置き、一番速く流す
    {
        BackgroundLayer layer;
        layer.texturePath = L"Assets\\texture\\bg_suburb_near.png";
        layer.parallax = 0.88f;
        layer.heightScale = 0.46f;
        layer.anchorBottom = true;
        layer.offsetY = 24.0f;
        m_layers.push_back(layer);
    }

    //--------------------------------------------------------
    // 段差
    //   1段ずつの踏み台を必ず隣に置き、
    //   3段目まで登れるようにしてある。
    //--------------------------------------------------------
    m_platforms.clear();

    AddPlatform(m_platforms, 600.0f, 288.0f, 1);
    AddPlatform(m_platforms, 950.0f, 288.0f, 2);

    AddPlatform(m_platforms, 1700.0f, 192.0f, 1);
    AddPlatform(m_platforms, 1930.0f, 192.0f, 2);
    AddPlatform(m_platforms, 2160.0f, 288.0f, 3);

    AddPlatform(m_platforms, 2900.0f, 384.0f, 1);

    AddPlatform(m_platforms, 3230.0f, 192.0f, 1);
    AddPlatform(m_platforms, 3500.0f, 288.0f, 2);

    AddPlatform(m_platforms, 4200.0f, 288.0f, 1);
    AddPlatform(m_platforms, 4550.0f, 192.0f, 2);
    AddPlatform(m_platforms, 4790.0f, 288.0f, 3);

    AddPlatform(m_platforms, 5500.0f, 384.0f, 1);

    //--------------------------------------------------------
    // 障害物
    //   樽 3,0 ／ 樽 3,7 ／ 木箱 10,6 ／ 石ブロック 8,8
    //   （絵が地面まで届いているマスだけを使う）
    //--------------------------------------------------------
    m_obstacles.clear();

    AddObstacle(m_obstacles, 1300.0f, 96.0f, 96.0f, 10, 6, 900.0f, 0);
    AddObstacle(m_obstacles, 1450.0f, 76.0f, 96.0f, 3, 0, 620.0f, 2);

    AddObstacle(m_obstacles, 2520.0f, 88.0f, 96.0f, 3, 7, 760.0f, 2);
    AddObstacle(m_obstacles, 2700.0f, 96.0f, 96.0f, 8, 8, 1000.0f, 1);

    AddObstacle(m_obstacles, 3400.0f, 96.0f, 96.0f, 10, 6, 900.0f, 0);
    AddObstacle(m_obstacles, 3900.0f, 76.0f, 96.0f, 3, 0, 620.0f, 2);
    AddObstacle(m_obstacles, 4060.0f, 96.0f, 96.0f, 8, 8, 1000.0f, 1);

    AddObstacle(m_obstacles, 5150.0f, 88.0f, 96.0f, 3, 7, 760.0f, 2);
    AddObstacle(m_obstacles, 5320.0f, 96.0f, 96.0f, 10, 6, 900.0f, 0);
    AddObstacle(m_obstacles, 6000.0f, 76.0f, 96.0f, 3, 0, 620.0f, 2);

    // 屋上の遮蔽物
    AddObstacle(
        m_obstacles, 2340.0f, 96.0f, 96.0f, 10, 6, 900.0f, 0,
        GROUND_Y - 3 * TILE
    );

    AddObstacle(
        m_obstacles, 4950.0f, 96.0f, 96.0f, 8, 8, 1000.0f, 1,
        GROUND_Y - 3 * TILE
    );

    //--------------------------------------------------------
    // 飾り
    //--------------------------------------------------------
    m_props.clear();

    // 家並み。奥に並べて「村のはずれ」を出す。
    for (int i = 0; i < 7; ++i)
    {
        const float x = 300.0f + i * 900.0f;

        AddProp(m_props, x, 208.0f, 232.0f, 2, 8);
        AddProp(m_props, x + 470.0f, 208.0f, 232.0f, 3, 8, (i % 2 == 1));
    }

    // 石壁を家と家の間に置く
    for (int i = 0; i < 18; ++i)
    {
        AddProp(m_props, 180.0f + i * 352.0f, 116.0f, 104.0f, 10, 3, (i % 2 == 1));
    }

    // 街灯。等間隔に置くと「整備された道」に見える。
    for (int i = 0; i < 14; ++i)
    {
        AddProp(m_props, 240.0f + i * 460.0f, 84.0f, 196.0f, 1, 7, (i % 2 == 1));
    }

    // 鉄柵と木柵
    for (int i = 0; i < 12; ++i)
    {
        AddProp(m_props, 420.0f + i * 520.0f, 178.0f, 158.0f, 7, 6);
    }

    for (int i = 0; i < 13; ++i)
    {
        AddProp(m_props, 700.0f + i * 470.0f, 172.0f, 92.0f, 11, 4, (i % 2 == 0));
    }

    // 手前の花壇
    for (int i = 0; i < 26; ++i)
    {
        AddProp(m_props, 80.0f + i * 246.0f, 96.0f, 62.0f, 9, 3, (i % 2 == 0), true);
    }

    // 見せ場：井戸・市場・鐘のアーチ・納屋
    AddProp(m_props, 820.0f, 158.0f, 190.0f, 5, 8);
    AddProp(m_props, 1550.0f, 196.0f, 180.0f, 11, 1);
    AddProp(m_props, 2360.0f, 148.0f, 196.0f, 6, 8);
    AddProp(m_props, 2980.0f, 152.0f, 128.0f, 0, 7);
    AddProp(m_props, 3120.0f, 156.0f, 108.0f, 2, 7, true);
    AddProp(m_props, 3760.0f, 186.0f, 148.0f, 9, 8);
    AddProp(m_props, 4400.0f, 158.0f, 190.0f, 5, 8, true);
    AddProp(m_props, 5050.0f, 176.0f, 176.0f, 8, 5);
    AddProp(m_props, 5620.0f, 92.0f, 150.0f, 8, 6);
    AddProp(m_props, 5900.0f, 196.0f, 180.0f, 11, 1, true);
    AddProp(m_props, 6180.0f, 148.0f, 196.0f, 6, 8, true);

    // 樹と柱で隙間を埋める
    for (int i = 0; i < 11; ++i)
    {
        AddProp(m_props, 520.0f + i * 566.0f, 176.0f, 194.0f, 0, 2, (i % 2 == 1));
        AddProp(m_props, 760.0f + i * 566.0f, 72.0f, 186.0f, 7, 5, (i % 3 == 0));
    }

    //--------------------------------------------------------
    // 湧き位置と宝箱
    //--------------------------------------------------------
    MakeSpawnPoints(m_spawnPoints, 540.0f, m_worldWidth - 340.0f, 13);

    m_chestPoints = {
        { 700.0f,  GROUND_Y - TILE },       // 最初の石垣
        { 2250.0f, GROUND_Y - 3 * TILE },   // 屋上
        { 4870.0f, GROUND_Y - 3 * TILE }    // 奥の屋上
    };
}

//============================================================
// BOSS
//
//   郊外の最奥。左右対称の闘技場。
//   中央に高台、その左右に踏み台を置き、
//   ボスの攻撃を避けながら登り降りできるようにしている。
//   遮蔽物は端に寄せ、中央を広く空けてある。
//============================================================
void MapBoss::Build()
{
    // ボス戦は逃げ回れる程度の広さにとどめる
    m_worldWidth = 2560.0f;   // 1280 x 2 区画
    m_groundY = GROUND_Y;
    m_leftLimit = 40.0f;
    m_goalX = m_worldWidth * 0.5f;

    m_terrainTexture = L"Assets\\texture\\terrain_suburb.png";

    m_bgmPath = BgmPath::Boss;

    // 草の乗った石垣 ＋ 石積みの中詰め（村の石段に見せる）
    m_platformTopTile = TileRef{ 8, 0 };
    m_platformFillTile = TileRef{ 3, 6 };

    m_layers.clear();

    // 遠景：ほとんど動かない
    {
        BackgroundLayer layer;
        layer.texturePath = L"Assets\\texture\\bg_suburb_far.png";
        layer.parallax = 0.12f;
        layer.heightScale = 1.00f;
        m_layers.push_back(layer);
    }

    // 中景：ステージの主な絵
    {
        BackgroundLayer layer;
        layer.texturePath = L"Assets\\texture\\bg_suburb_mid.png";
        layer.parallax = 0.42f;
        layer.heightScale = 1.00f;
        m_layers.push_back(layer);
    }

    // 近景：下端に置き、一番速く流す
    {
        BackgroundLayer layer;
        layer.texturePath = L"Assets\\texture\\bg_suburb_near.png";
        layer.parallax = 0.88f;
        layer.heightScale = 0.46f;
        layer.anchorBottom = true;
        layer.offsetY = 24.0f;
        m_layers.push_back(layer);
    }

    //--------------------------------------------------------
    // 段差（左右対称）
    //--------------------------------------------------------
    m_platforms.clear();

    AddPlatform(m_platforms, 360.0f, 240.0f, 1);
    AddPlatform(m_platforms, 860.0f, 192.0f, 1);

    // 中央の高台
    AddPlatform(m_platforms, 1100.0f, 360.0f, 2);

    AddPlatform(m_platforms, 1510.0f, 192.0f, 1);
    AddPlatform(m_platforms, 1960.0f, 240.0f, 1);

    //--------------------------------------------------------
    // 障害物
    //   中央を空け、端に寄せて置く。
    //   逃げ場にはなるが、ボスから隠れきれない距離。
    //--------------------------------------------------------
    m_obstacles.clear();

    AddObstacle(m_obstacles, 200.0f, 96.0f, 96.0f, 8, 8, 1600.0f, 1);
    AddObstacle(m_obstacles, 680.0f, 76.0f, 96.0f, 3, 0, 1200.0f, 2);
    AddObstacle(m_obstacles, 1800.0f, 76.0f, 96.0f, 3, 0, 1200.0f, 2);
    AddObstacle(m_obstacles, 2280.0f, 96.0f, 96.0f, 8, 8, 1600.0f, 1);

    //--------------------------------------------------------
    // 飾り
    //   崩れた石柱とアーチを並べ、決戦の場らしくする。
    //--------------------------------------------------------
    m_props.clear();

    AddProp(m_props, 120.0f, 176.0f, 176.0f, 8, 5);
    AddProp(m_props, 420.0f, 72.0f, 214.0f, 7, 5);
    AddProp(m_props, 620.0f, 178.0f, 158.0f, 7, 6);
    AddProp(m_props, 940.0f, 72.0f, 186.0f, 7, 5, true);

    // 中央の高台の背後にそびえさせ、決戦の場の目印にする
    AddProp(m_props, 1160.0f, 200.0f, 296.0f, 6, 8);

    AddProp(m_props, 1560.0f, 72.0f, 186.0f, 7, 5);
    AddProp(m_props, 1760.0f, 178.0f, 158.0f, 7, 6, true);
    AddProp(m_props, 2040.0f, 72.0f, 214.0f, 7, 5, true);
    AddProp(m_props, 2280.0f, 176.0f, 176.0f, 8, 5, true);

    // 街灯を左右対称に置く
    for (int i = 0; i < 6; ++i)
    {
        AddProp(m_props, 300.0f + i * 400.0f, 84.0f, 196.0f, 1, 7, (i % 2 == 1));
    }

    // 足元の草
    for (int i = 0; i < 16; ++i)
    {
        AddProp(m_props, 60.0f + i * 164.0f, 94.0f, 62.0f, 9, 3, (i % 2 == 0), true);
    }

    MakeSpawnPoints(m_spawnPoints, 400.0f, m_worldWidth - 300.0f, 5);

    m_chestPoints = {
        { 1250.0f, GROUND_Y - 2 * TILE }    // 中央の高台の上
    };
}

//============================================================

std::unique_ptr<MapBase> CreateMap(int stageIndex)
{
    std::unique_ptr<MapBase> map;

    switch (stageIndex)
    {
    case 0:  map = std::make_unique<Map1>();    break;
    case 1:  map = std::make_unique<Map2>();    break;
    case 2:  map = std::make_unique<Map3>();    break;
    default: map = std::make_unique<MapBoss>(); break;
    }

    map->Build();

    return map;
}

/*--------------------------------------------------------------------

ファイル名：Scene.h
お仕事　　：Scene共通の土台

設計書 Ver.3 19-1
    StartScene / LevelSelectScene / GameScene /
    ResultScene / GameOverScene / GameClearScene / OptionScene

main.cpp は SceneType を見て切り替えるだけにする。

----------------------------------------------------------------------*/

#ifndef SCENE_H
#define SCENE_H

#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

#include <string>
#include <vector>

#include "InputTracker.h"

enum class SceneType
{
    None,
    Start,
    LevelSelect,
    Game,
    Result,
    GameOver,
    GameClear,
    Option,
    Controls,
    Ranking,
    Exit
};

//------------------------------------------------------------
// Scene が描画に使う道具一式
// 各 Scene が個別に Brush や TextFormat を持たなくて済むようにする。
//------------------------------------------------------------
struct SceneContext
{
    ID2D1RenderTarget* renderTarget = nullptr;

    IDWriteTextFormat* titleFormat = nullptr;   // 大見出し
    IDWriteTextFormat* headerFormat = nullptr;  // 見出し
    IDWriteTextFormat* menuFormat = nullptr;    // メニュー
    IDWriteTextFormat* bodyFormat = nullptr;    // 本文

    ID2D1SolidColorBrush* brush = nullptr;      // 色を変えて使い回す

    // タイトル用の背景画像（無ければ nullptr）
    ID2D1Bitmap* titleBackground = nullptr;

    // 操作方法の背景
    ID2D1Bitmap* controlsBackground = nullptr;

    // 結果画面の背景
    ID2D1Bitmap* gameOverBackground = nullptr;
    ID2D1Bitmap* gameClearBackground = nullptr;

    //--------------------------------------------------------
    // タイトルで流す3ステージの背景
    //   0 森林 / 1 平原 / 2 郊外
    //--------------------------------------------------------
    ID2D1Bitmap* scenery[3] = { nullptr, nullptr, nullptr };

    // タイトルに立たせるキャラクター（0 Player1 / 1 Player2）
    ID2D1Bitmap* playerSheet[2] = { nullptr, nullptr };

    //--------------------------------------------------------
    // タイトル専用の背景（0 遠景 / 1 中景 / 2 近景）
    //--------------------------------------------------------
    ID2D1Bitmap* titleLayer[3] = { nullptr, nullptr, nullptr };

    float screenWidth = 1280.0f;
    float screenHeight = 720.0f;

    //--------------------------------------------------------
    // 描画ヘルパ
    //--------------------------------------------------------
    void SetColor(float r, float g, float b, float a = 1.0f) const;

    void DrawText(
        const std::wstring& text,
        float left, float top, float right, float bottom,
        IDWriteTextFormat* format
    ) const;

    void FillRect(float left, float top, float right, float bottom) const;

    void DrawRoundRect(
        float left, float top, float right, float bottom,
        float radius, float thickness
    ) const;

    void FillRoundRect(
        float left, float top, float right, float bottom,
        float radius
    ) const;
};

//------------------------------------------------------------
// メニュー1項目
//------------------------------------------------------------
struct MenuItem
{
    std::wstring label;
    SceneType next = SceneType::None;
    bool enabled = true;
};

//------------------------------------------------------------
// 上下選択メニューの共通処理
//
// キーボードとマウス両方で選べる（設計書 19-1.1）。
//------------------------------------------------------------
class MenuController
{
public:
    void SetItems(const std::vector<MenuItem>& items);

    void SetLayout(float centerX, float topY, float width, float height, float gap);

    // 決定されたら true を返す
    bool Update(
        const InputTracker& input,
        float mouseX,
        float mouseY,
        float deltaTime = 0.016f
    );

    void Draw(const SceneContext& context) const;

    int GetSelected() const { return m_selected; }
    const MenuItem* GetSelectedItem() const;

    void Reset();

private:
    D2D1_RECT_F GetItemRect(int index) const;

private:
    std::vector<MenuItem> m_items;

    int m_selected = 0;

    float m_centerX = 640.0f;
    float m_topY = 320.0f;
    float m_width = 360.0f;
    float m_height = 56.0f;
    float m_gap = 18.0f;

    // 選択が移った瞬間に 1.0 になり、そこから減衰する
    float m_glow = 1.0f;

    // 選択中の項目をゆっくり点滅させるための時間
    float m_blink = 0.0f;
    int m_lastSelected = -1;
};

//------------------------------------------------------------
// Scene の共通インターフェース
//------------------------------------------------------------
class Scene
{
public:
    virtual ~Scene() = default;

    virtual void Reset() {}

    // 次に移りたい Scene を返す。移らないなら None
    virtual SceneType Update(
        const InputTracker& input,
        float mouseX,
        float mouseY,
        float deltaTime
    ) = 0;

    virtual void Draw(const SceneContext& context) = 0;
};

#endif

/*--------------------------------------------------------------------

ファイル名：main.cpp
お仕事　　：ウィンドウ生成、DirectX11 / Direct2D 初期化、Scene切り替え

設計書 Ver.3 19-0
    ・Scene固有のゲームロジックはここへ書かない。
    ・装備抽選ロジックもここへ書かない。
    ・Scene切り替えとゲームループの管理だけを行う。

Scene遷移：
    Start → LevelSelect → Game
                            ├ StageClear → Result → 次のStage
                            ├ GameOver   → GameOverScene
                            └ Boss撃破   → GameClearScene

----------------------------------------------------------------------*/
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

#include <algorithm>
#include <chrono>
#include <cmath>

#include "AudioManager.h"
#include "ConfigData.h"
#include "PostEffect.h"
#include "ControlsScene.h"
#include "RankingScene.h"
#include "ResourceManager.h"
#include "GameClearScene.h"
#include "GameOverScene.h"
#include "GameScene.h"
#include "ControlsScene.h"
#include "LevelSelectScene.h"
#include "ResultScene.h"
#include "Scene.h"
#include "StartScene.h"
#include "option.h"

using Microsoft::WRL::ComPtr;

constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;

LRESULT CALLBACK WndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
);

//============================================================
// アプリケーション本体
//============================================================
class Application
{
public:
    bool Initialize(HWND hwnd);

    void Update();
    void Draw();

private:
    bool InitializeDirectX(HWND hwnd);
    bool InitializeDirect2D();
    bool InitializeText();

    void ChangeScene(SceneType next);
    void ApplySceneBgm(SceneType scene);
    Scene* GetCurrentScene();

    void UpdateMouse();

private:
    HWND m_hwnd{};

    //--------------------------------------------------------
    // Scene
    //--------------------------------------------------------
    SceneType m_currentScene = SceneType::Start;

    //--------------------------------------------------------
    // 設定画面から戻る先。
    //   タイトルから開いたときは Start、
    //   ポーズから開いたときは Game。
    //--------------------------------------------------------
    SceneType m_optionReturn = SceneType::Start;

    //--------------------------------------------------------
    // 操作説明から戻る先。
    //   タイトルから開いたときは Start、
    //   設定から開いたときは Option。
    //
    // ControlsScene 自身は「戻る」としか返さないので、
    // どこから開いたかはここで覚えておく。
    //--------------------------------------------------------
    SceneType m_controlsReturn = SceneType::Start;

    StartScene m_startScene;
    LevelSelectScene m_levelSelectScene;
    GameScene m_gameScene;
    ResultScene m_resultScene;
    GameOverScene m_gameOverScene;
    GameClearScene m_gameClearScene;
    OptionScene m_optionScene;
    ControlsScene m_controlsScene;
    RankingScene m_rankingScene;

    InputTracker m_input;

    float m_mouseX = 0.0f;
    float m_mouseY = 0.0f;

    OptionData m_option;

    // タイトル背景などシーン共通の画像
    ResourceManager m_sceneResources;

    //--------------------------------------------------------
    // DirectX11
    //--------------------------------------------------------
    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_context;
    ComPtr<IDXGISwapChain> m_swapChain;
    ComPtr<ID3D11RenderTargetView> m_renderTargetView;

    //--------------------------------------------------------
    // Direct2D / DirectWrite
    //--------------------------------------------------------
    //--------------------------------------------------------
    // 画面全体へ掛ける後処理（HLSL）
    //   用意できなかった場合はバックバッファへ直接描く。
    //--------------------------------------------------------
    PostEffect m_postEffect;

    ComPtr<ID2D1Factory> m_d2dFactory;
    ComPtr<ID2D1RenderTarget> m_d2dRenderTarget;

    ComPtr<IDWriteFactory> m_writeFactory;
    ComPtr<IDWriteTextFormat> m_titleFormat;
    ComPtr<IDWriteTextFormat> m_headerFormat;
    ComPtr<IDWriteTextFormat> m_menuFormat;
    ComPtr<IDWriteTextFormat> m_bodyFormat;

    ComPtr<ID2D1SolidColorBrush> m_brush;
    ComPtr<ID2D1SolidColorBrush> m_white;
    ComPtr<ID2D1SolidColorBrush> m_blue;
    ComPtr<ID2D1SolidColorBrush> m_gray;

    SceneContext m_sceneContext;

    std::chrono::steady_clock::time_point m_lastUpdate;
};

//------------------------------------------------------------
// DirectX11
//------------------------------------------------------------
bool Application::InitializeDirectX(HWND hwnd)
{
    DXGI_SWAP_CHAIN_DESC swapDesc{};

    swapDesc.BufferDesc.Width = SCREEN_WIDTH;
    swapDesc.BufferDesc.Height = SCREEN_HEIGHT;
    swapDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;

    swapDesc.SampleDesc.Count = 1;
    swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapDesc.BufferCount = 2;
    swapDesc.OutputWindow = hwnd;
    swapDesc.Windowed = TRUE;
    swapDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    // Direct2Dと相互運用するために BGRA サポートが必要
    const UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    const D3D_FEATURE_LEVEL levels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };

    D3D_FEATURE_LEVEL featureLevel{};

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        flags,
        levels,
        ARRAYSIZE(levels),
        D3D11_SDK_VERSION,
        &swapDesc,
        &m_swapChain,
        &m_device,
        &featureLevel,
        &m_context
    );

    if (FAILED(hr))
    {
        MessageBox(
            nullptr,
            L"DirectX11の初期化に失敗しました。",
            L"BULLET DRAFT",
            MB_OK | MB_ICONERROR
        );

        return false;
    }

    ComPtr<ID3D11Texture2D> backBuffer;

    hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));

    if (FAILED(hr))
    {
        return false;
    }

    hr = m_device->CreateRenderTargetView(
        backBuffer.Get(),
        nullptr,
        &m_renderTargetView
    );

    if (FAILED(hr))
    {
        return false;
    }

    //--------------------------------------------------------
    // 後処理の用意。
    //
    // .cso が見つからない、シェーダが作れないといった場合は
    // false が返るだけで、ゲームはこれまでどおり動く。
    //--------------------------------------------------------
    m_postEffect.Initialize(m_device.Get(), SCREEN_WIDTH, SCREEN_HEIGHT);

    return true;
}

//------------------------------------------------------------
// Direct2D
//------------------------------------------------------------
bool Application::InitializeDirect2D()
{
    HRESULT hr = D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        m_d2dFactory.GetAddressOf()
    );

    if (FAILED(hr))
    {
        return false;
    }

    //--------------------------------------------------------
    // Direct2D の描画先
    //
    //   後処理が使えるときは、いったんオフスクリーンへ描く。
    //   そこへシェーダを掛けてからバックバッファへ出す。
    //   使えないときは、これまでどおりバックバッファへ直接。
    //--------------------------------------------------------
    ComPtr<IDXGISurface> surface;

    if (m_postEffect.IsReady())
    {
        surface = m_postEffect.GetSceneSurface();
    }
    else
    {
        ComPtr<ID3D11Texture2D> backBuffer;

        hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));

        if (FAILED(hr))
        {
            return false;
        }

        hr = backBuffer.As(&surface);

        if (FAILED(hr))
        {
            return false;
        }
    }

    const D2D1_RENDER_TARGET_PROPERTIES properties =
        D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(
                DXGI_FORMAT_B8G8R8A8_UNORM,
                D2D1_ALPHA_MODE_PREMULTIPLIED
            )
        );

    hr = m_d2dFactory->CreateDxgiSurfaceRenderTarget(
        surface.Get(),
        &properties,
        &m_d2dRenderTarget
    );

    //--------------------------------------------------------
    // オフスクリーンへ Direct2D を結び付けられない環境では、
    // 後処理をあきらめてバックバッファへ直接描く。
    // 絵は素のままになるが、ゲームは動く。
    //--------------------------------------------------------
    if (FAILED(hr) && m_postEffect.IsReady())
    {
        m_postEffect.Shutdown();

        ComPtr<ID3D11Texture2D> backBuffer;

        hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));

        if (SUCCEEDED(hr))
        {
            hr = backBuffer.As(&surface);
        }

        if (SUCCEEDED(hr))
        {
            hr = m_d2dFactory->CreateDxgiSurfaceRenderTarget(
                surface.Get(),
                &properties,
                &m_d2dRenderTarget
            );
        }
    }

    if (FAILED(hr))
    {
        return false;
    }

    //--------------------------------------------------------
    // 共通ブラシ
    //--------------------------------------------------------
    struct BrushRequest
    {
        D2D1_COLOR_F color;
        ID2D1SolidColorBrush** target;
    };

    const BrushRequest requests[] =
    {
        { D2D1::ColorF(1.00f, 1.00f, 1.00f), &m_brush },
        { D2D1::ColorF(0.90f, 0.95f, 1.00f), &m_white },
        { D2D1::ColorF(0.10f, 0.70f, 1.00f), &m_blue  },
        { D2D1::ColorF(0.30f, 0.35f, 0.40f), &m_gray  }
    };

    for (const BrushRequest& request : requests)
    {
        hr = m_d2dRenderTarget->CreateSolidColorBrush(
            request.color,
            request.target
        );

        if (FAILED(hr))
        {
            return false;
        }
    }

    return true;
}

//------------------------------------------------------------
// DirectWrite
//------------------------------------------------------------
bool Application::InitializeText()
{
    HRESULT hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(m_writeFactory.GetAddressOf())
    );

    if (FAILED(hr))
    {
        return false;
    }

    struct FormatRequest
    {
        const wchar_t* font;
        float size;
        DWRITE_FONT_WEIGHT weight;
        bool center;
        IDWriteTextFormat** target;
    };

    const FormatRequest requests[] =
    {
        { L"Arial Black",  80.0f, DWRITE_FONT_WEIGHT_BLACK,     true,  &m_titleFormat  },
        { L"Yu Gothic UI", 30.0f, DWRITE_FONT_WEIGHT_BOLD,      true,  &m_headerFormat },
        { L"Yu Gothic UI", 26.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD, true,  &m_menuFormat   },
        { L"Yu Gothic UI", 18.0f, DWRITE_FONT_WEIGHT_NORMAL,    false, &m_bodyFormat   }
    };

    for (const FormatRequest& request : requests)
    {
        hr = m_writeFactory->CreateTextFormat(
            request.font,
            nullptr,
            request.weight,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            request.size,
            L"ja-jp",
            request.target
        );

        if (FAILED(hr))
        {
            return false;
        }

        (*request.target)->SetTextAlignment(
            request.center
            ? DWRITE_TEXT_ALIGNMENT_CENTER
            : DWRITE_TEXT_ALIGNMENT_LEADING
        );

        (*request.target)->SetParagraphAlignment(
            request.center
            ? DWRITE_PARAGRAPH_ALIGNMENT_CENTER
            : DWRITE_PARAGRAPH_ALIGNMENT_NEAR
        );
    }

    return true;
}

bool Application::Initialize(HWND hwnd)
{
    m_hwnd = hwnd;

    if (!InitializeDirectX(hwnd))
    {
        return false;
    }

    if (!InitializeDirect2D())
    {
        return false;
    }

    if (!InitializeText())
    {
        return false;
    }

    // 設定ファイルは無くても既定値で動く
    ConfigData::Load(m_option);

    //--------------------------------------------------------
    // 音声（設計書 19-2.12）
    // 初期化に失敗しても、以降の再生要求が無視されるだけ。
    //--------------------------------------------------------
    Audio::Initialize();

    Audio::SetVolume(
        m_option.masterVolume,
        m_option.bgmVolume,
        m_option.seVolume
    );

    m_optionScene.SetVolume(
        m_option.masterVolume,
        m_option.bgmVolume,
        m_option.seVolume
    );

    if (!m_gameScene.Initialize(
        m_d2dRenderTarget.Get(),
        m_writeFactory.Get()))
    {
        MessageBox(
            nullptr,
            L"GameSceneの初期化に失敗しました。\n"
            L"Assets\\texture フォルダが実行ファイルと同じ場所にあるか確認してください。",
            L"BULLET DRAFT",
            MB_OK | MB_ICONERROR
        );

        return false;
    }

    //--------------------------------------------------------
    // Scene が共有する描画道具
    //--------------------------------------------------------
    m_sceneContext.renderTarget = m_d2dRenderTarget.Get();
    m_sceneContext.titleFormat = m_titleFormat.Get();
    m_sceneContext.headerFormat = m_headerFormat.Get();
    m_sceneContext.menuFormat = m_menuFormat.Get();
    m_sceneContext.bodyFormat = m_bodyFormat.Get();
    m_sceneContext.brush = m_brush.Get();

    if (m_sceneResources.Initialize(m_d2dRenderTarget.Get()))
    {
        m_sceneContext.titleBackground =
            m_sceneResources.GetBitmap(L"Assets\\texture\\title_bg.png");

        //----------------------------------------------------
        // タイトルで流す3ステージの背景と、立たせるキャラクター
        //----------------------------------------------------
        m_sceneContext.scenery[0] =
            m_sceneResources.GetBitmap(L"Assets\\texture\\Forest.png");
        m_sceneContext.scenery[1] =
            m_sceneResources.GetBitmap(L"Assets\\texture\\Plain.png");
        m_sceneContext.scenery[2] =
            m_sceneResources.GetBitmap(L"Assets\\texture\\Surberb.png");

        m_sceneContext.playerSheet[0] =
            m_sceneResources.GetBitmap(L"Assets\\texture\\player_sheet.png");
        m_sceneContext.playerSheet[1] =
            m_sceneResources.GetBitmap(L"Assets\\texture\\player2_sheet.png");

        m_sceneContext.titleLayer[0] =
            m_sceneResources.GetBitmap(L"Assets\\texture\\title_far.png");
        m_sceneContext.titleLayer[1] =
            m_sceneResources.GetBitmap(L"Assets\\texture\\title_mid.png");
        m_sceneContext.titleLayer[2] =
            m_sceneResources.GetBitmap(L"Assets\\texture\\title_near.png");

        m_sceneContext.controlsBackground =
            m_sceneResources.GetBitmap(L"Assets\\texture\\controls_bg.png");

        m_sceneContext.gameOverBackground =
            m_sceneResources.GetBitmap(L"Assets\\texture\\gameover_bg.png");

        m_sceneContext.gameClearBackground =
            m_sceneResources.GetBitmap(L"Assets\\texture\\gameclear_bg.png");
    }
    m_sceneContext.screenWidth = static_cast<float>(SCREEN_WIDTH);
    m_sceneContext.screenHeight = static_cast<float>(SCREEN_HEIGHT);

    m_startScene.Reset();

    Audio::PlayBgm(BgmPath::Title);

    m_lastUpdate = std::chrono::steady_clock::now();

    return true;
}

//============================================================
// 更新
//============================================================

Scene* Application::GetCurrentScene()
{
    switch (m_currentScene)
    {
    case SceneType::Start:       return &m_startScene;
    case SceneType::LevelSelect: return &m_levelSelectScene;
    case SceneType::Result:      return &m_resultScene;
    case SceneType::GameOver:    return &m_gameOverScene;
    case SceneType::GameClear:   return &m_gameClearScene;
    case SceneType::Controls:    return &m_controlsScene;
    case SceneType::Ranking:     return &m_rankingScene;
    default:                     return nullptr;
    }
}

void Application::UpdateMouse()
{
    POINT cursor{};

    if (!GetCursorPos(&cursor))
    {
        return;
    }

    ScreenToClient(m_hwnd, &cursor);

    RECT client{};
    GetClientRect(m_hwnd, &client);

    const float width = static_cast<float>(client.right - client.left);
    const float height = static_cast<float>(client.bottom - client.top);

    if (width <= 0.0f || height <= 0.0f)
    {
        return;
    }

    m_mouseX = (cursor.x / width) * SCREEN_WIDTH;
    m_mouseY = (cursor.y / height) * SCREEN_HEIGHT;
}

//------------------------------------------------------------
// Scene切り替え
//------------------------------------------------------------
void Application::ChangeScene(SceneType next)
{
    if (next == SceneType::None || next == m_currentScene)
    {
        return;
    }

    switch (next)
    {
    case SceneType::Exit:
        PostQuitMessage(0);
        return;

    case SceneType::Start:
        m_startScene.Reset();
        break;

    case SceneType::LevelSelect:
        m_levelSelectScene.Reset();
        break;

    case SceneType::Option:
        m_optionScene.ResetBack();
        m_optionScene.ResetControlsRequest();
        break;

    case SceneType::Controls:
        m_controlsReturn = (m_currentScene == SceneType::Option)
            ? SceneType::Option
            : SceneType::Start;

        m_controlsScene.Reset();
        break;

    case SceneType::Ranking:
        // ゲームの結果を渡してから開く
        m_rankingScene.Begin(m_gameScene.GetResultData());
        break;

    case SceneType::Result:
        m_resultScene.SetResult(m_gameScene.GetResultData());
        m_resultScene.Reset();
        break;

    case SceneType::GameOver:
        m_gameOverScene.SetResult(m_gameScene.GetResultData());
        m_gameOverScene.Reset();
        break;

    case SceneType::GameClear:
        m_gameClearScene.SetResult(m_gameScene.GetResultData());
        m_gameClearScene.Reset();
        break;

    default:
        break;
    }

    m_currentScene = next;

    //--------------------------------------------------------
    // 画面が切り替わった合図
    //--------------------------------------------------------
    Audio::PlaySe(SeId::SceneChange);

    //--------------------------------------------------------
    // ポーズから設定（と、そこから開く操作説明）へ行く間は、
    // ステージのBGMを流したままにする。
    //
    // 音量を調整している最中に曲が変わってしまうと、
    // 大きさを確かめられないため。
    //--------------------------------------------------------
    const bool keepGameBgm =
        (m_optionReturn == SceneType::Game) &&
        (next == SceneType::Option || next == SceneType::Controls);

    if (!keepGameBgm)
    {
        ApplySceneBgm(next);
    }
}

//------------------------------------------------------------
// Scene に合わせて BGM を差し替える
//
// ゲーム中の BGM はステージごとに変わるため、
// GameScene::BeginStage() の側で鳴らしている。
//------------------------------------------------------------
void Application::ApplySceneBgm(SceneType scene)
{
    switch (scene)
    {
    case SceneType::Game:
        // ステージ側に任せる
        break;

    //----------------------------------------------------
    // 結果の3画面は短いファンファーレなので、
    // ループさせず1回だけ鳴らす。
    //----------------------------------------------------
    case SceneType::Result:
        Audio::PlayJingle(BgmPath::Result);
        break;

    case SceneType::GameOver:
        Audio::PlayJingle(BgmPath::GameOver);
        break;

    case SceneType::GameClear:
        Audio::PlayJingle(BgmPath::GameClear);
        break;

    default:
        // タイトル・ステージ選択・オプション・操作説明・ランキング
        Audio::PlayBgm(BgmPath::Title);
        break;
    }
}

void Application::Update()
{
    const auto now = std::chrono::steady_clock::now();

    float deltaTime =
        std::chrono::duration<float>(now - m_lastUpdate).count();

    m_lastUpdate = now;

    deltaTime = std::min(deltaTime, 0.05f);

    m_input.Capture();
    UpdateMouse();

    //--------------------------------------------------------
    // GameScene と OptionScene は独自に入力を取るので別扱い
    //--------------------------------------------------------
    if (m_currentScene == SceneType::Game)
    {
        m_gameScene.Update(m_hwnd);

        if (m_gameScene.IsVictory())
        {
            ChangeScene(SceneType::GameClear);
        }
        else if (m_gameScene.IsGameOver())
        {
            ChangeScene(SceneType::GameOver);
        }
        else if (m_gameScene.IsStageClearPending())
        {
            // クリアした次のステージを解放する
            m_levelSelectScene.UnlockStage(
                m_gameScene.GetStageIndex() + 1
            );

            ChangeScene(SceneType::Result);
        }
        else if (m_gameScene.IsBackToTitle())
        {
            m_gameScene.ResetBackRequest();
            ChangeScene(SceneType::Start);
        }
        else if (m_gameScene.IsOptionRequested())
        {
            //------------------------------------------------
            // ポーズ画面から設定を開く。
            // ポーズは開いたままなので、戻るとポーズ画面へ返る。
            //------------------------------------------------
            m_gameScene.ResetOptionRequest();

            m_optionReturn = SceneType::Game;

            ChangeScene(SceneType::Option);
        }

        return;
    }

    if (m_currentScene == SceneType::Option)
    {
        m_optionScene.Update(m_hwnd, m_swapChain.Get());

        if (m_optionScene.IsBack())
        {
            //------------------------------------------------
            // 変更した音量を設定ファイルへ残す（19-10.1）
            //------------------------------------------------
            m_option.masterVolume = m_optionScene.GetMasterVolume();
            m_option.bgmVolume = m_optionScene.GetBgmVolume();
            m_option.seVolume = m_optionScene.GetSeVolume();

            ConfigData::Save(m_option);

            m_optionScene.ResetBack();

            if (m_optionReturn == SceneType::Game)
            {
                //--------------------------------------------
                // ポーズから開いた場合はゲームへ戻す。
                //
                // ChangeScene を通さないのは、
                // SceneType::Game への切り替えが
                // 「ステージを始める」処理と結びついているため。
                // ここでは再開したいだけなので直接戻す。
                //--------------------------------------------
                m_gameScene.ResumeFromOption();

                m_currentScene = SceneType::Game;
                m_optionReturn = SceneType::Start;

                Audio::PlaySe(SeId::SceneChange);
            }
            else
            {
                m_optionReturn = SceneType::Start;

                ChangeScene(SceneType::Start);
            }
        }
        else if (m_optionScene.IsControlsRequested())
        {
            m_optionScene.ResetControlsRequest();
            ChangeScene(SceneType::Controls);
        }

        return;
    }

    //--------------------------------------------------------
    // 共通インターフェースを持つ Scene
    //--------------------------------------------------------
    Scene* scene = GetCurrentScene();

    if (!scene)
    {
        return;
    }

    SceneType next =
        scene->Update(m_input, m_mouseX, m_mouseY, deltaTime);

    if (next == SceneType::None)
    {
        return;
    }

    //--------------------------------------------------------
    // 操作説明は「開いた画面」へ戻す。
    // 画面の案内も「設定へ戻る」となっているので、
    // 設定から開いたときは設定へ返す。
    //--------------------------------------------------------
    if (m_currentScene == SceneType::Controls)
    {
        next = m_controlsReturn;
    }

    //--------------------------------------------------------
    // ゲームの始め方は、どの Scene から来たかで変わる
    //--------------------------------------------------------
    if (next == SceneType::Game)
    {
        if (m_currentScene == SceneType::LevelSelect)
        {
            m_gameScene.Reset(
                m_levelSelectScene.GetSelectedStage(),
                m_levelSelectScene.GetSelectedDifficulty(),
                m_levelSelectScene.GetSelectedPlayer()
            );
        }
        else if (m_currentScene == SceneType::Result)
        {
            if (m_resultScene.WantsRetry())
            {
                m_gameScene.RetryStage();
            }
            else
            {
                // NEXT：装備を引き継いだまま次のステージへ
                m_gameScene.AdvanceStage();
            }
        }
        else if (m_currentScene == SceneType::GameOver)
        {
            m_gameScene.RetryStage();
        }

        m_currentScene = SceneType::Game;
        return;
    }

    ChangeScene(next);
}

//============================================================
// 描画
//============================================================

void Application::Draw()
{
    const float clearColor[4] = { 0.005f, 0.015f, 0.030f, 1.0f };

    //--------------------------------------------------------
    // 描画先を決める。
    //   後処理があるときはオフスクリーンへ、
    //   無いときはバックバッファへ直接。
    //--------------------------------------------------------
    if (m_postEffect.IsReady())
    {
        m_postEffect.BeginScene(m_context.Get(), clearColor);
    }
    else
    {
        m_context->OMSetRenderTargets(
            1,
            m_renderTargetView.GetAddressOf(),
            nullptr
        );

        m_context->ClearRenderTargetView(
            m_renderTargetView.Get(),
            clearColor
        );
    }

    m_d2dRenderTarget->BeginDraw();

    if (m_currentScene == SceneType::Game)
    {
        m_gameScene.Draw(m_titleFormat.Get(), m_menuFormat.Get());
    }
    else if (m_currentScene == SceneType::Option)
    {
        m_optionScene.Draw(
            m_d2dRenderTarget.Get(),
            m_titleFormat.Get(),
            m_menuFormat.Get(),
            m_white.Get(),
            m_blue.Get(),
            m_gray.Get()
        );
    }
    else
    {
        Scene* scene = GetCurrentScene();

        if (scene)
        {
            scene->Draw(m_sceneContext);
        }
    }

    m_d2dRenderTarget->EndDraw();

    //--------------------------------------------------------
    // 出来上がった絵にシェーダを掛けてバックバッファへ出す。
    //
    // 被弾した直後は画面の縁を赤くしたいので、
    // ゲーム中だけ被弾の強さを渡す。
    //--------------------------------------------------------
    if (m_postEffect.IsReady())
    {
        const float damageFlash =
            (m_currentScene == SceneType::Game)
            ? m_gameScene.GetDamageFlash()
            : 0.0f;

        m_postEffect.Apply(
            m_context.Get(),
            m_renderTargetView.Get(),
            damageFlash
        );
    }

    m_swapChain->Present(1, 0);
}

//============================================================
// エントリポイント
//============================================================

Application g_application;

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    PWSTR,
    int nCmdShow
)
{
    // DPIスケーリングでマウス座標がずれるのを防ぐ
    SetProcessDPIAware();

    // WICでPNGを読むためCOMを初期化する
    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)))
    {
        return 0;
    }

    WNDCLASS windowClass{};

    windowClass.lpfnWndProc = WndProc;
    windowClass.hInstance = hInstance;
    windowClass.lpszClassName = L"BULLETDRAFTWindow";
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClass(&windowClass);

    RECT rect{ 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };

    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindow(
        windowClass.lpszClassName,
        L"BULLET DRAFT - DirectX11",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hwnd)
    {
        CoUninitialize();
        return 0;
    }

    if (!g_application.Initialize(hwnd))
    {
        CoUninitialize();
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    MSG message{};

    while (message.message != WM_QUIT)
    {
        if (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
        else
        {
            g_application.Update();
            g_application.Draw();
        }
    }

    Audio::Shutdown();

    CoUninitialize();

    return 0;
}

LRESULT CALLBACK WndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}
